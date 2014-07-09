/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Build;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.AlertDialogFragment;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;

/**
 * This fragment manages (and saves, if necessary) the output from the patcher.
 * This is done so that the UI state remains synchronized after eg. an
 * orientation/configuration change.
 */
public class PatcherTaskFragment extends Fragment {
    public static final String TAG = PatcherTaskFragment.class.getSimpleName();

    private static final int REQUEST_FILE = 1234;
    private static final int REQUEST_DIFF_FILE = 2345;

    private PatcherListener mListener;
    private Context mContext;

    private final ArrayList<String> mQueueAddTask = new ArrayList<String>();
    private String mSaveUpdateTask;
    private String mSaveUpdateDetails;
    private int mSaveMaxProgress = -1;
    private int mSaveProgress = -1;
    private boolean mSaveFailed;
    private String mSaveMessage;
    private String mSaveNewFile;
    private boolean mFinishedPatching;
    private String mSaveSupportedFile;
    private boolean mSaveSupported;
    private boolean mCheckedSupported;
    private String mSaveFile;
    private String mSaveDiffFile;

    public static interface PatcherListener {
        public void addTask(String task);

        public void updateTask(String task);

        public void updateDetails(String text);

        public void setMaxProgress(int i);

        public void setProgress(int i);

        public void finishedPatching(boolean failed, String message,
                String newFile);

        public void checkedSupported(String zipFile, boolean supported);

        public void requestedFile(String file);

        public void requestedDiffFile(String file);
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(PatcherService.STATE);

                if (PatcherService.STATE_ADD_TASK.equals(state)) {
                    addTask(bundle.getString(PatcherService.RESULT_TASK));
                } else if (PatcherService.STATE_UPDATE_TASK.equals(state)) {
                    updateTask(bundle.getString(PatcherService.RESULT_TASK));
                } else if (PatcherService.STATE_UPDATE_DETAILS.equals(state)) {
                    updateDetails(bundle
                            .getString(PatcherService.RESULT_DETAILS));
                } else if (PatcherService.STATE_NEW_OUTPUT_LINE.equals(state)) {
                    String line = bundle.getString(PatcherService.RESULT_LINE);
                    String stream = bundle
                            .getString(PatcherService.RESULT_STREAM);

                    if (stream.equals(CommandUtils.STREAM_STDOUT)) {
                        updateDetails(line);
                    }
                } else if (PatcherService.STATE_SET_PROGRESS_MAX.equals(state)) {
                    int maxProgress = bundle
                            .getInt(PatcherService.RESULT_MAX_PROGRESS);
                    setMaxProgress(maxProgress);
                } else if (PatcherService.STATE_SET_PROGRESS.equals(state)) {
                    int curProgress = bundle
                            .getInt(PatcherService.RESULT_PROGRESS);
                    setProgress(curProgress);
                } else if (PatcherService.STATE_PATCHED_FILE.equals(state)) {
                    updateDetails(getActivity()
                            .getString(R.string.details_done));

                    boolean failed = bundle
                            .getBoolean(PatcherUtils.RESULT_PATCH_FILE_FAILED);
                    String message = bundle
                            .getString(PatcherUtils.RESULT_PATCH_FILE_MESSAGE);
                    String newFile = bundle
                            .getString(PatcherUtils.RESULT_PATCH_FILE_NEW_FILE);

                    finishedPatching(failed, message, newFile);
                } else if (PatcherService.STATE_CHECKED_SUPPORTED.equals(state)) {
                    String zipFile = bundle
                            .getString(PatcherService.RESULT_FILENAME);
                    boolean supported = bundle
                            .getBoolean(PatcherService.RESULT_SUPPORTED);

                    checkedSupported(zipFile, supported);
                }

                // Ignored:
                // PatcherService.STATE_FETCHED_PATCHER_INFO
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setRetainInstance(true);
    }

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        if (mContext == null) {
            mContext = getActivity().getApplicationContext();
            mContext.registerReceiver(mReceiver, new IntentFilter(
                    PatcherService.BROADCAST_INTENT));
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mContext != null) {
            mContext.unregisterReceiver(mReceiver);
        }
    }

    @Override
    public void onActivityResult(int request, int result, Intent data) {
        switch (request) {
        case REQUEST_FILE:
            if (data != null && result == Activity.RESULT_OK) {
                final String file = FileUtils.getPathFromUri(getActivity()
                        .getApplicationContext(), data.getData());

                requestedFile(file);

            }
            break;

        case REQUEST_DIFF_FILE:
            if (data != null && result == Activity.RESULT_OK) {
                final String file = FileUtils.getPathFromUri(getActivity()
                        .getApplicationContext(), data.getData());

                requestedDiffFile(file);
            }
        }

        super.onActivityResult(request, result, data);
    }

    public void startFileChooser() {
        Intent fileIntent = new Intent(Intent.ACTION_GET_CONTENT);
        if (Build.VERSION.SDK_INT < 19) {
            // Intent fileIntent = new
            // Intent(Intent.ACTION_OPEN_DOCUMENT);
            fileIntent.addCategory(Intent.CATEGORY_OPENABLE);
        }
        fileIntent.setType("application/zip");

        Context context = getActivity().getApplicationContext();
        final PackageManager pm = context.getPackageManager();
        List<ResolveInfo> list = pm.queryIntentActivities(fileIntent, 0);

        if (list.size() == 0) {
            FragmentTransaction ft = getFragmentManager().beginTransaction();
            Fragment prev = getFragmentManager().findFragmentByTag(
                    AlertDialogFragment.TAG);

            if (prev != null) {
                ft.remove(prev);
            }

            AlertDialogFragment f = AlertDialogFragment.newInstance(
                    context.getString(R.string.filemanager_missing_title),
                    context.getString(R.string.filemanager_missing_desc), null,
                    null, context.getString(R.string.ok), null);
            f.show(ft, AlertDialogFragment.TAG);
            return;
        }

        startActivityForResult(fileIntent, REQUEST_FILE);
    }

    public void startFileChooserDiff() {
        Intent fileIntent = new Intent(Intent.ACTION_GET_CONTENT);
        if (Build.VERSION.SDK_INT < 19) {
            fileIntent.addCategory(Intent.CATEGORY_OPENABLE);
        }
        // Android does not recognize any of the following:
        // - application/x-patch
        // - text/x-diff
        // - text/x-patch
        fileIntent.setType("*/*");
        startActivityForResult(fileIntent, REQUEST_DIFF_FILE);
    }

    public synchronized void attachListener(PatcherListener listener) {
        mListener = listener;

        // Process queued events
        for (String task : mQueueAddTask) {
            mListener.addTask(task);
        }
        mQueueAddTask.clear();

        if (mSaveUpdateTask != null) {
            mListener.updateTask(mSaveUpdateTask);
            mSaveUpdateTask = null;
        }

        if (mSaveUpdateDetails != null) {
            mListener.updateDetails(mSaveUpdateDetails);
            mSaveUpdateDetails = null;
        }

        if (mSaveMaxProgress != -1) {
            mListener.setMaxProgress(mSaveMaxProgress);
            mSaveMaxProgress = -1;
        }

        if (mSaveProgress != -1) {
            mListener.setProgress(mSaveProgress);
            mSaveProgress = -1;
        }

        if (mFinishedPatching) {
            mListener.finishedPatching(mSaveFailed, mSaveMessage, mSaveNewFile);
            mFinishedPatching = false;
        }

        if (mCheckedSupported) {
            mListener.checkedSupported(mSaveSupportedFile, mSaveSupported);
            mCheckedSupported = false;
        }

        if (mSaveFile != null) {
            mListener.requestedFile(mSaveFile);
            mSaveFile = null;
        }

        if (mSaveDiffFile != null) {
            mListener.requestedDiffFile(mSaveDiffFile);
            mSaveDiffFile = null;
        }
    }

    public synchronized void detachListener() {
        mListener = null;
    }

    private synchronized void addTask(String task) {
        if (mListener != null) {
            mListener.addTask(task);
        } else {
            mQueueAddTask.add(task);
        }
    }

    private synchronized void updateTask(String task) {
        if (mListener != null) {
            mListener.updateTask(task);
        } else {
            mSaveUpdateTask = task;
        }
    }

    private synchronized void updateDetails(String text) {
        if (mListener != null) {
            mListener.updateDetails(text);
        } else {
            mSaveUpdateDetails = text;
        }
    }

    private synchronized void setMaxProgress(int i) {
        if (mListener != null) {
            mListener.setMaxProgress(i);
        } else {
            mSaveMaxProgress = i;
        }
    }

    private synchronized void setProgress(int i) {
        if (mListener != null) {
            mListener.setProgress(i);
        } else {
            mSaveProgress = i;
        }
    }

    private synchronized void finishedPatching(boolean failed, String message,
            String newFile) {
        if (mListener != null) {
            mListener.finishedPatching(failed, message, newFile);
        } else {
            mFinishedPatching = true;
            mSaveFailed = failed;
            mSaveMessage = message;
            mSaveNewFile = newFile;
        }
    }

    private synchronized void checkedSupported(String zipFile, boolean supported) {
        if (mListener != null) {
            mListener.checkedSupported(zipFile, supported);
        } else {
            mCheckedSupported = true;
            mSaveSupportedFile = zipFile;
            mSaveSupported = supported;
        }
    }

    private synchronized void requestedFile(String file) {
        if (mListener != null) {
            mListener.requestedFile(file);
        } else {
            mSaveFile = file;
        }
    }

    private synchronized void requestedDiffFile(String file) {
        if (mListener != null) {
            mListener.requestedDiffFile(file);
        } else {
            mSaveDiffFile = file;
        }
    }
}
