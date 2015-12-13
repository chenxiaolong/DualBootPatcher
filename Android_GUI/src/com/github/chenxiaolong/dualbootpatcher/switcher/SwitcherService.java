/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.RomUtils.CacheWallpaperResult;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SwitchRomResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.KernelStatus;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask
        .BaseServiceTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask.TaskState;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CacheWallpaperTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CacheWallpaperTask
        .CacheWallpaperTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateLauncherTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateLauncherTask
        .CreateLauncherTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.FlashZipsTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.FlashZipsTask.FlashZipsTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomDetailsTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomDetailsTask
        .GetRomDetailsTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomsStateTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomsStateTask
        .GetRomsStateTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask.SetKernelTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask.SwitchRomTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.UpdateRamdiskTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.UpdateRamdiskTask
        .UpdateRamdiskTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.VerifyZipTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.VerifyZipTask.VerifyZipTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.WipeRomTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.WipeRomTask.WipeRomTaskListener;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class SwitcherService extends ThreadPoolService {
    private static final String TAG = SwitcherService.class.getSimpleName();

    private static final String THREAD_POOL_DEFAULT = "default";
    private static final int THREAD_POOL_DEFAULT_THREADS = 2;

    /** List of callbacks for receiving events */
    private final ArrayList<BaseServiceTaskListener> mCallbacks = new ArrayList<>();
    /** Read/write lock for callbacks. */
    private final ReentrantReadWriteLock mCallbacksLock = new ReentrantReadWriteLock();

    public void registerCallback(BaseServiceTaskListener callback) {
        if (callback == null) {
            Log.w(TAG, "Tried to register null callback!");
            return;
        }

        try {
            mCallbacksLock.writeLock().lock();
            mCallbacks.add(callback);
        } finally {
            mCallbacksLock.writeLock().unlock();
        }
    }

    public void unregisterCallback(BaseServiceTaskListener callback) {
        if (callback == null) {
            Log.w(TAG, "Tried to unregister null callback!");
            return;
        }

        try {
            mCallbacksLock.writeLock().lock();
            if (!mCallbacks.remove(callback)) {
                Log.w(TAG, "Callback was never registered: " + callback);
            }
        } finally {
            mCallbacksLock.writeLock().unlock();
        }
    }

    private void executeAllCallbacks(CallbackRunnable runnable) {
        try {
            mCallbacksLock.readLock().lock();
            for (BaseServiceTaskListener cb : mCallbacks) {
                runnable.call(cb);
            }
        } finally {
            mCallbacksLock.readLock().unlock();
        }
    }

    private interface CallbackRunnable {
        void call(BaseServiceTaskListener callback);
    }

    private static final AtomicInteger sNewTaskId = new AtomicInteger(0);
    private static final HashMap<Integer, BaseServiceTask> sTaskCache = new HashMap<>();
    private static final ReentrantReadWriteLock sTaskCacheLock = new ReentrantReadWriteLock();

    private void addTask(int taskId, BaseServiceTask task) {
        try {
            sTaskCacheLock.writeLock().lock();
            sTaskCache.put(taskId, task);
        } finally {
            sTaskCacheLock.writeLock().unlock();
        }
    }

    private BaseServiceTask getTask(int taskId) {
        try {
            sTaskCacheLock.readLock().lock();
            return sTaskCache.get(taskId);
        } finally {
            sTaskCacheLock.readLock().unlock();
        }
    }

    private BaseServiceTask removeTask(int taskId) {
        try {
            sTaskCacheLock.writeLock().lock();
            return sTaskCache.remove(taskId);
        } finally {
            sTaskCacheLock.writeLock().unlock();
        }
    }

    private void enqueueTask(BaseServiceTask task) {
        addTask(task.getTaskId(), task);
        enqueueOperation(THREAD_POOL_DEFAULT, task);
    }

    public TaskState getCachedTaskState(int taskId) {
        BaseServiceTask task = getTask(taskId);
        return task.getState();
    }

    public boolean removeCachedTask(int taskId) {
        BaseServiceTask task = removeTask(taskId);
        return task != null;
    }

    public void enforceFinishedState(BaseServiceTask task) {
        if (task.getState() != TaskState.FINISHED) {
            throw new IllegalStateException("Tried to get result for task " + task.getTaskId() +
                    " in state " + task.getState().name());
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onCreate() {
        super.onCreate();

        addThreadPool(THREAD_POOL_DEFAULT, THREAD_POOL_DEFAULT_THREADS);
    }

    // Get ROMs state

    public int getRomsState() {
        int taskId = sNewTaskId.getAndIncrement();
        GetRomsStateTask task = new GetRomsStateTask(taskId, this, mGetRomsStateTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final GetRomsStateTaskListener mGetRomsStateTaskListener =
            new GetRomsStateTaskListener() {
        @Override
        public void onGotRomsState(final int taskId, final RomInformation[] roms,
                                   final RomInformation currentRom, final String activeRomId,
                                   final KernelStatus kernelStatus) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof GetRomsStateTaskListener) {
                        ((GetRomsStateTaskListener) callback).onGotRomsState(
                                taskId, roms, currentRom, activeRomId, kernelStatus);
                    }
                }
            });
        }
    };

    public RomInformation[] getResultRomsStateRomsList(int taskId) {
        GetRomsStateTask task = (GetRomsStateTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRoms;
    }

    public RomInformation getResultRomsStateCurrentRom(int taskId) {
        GetRomsStateTask task = (GetRomsStateTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mCurrentRom;
    }

    public String getResultRomsStateActiveRomId(int taskId) {
        GetRomsStateTask task = (GetRomsStateTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mActiveRomId;
    }

    public KernelStatus getResultRomsStateKernelStatus(int taskId) {
        GetRomsStateTask task = (GetRomsStateTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mKernelStatus;
    }

    // Switch ROM

    public int switchRom(String romId, boolean forceChecksumsUpdate) {
        int taskId = sNewTaskId.getAndIncrement();
        SwitchRomTask task = new SwitchRomTask(
                taskId, this, romId, forceChecksumsUpdate, mSwitchRomTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final SwitchRomTaskListener mSwitchRomTaskListener = new SwitchRomTaskListener() {
        @Override
        public void onSwitchedRom(final int taskId, final String romId,
                                  final SwitchRomResult result) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof SwitchRomTaskListener) {
                        ((SwitchRomTaskListener) callback).onSwitchedRom(taskId, romId, result);
                    }
                }
            });
        }
    };

    public String getResultSwitchRomRomId(int taskId) {
        SwitchRomTask task = (SwitchRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomId;
    }

    public SwitchRomResult getResultSwitchRomResult(int taskId) {
        SwitchRomTask task = (SwitchRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mResult;
    }

    // Set kernel

    public int setKernel(String romId) {
        int taskId = sNewTaskId.getAndIncrement();
        SetKernelTask task = new SetKernelTask(taskId, this, romId, mSetKernelTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final SetKernelTaskListener mSetKernelTaskListener = new SetKernelTaskListener() {
        @Override
        public void onSetKernel(final int taskId, final String romId,
                                final SetKernelResult result) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof SetKernelTaskListener) {
                        ((SetKernelTaskListener) callback).onSetKernel(taskId, romId, result);
                    }
                }
            });
        }
    };

    public String getResultSetKernelRomId(int taskId) {
        SetKernelTask task = (SetKernelTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomId;
    }

    public SetKernelResult getResultSetKernelResult(int taskId) {
        SetKernelTask task = (SetKernelTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mResult;
    }

    // Update ramdisk

    public int updateRamdisk(RomInformation romInfo) {
        int taskId = sNewTaskId.getAndIncrement();
        UpdateRamdiskTask task = new UpdateRamdiskTask(
                taskId, this, romInfo, mUpdateRamdiskTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final UpdateRamdiskTaskListener mUpdateRamdiskTaskListener =
            new UpdateRamdiskTaskListener() {
        @Override
        public void onUpdatedRamdisk(final int taskId, final RomInformation romInfo,
                                     final boolean success) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof UpdateRamdiskTaskListener) {
                        ((UpdateRamdiskTaskListener) callback).onUpdatedRamdisk(
                                taskId, romInfo, success);
                    }
                }
            });
        }
    };

    public RomInformation getResultUpdateRamdiskRom(int taskId) {
        UpdateRamdiskTask task = (UpdateRamdiskTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomInfo;
    }

    public boolean getResultUpdateRamdiskSuccess(int taskId) {
        UpdateRamdiskTask task = (UpdateRamdiskTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mSuccess;
    }

    // Create launcher

    public int createLauncher(RomInformation romInfo, boolean reboot) {
        int taskId = sNewTaskId.getAndIncrement();
        CreateLauncherTask task = new CreateLauncherTask(
                taskId, this, romInfo, reboot, mCreateLauncherTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final CreateLauncherTaskListener mCreateLauncherTaskListener =
            new CreateLauncherTaskListener() {
        @Override
        public void onCreatedLauncher(final int taskId, final RomInformation romInfo) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof CreateLauncherTaskListener) {
                        ((CreateLauncherTaskListener) callback).onCreatedLauncher(taskId, romInfo);
                    }
                }
            });
        }
    };

    // Verify zip

    public int verifyZip(String path) {
        int taskId = sNewTaskId.getAndIncrement();
        VerifyZipTask task = new VerifyZipTask(taskId, this, path, mVerifyZipTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final VerifyZipTaskListener mVerifyZipTaskListener = new VerifyZipTaskListener() {
        @Override
        public void onVerifiedZip(final int taskId, final String path,
                                  final VerificationResult result, final String romId) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof VerifyZipTaskListener) {
                        ((VerifyZipTaskListener) callback).onVerifiedZip(
                                taskId, path, result, romId);
                    }
                }
            });
        }
    };

    public String getResultVerifyZipPath(int taskId) {
        VerifyZipTask task = (VerifyZipTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mPath;
    }

    public VerificationResult getResultVerifyZipResult(int taskId) {
        VerifyZipTask task = (VerifyZipTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mResult;
    }

    public String getResultVerifyZipRomId(int taskId) {
        VerifyZipTask task = (VerifyZipTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomId;
    }

    // In-app flashing

    public int flashZips(PendingAction[] actions) {
        int taskId = sNewTaskId.getAndIncrement();
        FlashZipsTask task = new FlashZipsTask(taskId, this, actions, mFlashZipsTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final FlashZipsTaskListener mFlashZipsTaskListener = new FlashZipsTaskListener() {
        @Override
        public void onFlashedZips(final int taskId, final int totalActions,
                                  final int failedActions) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof FlashZipsTaskListener) {
                        ((FlashZipsTaskListener) callback).onFlashedZips(
                                taskId, totalActions, failedActions);
                    }
                }
            });
        }

        @Override
        public void onCommandOutput(final int taskId, final String line) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof FlashZipsTaskListener) {
                        ((FlashZipsTaskListener) callback).onCommandOutput(taskId, line);
                    }
                }
            });
        }
    };

    public int getResultFlashZipsTotalActions(int taskId) {
        FlashZipsTask task = (FlashZipsTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mTotal;
    }

    public int getResultFlashZipsFailedActions(int taskId) {
        FlashZipsTask task = (FlashZipsTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mFailed;
    }

    public String[] getResultFlashZipsOutputLines(int taskId) {
        FlashZipsTask task = (FlashZipsTask) getTask(taskId);
        return task.getLines();
    }

    // Wipe ROM

    public int wipeRom(String romId, short[] targets) {
        int taskId = sNewTaskId.getAndIncrement();
        WipeRomTask task = new WipeRomTask(taskId, this, romId, targets, mWipeRomTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final WipeRomTaskListener mWipeRomTaskListener = new WipeRomTaskListener() {
        @Override
        public void onWipedRom(final int taskId, final String romId, final short[] succeeded,
                               final short[] failed) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof WipeRomTaskListener) {
                        ((WipeRomTaskListener) callback).onWipedRom(
                                taskId, romId, succeeded, failed);
                    }
                }
            });
        }
    };

    public String getResultWipeRomRom(int taskId) {
        WipeRomTask task = (WipeRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomId;
    }

    public short[] getResultWipeRomTargetsSucceeded(int taskId) {
        WipeRomTask task = (WipeRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mTargetsSucceeded;
    }

    public short[] getResultWipeRomTargetsFailed(int taskId) {
        WipeRomTask task = (WipeRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mTargetsFailed;
    }

    // Cache wallpaper

    public int cacheWallpaper(RomInformation info) {
        int taskId = sNewTaskId.getAndIncrement();
        CacheWallpaperTask task = new CacheWallpaperTask(
                taskId, this, info, mCacheWallpaperTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final CacheWallpaperTaskListener mCacheWallpaperTaskListener =
            new CacheWallpaperTaskListener() {
        @Override
        public void onCachedWallpaper(final int taskId, final RomInformation romInfo,
                                      final CacheWallpaperResult result) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof CacheWallpaperTaskListener) {
                        ((CacheWallpaperTaskListener) callback).onCachedWallpaper(
                                taskId, romInfo, result);
                    }
                }
            });
        }
    };

    public RomInformation getResultCacheWallpaperRom(int taskId) {
        CacheWallpaperTask task = (CacheWallpaperTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomInfo;
    }

    public CacheWallpaperResult getResultCacheWallpaperResult(int taskId) {
        CacheWallpaperTask task = (CacheWallpaperTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mResult;
    }

    // Get ROM details

    public int getRomDetails(RomInformation romInfo) {
        int taskId = sNewTaskId.getAndIncrement();
        GetRomDetailsTask task = new GetRomDetailsTask(
                taskId, this, romInfo, mGetRomDetailsTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final GetRomDetailsTaskListener mGetRomDetailsTaskListener =
            new GetRomDetailsTaskListener() {
        @Override
        public void onRomDetailsGotSystemSize(final int taskId, final RomInformation romInfo,
                                              final boolean success, final long size) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof GetRomDetailsTaskListener) {
                        ((GetRomDetailsTaskListener) callback).onRomDetailsGotSystemSize(
                                taskId, romInfo, success, size);
                    }
                }
            });
        }

        @Override
        public void onRomDetailsGotCacheSize(final int taskId, final RomInformation romInfo,
                                             final boolean success, final long size) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof GetRomDetailsTaskListener) {
                        ((GetRomDetailsTaskListener) callback).onRomDetailsGotCacheSize(
                                taskId, romInfo, success, size);
                    }
                }
            });
        }

        @Override
        public void onRomDetailsGotDataSize(final int taskId, final RomInformation romInfo,
                                            final boolean success, final long size) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof GetRomDetailsTaskListener) {
                        ((GetRomDetailsTaskListener) callback).onRomDetailsGotDataSize(
                                taskId, romInfo, success, size);
                    }
                }
            });
        }

        @Override
        public void onRomDetailsGotPackagesCounts(final int taskId, final RomInformation romInfo,
                                                  final boolean success, final int systemPackages,
                                                  final int updatedPackages,
                                                  final int userPackages) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof GetRomDetailsTaskListener) {
                        ((GetRomDetailsTaskListener) callback).onRomDetailsGotPackagesCounts(taskId,
                                romInfo, success, systemPackages, updatedPackages, userPackages);
                    }
                }
            });
        }

        @Override
        public void onRomDetailsFinished(final int taskId, final RomInformation romInfo) {
            executeAllCallbacks(new CallbackRunnable() {
                @Override
                public void call(BaseServiceTaskListener callback) {
                    if (callback instanceof GetRomDetailsTaskListener) {
                        ((GetRomDetailsTaskListener) callback).onRomDetailsFinished(
                                taskId, romInfo);
                    }
                }
            });
        }
    };

    public RomInformation getResultRomDetailsRom(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomInfo;
    }

    public boolean hasResultRomDetailsSystemSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        return task.mHaveSystemSize.get();
    }

    public boolean hasResultRomDetailsCacheSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        return task.mHaveCacheSize.get();
    }

    public boolean hasResultRomDetailsDataSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        return task.mHaveDataSize.get();
    }

    public boolean hasResultRomDetailsPackagesCounts(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        return task.mHavePackagesCounts.get();
    }

    public boolean getResultRomDetailsSystemSizeSuccess(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveSystemSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting system size");
        }
        return task.mSystemSizeSuccess;
    }

    public long getResultRomDetailsSystemSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveSystemSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting system size");
        }
        return task.mSystemSize;
    }

    public boolean getResultRomDetailsCacheSizeSuccess(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveCacheSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting cache size");
        }
        return task.mCacheSizeSuccess;
    }

    public long getResultRomDetailsCacheSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveCacheSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting cache size");
        }
        return task.mCacheSize;
    }

    public boolean getResultRomDetailsDataSizeSuccess(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveDataSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting data size");
        }
        return task.mDataSizeSuccess;
    }

    public long getResultRomDetailsDataSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveDataSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting data size");
        }
        return task.mDataSize;
    }

    public boolean getResultRomDetailsPackagesCountsSuccess(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHavePackagesCounts.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting packages counts");
        }
        return task.mPackagesCountsSuccess;
    }

    public int getResultRomDetailsSystemPackages(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHavePackagesCounts.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting packages counts");
        }
        return task.mSystemPackages;
    }

    public int getResultRomDetailsUpdatedPackages(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHavePackagesCounts.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting packages counts");
        }
        return task.mSystemPackages;
    }

    public int getResultRomDetailsUserPackages(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHavePackagesCounts.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting packages counts");
        }
        return task.mSystemPackages;
    }
}
