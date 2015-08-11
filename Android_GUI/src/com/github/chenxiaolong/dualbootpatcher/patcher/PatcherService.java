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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Patcher;

public class PatcherService extends IntentService {
    private static final String TAG = PatcherService.class.getSimpleName();
    public static final String BROADCAST_INTENT = "com.chenxiaolong.github.dualbootpatcher.BROADCAST_PATCHER_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_PATCH_FILE = "patch_file";

    public static final String STATE = "state";
    public static final String STATE_UPDATE_DETAILS = "update_details";
    public static final String STATE_UPDATE_PROGRESS = "update_progress";
    public static final String STATE_UPDATE_FILES = "update_files";
    public static final String STATE_PATCHED_FILE = "patched_file";

    public static final String RESULT_DETAILS = "details";
    public static final String RESULT_BYTES = "bytes";
    public static final String RESULT_MAX_BYTES = "max_bytes";
    public static final String RESULT_FILES = "files";
    public static final String RESULT_MAX_FILES = "max_files";

    public PatcherService() {
        super(TAG);
    }

    private void updateDetails(String details) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_DETAILS);
        i.putExtra(RESULT_DETAILS, details);
        sendBroadcast(i);
    }

    private void updateProgress(long bytes, long maxBytes) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_PROGRESS);
        i.putExtra(RESULT_BYTES, bytes);
        i.putExtra(RESULT_MAX_BYTES, maxBytes);
        sendBroadcast(i);
    }

    public void updateFiles(long files, long maxFiles) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_FILES);
        i.putExtra(RESULT_FILES, files);
        i.putExtra(RESULT_MAX_FILES, maxFiles);
        sendBroadcast(i);
    }

    private void onPatchedFile(boolean failed, int errorCode, String newFile) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_PATCHED_FILE);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_FAILED, failed);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_ERROR_CODE, errorCode);
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
            @Override
            public void onProgressUpdated(long bytes, long maxBytes) {
                updateProgress(bytes, maxBytes);
            }

            @Override
            public void onFilesUpdated(long files, long maxFiles) {
                builder.setContentText(String.format(getString(
                        R.string.overall_progress_files), files, maxFiles));
                builder.setProgress((int) maxFiles, (int) files, false);
                nm.notify(1, builder.build());

                updateFiles(files, maxFiles);
            }

            @Override
            public void onDetailsUpdated(String text) {
                updateDetails(text);
            }
        };

        Bundle result = PatcherUtils.patchFile(this, data, listener);

        if (result != null) {
            String newFile = result.getString(PatcherUtils.RESULT_PATCH_FILE_NEW_FILE);
            int errorCode = result.getInt(PatcherUtils.RESULT_PATCH_FILE_ERROR_CODE);
            boolean failed = result.getBoolean(PatcherUtils.RESULT_PATCH_FILE_FAILED);
            onPatchedFile(failed, errorCode, newFile);
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
