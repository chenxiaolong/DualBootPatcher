/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher.service;

import android.content.Context;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;

import org.apache.commons.io.IOUtils;

public final class UpdateMbtoolWithRootTask extends BaseServiceTask {
    private static final String TAG = UpdateMbtoolWithRootTask.class.getSimpleName();

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private boolean mSuccess;

    public interface UpdateMbtoolWithRootTaskListener extends BaseServiceTaskListener {
        void onMbtoolUpdateSucceeded(int taskId);

        void onMbtoolUpdateFailed(int taskId);
    }

    public UpdateMbtoolWithRootTask(int taskId, Context context) {
        super(taskId, context);
    }

    @Override
    public void execute() {
        // If we've reached this point, then the SignedExec upgrade should have failed, but just in
        // case, we'll retry that method.

        boolean ret;

        if ((ret = MbtoolConnection.replaceViaSignedExec(getContext()))) {
            Log.v(TAG, "Successfully updated mbtool with SignedExec method");
        } else {
            Log.w(TAG, "Failed to update mbtool with SignedExec method");

            if ((ret = MbtoolConnection.replaceViaRoot(getContext()))) {
                Log.v(TAG, "Successfully updated mbtool with root method");
            } else {
                Log.w(TAG, "Failed to update mbtool with root method");
            }
        }

        Log.d(TAG, "Attempting mbtool connection after update");

        if (ret) {
            MbtoolConnection conn = null;
            try {
                conn = new MbtoolConnection(getContext());
                MbtoolInterface iface = conn.getInterface();
                Log.d(TAG, "mbtool was updated to " + iface.getVersion());
            } catch (Exception e) {
                Log.e(TAG, "Failed to connect to mbtool", e);
                ret = false;
            } finally {
                IOUtils.closeQuietly(conn);
            }
        }

        synchronized (mStateLock) {
            mSuccess = ret;
            sendReply();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            if (mFinished) {
                sendReply();
            }
        }
    }

    private void sendReply() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                UpdateMbtoolWithRootTaskListener l = (UpdateMbtoolWithRootTaskListener) listener;
                if (mSuccess) {
                    l.onMbtoolUpdateSucceeded(getTaskId());
                } else {
                    l.onMbtoolUpdateFailed(getTaskId());
                }
            }
        });
    }
}
