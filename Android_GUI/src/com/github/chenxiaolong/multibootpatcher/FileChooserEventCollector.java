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

package com.github.chenxiaolong.multibootpatcher;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.Build;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.AlertDialogFragment;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;

import java.util.List;

public class FileChooserEventCollector extends EventCollector {
    public static final String TAG = FileChooserEventCollector.class.getSimpleName();

    private static final int REQUEST_FILE = 1234;

    private Context mContext;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (mContext == null) {
            mContext = getActivity().getApplicationContext();
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

        final PackageManager pm = mContext.getPackageManager();
        List<ResolveInfo> list = pm.queryIntentActivities(fileIntent, 0);

        if (list.size() == 0) {
            FragmentTransaction ft = getFragmentManager().beginTransaction();
            Fragment prev = getFragmentManager().findFragmentByTag(
                    AlertDialogFragment.TAG);

            if (prev != null) {
                ft.remove(prev);
            }

            AlertDialogFragment f = AlertDialogFragment.newInstance(
                    mContext.getString(R.string.filemanager_missing_title),
                    mContext.getString(R.string.filemanager_missing_desc), null,
                    null, mContext.getString(R.string.ok), null);
            f.show(ft, AlertDialogFragment.TAG);
            return;
        }

        startActivityForResult(fileIntent, REQUEST_FILE);
    }

    // Events

    public static class RequestedFileEvent extends BaseEvent {
        public String file;

        public RequestedFileEvent(String file) {
            this.file = file;
        }
    }
}
