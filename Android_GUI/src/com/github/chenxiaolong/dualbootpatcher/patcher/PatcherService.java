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

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandListener;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation;
import com.github.chenxiaolong.dualbootpatcher.R;

public class PatcherService extends IntentService {
    private static final String TAG = PatcherService.class.getSimpleName();
    public static final String BROADCAST_INTENT = "com.chenxiaolong.github.dualbootpatcher.BROADCAST_PATCHER_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_PATCH_FILE = "patch_file";
    public static final String ACTION_GET_PATCHER_INFORMATION = "get_patcher_information";
    public static final String ACTION_CHECK_SUPPORTED = "check_supported";

    public static final String STATE = "state";
    public static final String STATE_ADD_TASK = "add_task";
    public static final String STATE_UPDATE_TASK = "update_task";
    public static final String STATE_UPDATE_DETAILS = "update_details";
    public static final String STATE_NEW_OUTPUT_LINE = "new_output_line";
    public static final String STATE_SET_PROGRESS_MAX = "set_progress_max";
    public static final String STATE_SET_PROGRESS = "set_progress";
    public static final String STATE_FETCHED_PATCHER_INFO = "fetched_patcher_info";
    public static final String STATE_PATCHED_FILE = "patched_file";
    public static final String STATE_CHECKED_SUPPORTED = "checked_supported";

    public static final String RESULT_TASK = "task";
    public static final String RESULT_DETAILS = "details";
    public static final String RESULT_LINE = "line";
    public static final String RESULT_STREAM = "stream";
    public static final String RESULT_MAX_PROGRESS = "max_progress";
    public static final String RESULT_PROGRESS = "progress";
    public static final String RESULT_PATCHER_INFO = "patcher_info";
    public static final String RESULT_FILENAME = "filename";
    public static final String RESULT_SUPPORTED = "supported";

    private static final String PREFIX_ADD_TASK = "ADDTASK:";
    private static final String PREFIX_SET_TASK = "SETTASK:";
    private static final String PREFIX_SET_DETAILS = "SETDETAILS:";
    private static final String PREFIX_SET_MAX_PROGRESS = "SETMAXPROGRESS:";
    private static final String PREFIX_SET_PROGRESS = "SETPROGRESS:";

    public PatcherService() {
        super(TAG);
    }

    private void addTask(String task) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_ADD_TASK);
        i.putExtra(RESULT_TASK, task);
        sendBroadcast(i);
    }

    private void updateTask(String task) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_TASK);
        i.putExtra(RESULT_TASK, task);
        sendBroadcast(i);
    }

    private void updateDetails(String details) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_DETAILS);
        i.putExtra(RESULT_DETAILS, details);
        sendBroadcast(i);
    }

    private void newOutputLine(String line, String stream) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_NEW_OUTPUT_LINE);
        i.putExtra(RESULT_LINE, line);
        i.putExtra(RESULT_STREAM, stream);
        sendBroadcast(i);
    }

    private void setProgressMax(int max) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SET_PROGRESS_MAX);
        i.putExtra(RESULT_MAX_PROGRESS, max);
        sendBroadcast(i);
    }

    private void setProgress(int value) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SET_PROGRESS);
        i.putExtra(RESULT_PROGRESS, value);
        sendBroadcast(i);
    }

    private void onFetchedPatcherInformation(PatcherInformation info) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_FETCHED_PATCHER_INFO);
        i.putExtra(RESULT_PATCHER_INFO, info);
        sendBroadcast(i);
    }

    private void onPatchedFile(boolean failed, String message, String newFile) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_PATCHED_FILE);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_FAILED, failed);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_MESSAGE, message);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_NEW_FILE, newFile);
        sendBroadcast(i);
    }

    private void onCheckedSupported(String filename, boolean supported) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_CHECKED_SUPPORTED);
        i.putExtra(RESULT_FILENAME, filename);
        i.putExtra(RESULT_SUPPORTED, supported);
        sendBroadcast(i);
    }

    private void getPatcherInformation() {
        PatcherInformation info = PatcherUtils.getPatcherInformation(this);

        onFetchedPatcherInformation(info);
    }

    private void patchFile(Bundle data) {
        Intent resultIntent = new Intent(this, MainActivity.class);
        resultIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        resultIntent.setAction(Intent.ACTION_MAIN);

        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, resultIntent, 0);

        final Notification.Builder builder = new Notification.Builder(this);
        builder.setSmallIcon(R.drawable.ic_launcher);
        builder.setOngoing(true);
        builder.setContentTitle(getString(R.string.overall_progress));
        builder.setContentIntent(pendingIntent);
        builder.setProgress(0, 0, true);

        final NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        nm.notify(1, builder.build());

        CommandListener listener = new CommandListener() {
            private int mCurProgress;
            private int mMaxProgress;

            @Override
            public void onNewOutputLine(String line, String stream) {
                if (line.contains(PREFIX_ADD_TASK)) {
                    addTask(line.replace(PREFIX_ADD_TASK, ""));
                } else if (line.contains(PREFIX_SET_TASK)) {
                    updateTask(line.replace(PREFIX_SET_TASK, ""));
                } else if (line.contains(PREFIX_SET_DETAILS)) {
                    updateDetails(line.replace(PREFIX_SET_DETAILS, ""));
                } else if (line.contains(PREFIX_SET_MAX_PROGRESS)) {
                    mMaxProgress = Integer.parseInt(line.replace(PREFIX_SET_MAX_PROGRESS, ""));

                    builder.setContentText(String.format(getString(
                            R.string.overall_progress_files), mCurProgress, mMaxProgress));
                    builder.setProgress(mMaxProgress, mCurProgress, false);
                    nm.notify(1, builder.build());

                    setProgressMax(mMaxProgress);
                } else if (line.contains(PREFIX_SET_PROGRESS)) {
                    mCurProgress = Integer.parseInt(line.replace(PREFIX_SET_PROGRESS, ""));

                    builder.setContentText(String.format(getString(
                            R.string.overall_progress_files), mCurProgress, mMaxProgress));
                    builder.setProgress(mMaxProgress, mCurProgress, false);
                    nm.notify(1, builder.build());

                    setProgress(mCurProgress);
                } else {
                    newOutputLine(line, stream);
                }
            }

            @Override
            public void onCommandCompletion(CommandResult result) {
            }
        };

        Bundle result = PatcherUtils.patchFile(this, listener, data);

        if (result != null) {
            String newFile = result.getString(PatcherUtils.RESULT_PATCH_FILE_NEW_FILE);
            String message = result.getString(PatcherUtils.RESULT_PATCH_FILE_MESSAGE);
            boolean failed = result.getBoolean(PatcherUtils.RESULT_PATCH_FILE_FAILED);
            onPatchedFile(failed, message, newFile);
        }

        nm.cancel(1);
    }

    private void checkSupported(Bundle data) {
        boolean supported = PatcherUtils.isFileSupported(this, data);

        onCheckedSupported(data.getString(PatcherUtils.PARAM_FILENAME), supported);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_GET_PATCHER_INFORMATION.equals(action)) {
            getPatcherInformation();
        } else if (ACTION_PATCH_FILE.equals(action)) {
            patchFile(intent.getExtras());
        } else if (ACTION_CHECK_SUPPORTED.equals(action)) {
            checkSupported(intent.getExtras());
        }
    }
}
