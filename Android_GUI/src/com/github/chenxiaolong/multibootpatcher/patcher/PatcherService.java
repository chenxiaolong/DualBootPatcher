/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.multibootpatcher.patcher;

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Patcher;

public class PatcherService extends IntentService {
    private static final String TAG = PatcherService.class.getSimpleName();
    public static final String BROADCAST_INTENT = "com.chenxiaolong.github.dualbootpatcher.BROADCAST_PATCHER_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_PATCH_FILE = "patch_file";

    public static final String STATE = "state";
    public static final String STATE_UPDATE_DETAILS = "update_details";
    public static final String STATE_SET_PROGRESS_MAX = "set_progress_max";
    public static final String STATE_SET_PROGRESS = "set_progress";
    public static final String STATE_PATCHED_FILE = "patched_file";

    public static final String RESULT_DETAILS = "details";
    public static final String RESULT_MAX_PROGRESS = "max_progress";
    public static final String RESULT_PROGRESS = "progress";

    public PatcherService() {
        super(TAG);
    }

    private void updateDetails(String details) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_DETAILS);
        i.putExtra(RESULT_DETAILS, details);
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

    private void onPatchedFile(boolean failed, String message, String newFile) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_PATCHED_FILE);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_FAILED, failed);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_MESSAGE, message);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_NEW_FILE, newFile);
        sendBroadcast(i);
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

        Patcher.ProgressListener listener = new Patcher.ProgressListener() {
            private int mCurProgress;
            private int mMaxProgress;

            @Override
            public void onMaxProgressUpdated(int max) {
                mMaxProgress = max;

                builder.setContentText(String.format(getString(
                        R.string.overall_progress_files), mCurProgress, mMaxProgress));
                builder.setProgress(mMaxProgress, mCurProgress, false);
                nm.notify(1, builder.build());

                setProgressMax(mMaxProgress);
            }

            @Override
            public void onProgressUpdated(int value) {
                mCurProgress = value;

                builder.setContentText(String.format(getString(
                        R.string.overall_progress_files), mCurProgress, mMaxProgress));
                builder.setProgress(mMaxProgress, mCurProgress, false);
                nm.notify(1, builder.build());

                setProgress(mCurProgress);
            }

            @Override
            public void onDetailsUpdated(String text) {
                updateDetails(text);
            }
        };

        Bundle result = PatcherUtils.patchFile(this, data, listener);

        if (result != null) {
            String newFile = result.getString(PatcherUtils.RESULT_PATCH_FILE_NEW_FILE);
            String message = result.getString(PatcherUtils.RESULT_PATCH_FILE_MESSAGE);
            boolean failed = result.getBoolean(PatcherUtils.RESULT_PATCH_FILE_FAILED);
            onPatchedFile(failed, message, newFile);
        }

        nm.cancel(1);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_PATCH_FILE.equals(action)) {
            patchFile(intent.getExtras());
        }
    }
}
