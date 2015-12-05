/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.content.Intent;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.BuildConfig;
import com.github.chenxiaolong.dualbootpatcher.LogUtils;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.FileInfo;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Patcher;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Patcher.ProgressListener;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.PatcherConfig;

import java.util.HashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;

public class PatcherService extends ThreadPoolService {
    private static final String TAG = PatcherService.class.getSimpleName();
    private static final String ACTION_BASE = PatcherService.class.getCanonicalName();

    private static final String THREAD_POOL_DEFAULT = "default";
    private static final String THREAD_POOL_PATCHING = "patching";
    private static final int THREAD_POOL_DEFAULT_THREADS = 2;
    private static final int THREAD_POOL_PATCHING_THREADS = 2;

    /**
     * {@inheritDoc}
     */
    @Override
    public void onCreate() {
        super.onCreate();

        addThreadPool(THREAD_POOL_DEFAULT, THREAD_POOL_DEFAULT_THREADS);
        addThreadPool(THREAD_POOL_PATCHING, THREAD_POOL_PATCHING_THREADS);
    }

    // Patcher intents

    public static final String ACTION_PATCHER_INITIALIZED =
            ACTION_BASE + ".patcher.initialized";
    public static final String ACTION_PATCHER_DETAILS_CHANGED =
            ACTION_BASE + ".patcher.details_changed";
    public static final String ACTION_PATCHER_PROGRESS_CHANGED =
            ACTION_BASE + ".patcher.progress_changed";
    public static final String ACTION_PATCHER_FILES_PROGRESS_CHANGED =
            ACTION_BASE + ".patcher.files_progress_changed";
    public static final String ACTION_PATCHER_STARTED =
            ACTION_BASE + ".patcher.started";
    public static final String ACTION_PATCHER_FINISHED =
            ACTION_BASE + ".patcher.finished";
    public static final String RESULT_PATCHER_TASK_ID = "task_id";
    public static final String RESULT_PATCHER_DETAILS = "details";
    public static final String RESULT_PATCHER_CURRENT_BYTES = "current_bytes";
    public static final String RESULT_PATCHER_MAX_BYTES = "max_bytes";
    public static final String RESULT_PATCHER_CURRENT_FILES = "current_files";
    public static final String RESULT_PATCHER_MAX_FILES = "max_files";
    public static final String RESULT_PATCHER_CANCELLED = "cancelled";
    public static final String RESULT_PATCHER_SUCCESS = "success";
    public static final String RESULT_PATCHER_ERROR_CODE = "error_code";
    public static final String RESULT_PATCHER_NEW_FILE = "new_file";

    // TODO: Won't survive out-of-memory app restart
    private static final HashMap<Integer, PatchFileTask> sPatcherTasks = new HashMap<>();
    private static final AtomicInteger sPatcherNewTaskId = new AtomicInteger(0);

    // Patcher helper methods

    private void addTask(int taskId, PatchFileTask task) {
        synchronized (sPatcherTasks) {
            sPatcherTasks.put(taskId, task);
        }
    }

    private PatchFileTask getTask(int taskId) {
        synchronized (sPatcherTasks) {
            return sPatcherTasks.get(taskId);
        }
    }

    private PatchFileTask removeTask(int taskId) {
        synchronized (sPatcherTasks) {
            return sPatcherTasks.remove(taskId);
        }
    }

    /**
     * Asynchronously initialize the patcher
     *
     * This will:
     * - Extract the data archive
     * - Clean up old data directories
     * - Create an singleton instance of {@link PatcherConfig} in {@link PatcherUtils#sPC}
     *
     * After the task finishes, {@link #ACTION_PATCHER_INITIALIZED} will be broadcast.
     */
    public void initializePatcher() {
        InitializePatcherTask task = new InitializePatcherTask();
        enqueueOperation(THREAD_POOL_DEFAULT, task);
    }

    /**
     * Get list of file patching task IDs
     *
     * NOTE: This list should correspond with what is shown in the UI.
     *
     * @return Array of file patching task IDs.
     */
    public int[] getPatchFileTaskIds() {
        synchronized (sPatcherTasks) {
            int[] taskIds = new int[sPatcherTasks.size()];
            int index = 0;
            for (int taskId : sPatcherTasks.keySet()) {
                taskIds[index++] = taskId;
            }
            return taskIds;
        }
    }

    /**
     * Add file patching task
     *
     * The task does not execute until {@link #startPatching(int)} is called.
     *
     * @param patcherId libmbp patcher ID
     * @param path Path of file to patch
     * @param device Target device
     * @param romId Target ROM ID
     * @return Task ID for the new task
     */
    public int addPatchFileTask(String patcherId, String path, Device device, String romId) {
        int taskId = sPatcherNewTaskId.getAndIncrement();
        PatchFileTask task = new PatchFileTask(taskId);
        task.mPatcherId = patcherId;
        task.mPath = path;
        task.mDevice = device;
        task.mRomId = romId;
        addTask(taskId, task);
        return taskId;
    }

    /**
     * Remove file patching task
     *
     * @param taskId Task ID
     * @return Whether the task was removed
     */
    public boolean removePatchFileTask(int taskId) {
        cancelPatching(taskId);
        PatchFileTask task = removeTask(taskId);
        return task != null;
    }

    /**
     * Asynchronously start patching a file
     *
     * During patching, the following broadcasts may be sent:
     * - {@link #ACTION_PATCHER_DETAILS_CHANGED} : Sent when libmbp reports a single-line status
     *   text item (usually the file inside the archive being processed). The data will include:
     *   - {@link #RESULT_PATCHER_TASK_ID} : int : The task ID
     *   - {@link #RESULT_PATCHER_DETAILS} : string : Status details text
     * - {@link #ACTION_PATCHER_PROGRESS_CHANGED} : Sent when libmbp reports the current progress
     *   value and the maximum progress value. Make no assumptions about the current and maximum
     *   progress values. It is not guaranteed that current <= maximum or that they are positive.
     *   The data will include:
     *   - {@link #RESULT_PATCHER_TASK_ID} : int : The task ID
     *   - {@link #RESULT_PATCHER_CURRENT_BYTES} : long : Current progress
     *   - {@link #RESULT_PATCHER_MAX_BYTES} : long : Maximum progress
     * - {@link #ACTION_PATCHER_FILES_PROGRESS_CHANGED} : Sent when libmbp reports the current
     *   progress and maximum progress in terms of the number of files within the archive that have
     *   been processed. Make no assumptions about the current and maximum progress values. It is
     *   not guaranteed that current <= maximum or that they are positive. The data will include:
     *   - {@link #RESULT_PATCHER_TASK_ID} : int : The task ID
     *   - {@link #RESULT_PATCHER_CURRENT_FILES} : long : Current files progress
     *   - {@link #RESULT_PATCHER_MAX_FILES} : long : Maximum files progress
     *
     * When the patching is finished, {@link #ACTION_PATCHER_FINISHED} will be broadcast and the
     * data will include:
     * - {@link #RESULT_PATCHER_TASK_ID} : int : The task ID
     * - {@link #RESULT_PATCHER_CANCELLED} : boolean : Whether the patching was cancelled
     * - {@link #RESULT_PATCHER_SUCCESS} : boolean : Whether the patching succeeded
     * - {@link #RESULT_PATCHER_ERROR_CODE} : int : libmbp error code (if patching failed)
     * - {@link #RESULT_PATCHER_NEW_FILE} : string : Path to newly patched file (if patching succeeded)
     *
     * @param taskId Task ID
     */
    public void startPatching(int taskId) {
        PatchFileTask task = getTask(taskId);
        enqueueOperation(THREAD_POOL_PATCHING, task);
    }

    /**
     * Cancel file patching
     *
     * This method will attempt to cancel a patching operation in progress. The task will only be
     * cancelled if the corresponding libmbp patcher respects the cancelled flag and stops when it
     * is set. If a task has been cancelled, {@link #ACTION_PATCHER_FINISHED} will be broadcast in
     * the same manner as described in {@link #startPatching(int)}. The returned error code may or
     * may not specify that the task is cancelled.
     *
     * @param taskId Task ID
     */
    public void cancelPatching(int taskId) {
        PatchFileTask task = getTask(taskId);
        cancelOperation(THREAD_POOL_PATCHING, task);
        task.cancel();
    }

    private void enforceQueuedState(PatchFileTask task) {
        PatchFileState state = task.mState.get();
        if (state != PatchFileState.QUEUED) {
            throw new IllegalStateException(
                    "Cannot change task properties in " + state.name() + " state");
        }
    }

    public void setPatcherId(int taskId, String patcherId) {
        PatchFileTask task = getTask(taskId);
        enforceQueuedState(task);
        task.mPatcherId = patcherId;
    }

    public String getPatcherId(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mPatcherId;
    }

    public void setPath(int taskId, String path) {
        PatchFileTask task = getTask(taskId);
        enforceQueuedState(task);
        task.mPath = path;
    }

    public String getPath(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mPath;
    }

    public void setDevice(int taskId, Device device) {
        PatchFileTask task = getTask(taskId);
        enforceQueuedState(task);
        task.mDevice = device;
    }

    public Device getDevice(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mDevice;
    }

    public void setRomId(int taskId, String romId) {
        PatchFileTask task = getTask(taskId);
        enforceQueuedState(task);
        task.mRomId = romId;
    }

    public String getRomId(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mRomId;
    }

    public PatchFileState getState(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mState.get();
    }

    public String getDetails(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mDetails.get();
    }

    public long getCurrentBytes(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mBytes.get();
    }

    public long getMaximumBytes(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mMaxBytes.get();
    }

    public long getCurrentFiles(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mFiles.get();
    }

    public long getMaximumFiles(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mMaxFiles.get();
    }

    public boolean isSuccessful(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mSuccessful.get();
    }

    public int getErrorCode(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mErrorCode.get();
    }

    public String getNewPath(int taskId) {
        PatchFileTask task = getTask(taskId);
        return task.mNewPath.get();
    }

    // Patcher event dispatch methods

    private void onPatcherInitialized() {
        Intent intent = new Intent(ACTION_PATCHER_INITIALIZED);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void onPatcherUpdateDetails(int taskId, String details) {
        Intent intent = new Intent(ACTION_PATCHER_DETAILS_CHANGED);
        intent.putExtra(RESULT_PATCHER_TASK_ID, taskId);
        intent.putExtra(RESULT_PATCHER_DETAILS, details);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void onPatcherUpdateProgress(int taskId, long bytes, long maxBytes) {
        Intent intent = new Intent(ACTION_PATCHER_PROGRESS_CHANGED);
        intent.putExtra(RESULT_PATCHER_TASK_ID, taskId);
        intent.putExtra(RESULT_PATCHER_CURRENT_BYTES, bytes);
        intent.putExtra(RESULT_PATCHER_MAX_BYTES, maxBytes);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void onPatcherUpdateFilesProgress(int taskId, long files, long maxFiles) {
        Intent intent = new Intent(ACTION_PATCHER_FILES_PROGRESS_CHANGED);
        intent.putExtra(RESULT_PATCHER_TASK_ID, taskId);
        intent.putExtra(RESULT_PATCHER_CURRENT_FILES, files);
        intent.putExtra(RESULT_PATCHER_MAX_FILES, maxFiles);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void onPatcherStarted(int taskId) {
        Intent intent = new Intent(ACTION_PATCHER_STARTED);
        intent.putExtra(RESULT_PATCHER_TASK_ID, taskId);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    private void onPatcherFinished(int taskId, boolean cancelled, boolean ret, int errorCode,
                                   String newPath) {
        Intent intent = new Intent(ACTION_PATCHER_FINISHED);
        intent.putExtra(RESULT_PATCHER_TASK_ID, taskId);
        intent.putExtra(RESULT_PATCHER_CANCELLED, cancelled);
        intent.putExtra(RESULT_PATCHER_SUCCESS, ret);
        intent.putExtra(RESULT_PATCHER_ERROR_CODE, errorCode);
        intent.putExtra(RESULT_PATCHER_NEW_FILE, newPath);
        LocalBroadcastManager.getInstance(this).sendBroadcast(intent);
    }

    // Patcher task

    private final class PatchFileTask implements Runnable, ProgressListener {
        /** Task ID */
        private final int mTaskId;
        /** libmbp {@link Patcher} object */
        private Patcher mPatcher;
        /** Whether this task has already been executed */
        private boolean mExecuted;

        // Patching information

        /** Patcher ID for creating {@link #mPatcher} */
        String mPatcherId;
        /** Path to file to patch */
        String mPath;
        /** Target {@link Device} */
        Device mDevice;
        /** Target ROM ID */
        String mRomId;

        // State information

        /** Patching state */
        AtomicReference<PatchFileState> mState = new AtomicReference<>(PatchFileState.QUEUED);
        /** Whether the patching was cancelled */
        AtomicBoolean mCancelled = new AtomicBoolean(false);
        /** Details text */
        AtomicReference<String> mDetails = new AtomicReference<>();
        /** Current bytes processed */
        AtomicLong mBytes = new AtomicLong(0);
        /** Maximum bytes */
        AtomicLong mMaxBytes = new AtomicLong(0);
        /** Current files processed */
        AtomicLong mFiles = new AtomicLong(0);
        /** Maximum files */
        AtomicLong mMaxFiles = new AtomicLong(0);

        // Completion information

        /** Whether patching was successful */
        AtomicBoolean mSuccessful = new AtomicBoolean(false);
        /** Error code if patching failed */
        AtomicInteger mErrorCode = new AtomicInteger(0);
        /** Path to the newly patched file */
        AtomicReference<String> mNewPath = new AtomicReference<>();

        public PatchFileTask(int taskId) {
            mTaskId = taskId;
        }

        public void cancel() {
            // If the file was patching, then it should be considered cancelled
            mCancelled.set(true);

            synchronized (this) {
                if (mPatcher != null) {
                    mPatcher.cancelPatching();
                }
            }

            // If the file has not started patching yet, allow it to be patched later
            mState.compareAndSet(PatchFileState.PENDING, PatchFileState.QUEUED);
        }

        @Override
        public void run() {
            if (mExecuted) {
                throw new IllegalStateException("Task " + mTaskId + " has already been executed!");
            } else {
                mExecuted = true;
            }

            mState.set(PatchFileState.IN_PROGRESS);
            onPatcherStarted(mTaskId);

            Log.d(TAG, "Android GUI version: " + BuildConfig.VERSION_NAME);
            Log.d(TAG, "libmbp version: " + PatcherUtils.sPC.getVersion());
            Log.d(TAG, "Patching file:");
            Log.d(TAG, "- Patcher ID: " + mPatcherId);
            Log.d(TAG, "- Path:       " + mPath);
            Log.d(TAG, "- Device:     " + mDevice.getId());
            Log.d(TAG, "- ROM ID:     " + mRomId);

            // Make sure patcher is extracted first
            PatcherUtils.initializePatcher(PatcherService.this);

            // Create patcher instance
            synchronized (this) {
                mPatcher = PatcherUtils.sPC.createPatcher(mPatcherId);
            }
            FileInfo fileInfo = new FileInfo();
            try {
                fileInfo.setDevice(mDevice);
                fileInfo.setFilename(mPath);
                fileInfo.setRomId(mRomId);

                mPatcher.setFileInfo(fileInfo);

                boolean ret = mPatcher.patchFile(this);
                mSuccessful.set(ret);
                mErrorCode.set(mPatcher.getError());
                mNewPath.set(mPatcher.newFilePath());

                boolean cancelled = mCancelled.get();

                onPatcherFinished(mTaskId, cancelled, ret, mPatcher.getError(),
                        mPatcher.newFilePath());

                // Set to complete if the task wasn't cancelled
                if (cancelled) {
                    mState.set(PatchFileState.CANCELLED);
                } else {
                    mState.set(PatchFileState.COMPLETED);
                }
            } finally {
                // Ensure we destroy allocated objects on the C++ side
                synchronized (this) {
                    PatcherUtils.sPC.destroyPatcher(mPatcher);
                    mPatcher = null;
                }
                fileInfo.destroy();

                // Save log
                LogUtils.dump("patch-file.log");
            }
        }

        @Override
        public void onProgressUpdated(long bytes, long maxBytes) {
            mBytes.set(bytes);
            mMaxBytes.set(maxBytes);
            onPatcherUpdateProgress(mTaskId, bytes, maxBytes);
        }

        @Override
        public void onFilesUpdated(long files, long maxFiles) {
            mFiles.set(files);
            mMaxFiles.set(maxFiles);
            onPatcherUpdateFilesProgress(mTaskId, files, maxFiles);
        }

        @Override
        public void onDetailsUpdated(String text) {
            mDetails.set(text);
            onPatcherUpdateDetails(mTaskId, text);
        }
    }

    private final class InitializePatcherTask implements Runnable {
        @Override
        public void run() {
            PatcherUtils.initializePatcher(PatcherService.this);
            onPatcherInitialized();
        }
    }
}
