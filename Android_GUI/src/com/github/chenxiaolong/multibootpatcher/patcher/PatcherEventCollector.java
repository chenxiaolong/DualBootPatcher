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

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.multibootpatcher.EventCollector;

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
                } else if (PatcherService.STATE_SET_PROGRESS_MAX.equals(state)) {
                    int maxProgress = bundle.getInt(PatcherService.RESULT_MAX_PROGRESS);
                    sendEvent(new SetMaxProgressEvent(maxProgress));
                } else if (PatcherService.STATE_SET_PROGRESS.equals(state)) {
                    int curProgress = bundle.getInt(PatcherService.RESULT_PROGRESS);
                    sendEvent(new SetProgressEvent(curProgress));
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

    public static class SetMaxProgressEvent extends BaseEvent {
        int maxProgress;

        public SetMaxProgressEvent(int maxProgress) {
            this.maxProgress = maxProgress;
        }
    }

    public static class SetProgressEvent extends BaseEvent {
        int progress;

        public SetProgressEvent(int progress) {
            this.progress = progress;
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
