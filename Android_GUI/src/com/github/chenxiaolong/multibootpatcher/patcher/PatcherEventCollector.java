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
import com.github.chenxiaolong.multibootpatcher.EventCollector;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;

import java.util.List;

public class PatcherEventCollector extends EventCollector {
    public static final String TAG = PatcherEventCollector.class.getSimpleName();

    private static final int REQUEST_FILE = 1234;

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

    @Override
    public void onActivityResult(int request, int result, Intent data) {
        switch (request) {
        case REQUEST_FILE:
            if (data != null && result == Activity.RESULT_OK) {
                final String file = FileUtils.getPathFromUri(mContext, data.getData());

                sendEvent(new RequestedFileEvent(file));
            }
            break;
        }

        super.onActivityResult(request, result, data);
    }

    // Choose files

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

    // Events

    public class UpdateDetailsEvent extends BaseEvent {
        String text;

        public UpdateDetailsEvent(String text) {
            this.text = text;
        }
    }

    public class SetMaxProgressEvent extends BaseEvent {
        int maxProgress;

        public SetMaxProgressEvent(int maxProgress) {
            this.maxProgress = maxProgress;
        }
    }

    public class SetProgressEvent extends BaseEvent {
        int progress;

        public SetProgressEvent(int progress) {
            this.progress = progress;
        }
    }

    public class FinishedPatchingEvent extends BaseEvent {
        boolean failed;
        String message;
        String newFile;

        public FinishedPatchingEvent(boolean failed, String message, String newFile) {
            this.failed = failed;
            this.message = message;
            this.newFile = newFile;
        }
    }

    public class RequestedFileEvent extends BaseEvent {
        String file;

        public RequestedFileEvent(String file) {
            this.file = file;
        }
    }
}
