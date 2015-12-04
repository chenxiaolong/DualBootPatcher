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

package com.github.chenxiaolong.dualbootpatcher;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

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
        Intent intent = FileUtils.getFileChooserIntent(mContext);
        if (intent == null) {
            FileUtils.showMissingFileChooserDialog(mContext, getFragmentManager());
        } else {
            startActivityForResult(intent, REQUEST_FILE);
        }
    }

    // Events

    public static class RequestedFileEvent extends BaseEvent {
        public String file;

        public RequestedFileEvent(String file) {
            this.file = file;
        }
    }
}
