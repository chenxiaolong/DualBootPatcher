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

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.EventCollector;
import com.github.chenxiaolong.dualbootpatcher.R;

public class PatcherEventCollector extends EventCollector {
    public static final String TAG = PatcherEventCollector.class.getSimpleName();

    private Context mContext;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(PatcherService.STATE);

                if (PatcherService.STATE_UPDATE_DETAILS.equals(state)) {
                    String details = bundle.getString(PatcherService.RESULT_DETAILS);
                    sendEvent(new UpdateDetailsEvent(details));
                } else if (PatcherService.STATE_UPDATE_PROGRESS.equals(state)) {
                    long bytes = bundle.getLong(PatcherService.RESULT_BYTES);
                    long maxBytes = bundle.getLong(PatcherService.RESULT_MAX_BYTES);

                    sendEvent(new UpdateProgressEvent(bytes, maxBytes));
                } else if (PatcherService.STATE_UPDATE_FILES.equals(state)) {
                    long files = bundle.getLong(PatcherService.RESULT_FILES);
                    long maxFiles = bundle.getLong(PatcherService.RESULT_MAX_FILES);

                    sendEvent(new UpdateFilesEvent(files, maxFiles));
                } else if (PatcherService.STATE_PATCHED_FILE.equals(state)) {
                    String done = mContext.getString(R.string.details_done);
                    sendEvent(new UpdateDetailsEvent(done));

                    boolean failed = bundle.getBoolean(PatcherUtils.RESULT_PATCH_FILE_FAILED);
                    String message = bundle.getString(PatcherUtils.RESULT_PATCH_FILE_MESSAGE);
                    String newFile = bundle.getString(PatcherUtils.RESULT_PATCH_FILE_NEW_FILE);

                    sendEvent(new FinishedPatchingEvent(failed, message, newFile));
                }
            }
        }
    };

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        if (mContext == null) {
            mContext = getActivity().getApplicationContext();
            mContext.registerReceiver(mReceiver, new IntentFilter(PatcherService.BROADCAST_INTENT));
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mContext != null) {
            mContext.unregisterReceiver(mReceiver);
        }
    }

    // Events

    public static class UpdateDetailsEvent extends BaseEvent {
        String text;

        public UpdateDetailsEvent(String text) {
            this.text = text;
        }
    }

    public static class UpdateProgressEvent extends BaseEvent {
        long bytes;
        long maxBytes;

        public UpdateProgressEvent(long bytes, long maxBytes) {
            this.bytes = bytes;
            this.maxBytes = maxBytes;
        }
    }

    public static class UpdateFilesEvent extends BaseEvent {
        long files;
        long maxFiles;

        public UpdateFilesEvent(long files, long maxFiles) {
            this.files = files;
            this.maxFiles = maxFiles;
        }
    }

    public static class FinishedPatchingEvent extends BaseEvent {
        boolean failed;
        String message;
        String newFile;

        public FinishedPatchingEvent(boolean failed, String message, String newFile) {
            this.failed = failed;
            this.message = message;
            this.newFile = newFile;
        }
    }
}
