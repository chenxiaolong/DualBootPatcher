/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SwitchRomResult;

import org.apache.commons.io.IOUtils;

import java.io.IOException;

public final class SwitchRomTask extends BaseServiceTask {
    private static final String TAG = SwitchRomTask.class.getSimpleName();

    private final String mRomId;
    private final boolean mForceChecksumsUpdate;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private SwitchRomResult mResult;

    public interface SwitchRomTaskListener extends BaseServiceTaskListener, MbtoolErrorListener {
        void onSwitchedRom(int taskId, String romId, SwitchRomResult result);
    }

    public SwitchRomTask(int taskId, Context context, String romId, boolean forceChecksumsUpdate) {
        super(taskId, context);
        mRomId = romId;
        mForceChecksumsUpdate = forceChecksumsUpdate;
    }

    @Override
    public void execute() {
        Log.d(TAG, "Switching to " + mRomId + " (force=" + mForceChecksumsUpdate + ")");

        SwitchRomResult result = SwitchRomResult.FAILED;
        MbtoolConnection conn = null;

        try {
            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            result = iface.switchRom(getContext(), mRomId, mForceChecksumsUpdate);
        } catch (IOException e) {
            Log.e(TAG, "mbtool communication error", e);
        } catch (MbtoolException e) {
            Log.e(TAG, "mbtool error", e);
            sendOnMbtoolError(e.getReason());
        } catch (MbtoolCommandException e) {
            Log.w(TAG, "mbtool command error", e);
        } finally {
            IOUtils.closeQuietly(conn);
        }

        synchronized (mStateLock) {
            mResult = result;
            sendOnSwitchedRom();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            if (mFinished) {
                sendOnSwitchedRom();
            }
        }
    }

    private void sendOnSwitchedRom() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((SwitchRomTaskListener) listener).onSwitchedRom(getTaskId(), mRomId, mResult);
            }
        });
    }

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((SwitchRomTaskListener) listener).onMbtoolConnectionFailed(getTaskId(), reason);
            }
        });
    }
}
