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
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.WipeResult;

import org.apache.commons.io.IOUtils;

import java.io.IOException;

public final class WipeRomTask extends BaseServiceTask {
    private static final String TAG = WipeRomTask.class.getSimpleName();

    private final String mRomId;
    private final short[] mTargets;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private short[] mTargetsSucceeded;
    private short[] mTargetsFailed;

    public interface WipeRomTaskListener extends BaseServiceTaskListener, MbtoolErrorListener {
        void onWipedRom(int taskId, String romId, short[] succeeded, short[] failed);
    }

    public WipeRomTask(int taskId, Context context, String romId, short[] targets) {
        super(taskId, context);
        mRomId = romId;
        mTargets = targets;
    }

    @Override
    public void execute() {
        MbtoolConnection conn = null;

        try {
            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            WipeResult result = iface.wipeRom(mRomId, mTargets);
            synchronized (mStateLock) {
                mTargetsSucceeded = result.succeeded;
                mTargetsFailed = result.failed;
            }
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
            sendOnWipedRom();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        // Resend result if completed
        synchronized (mStateLock) {
            if (mFinished) {
                sendOnWipedRom();
            }
        }
    }

    private void sendOnWipedRom() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((WipeRomTaskListener) listener).onWipedRom(
                        getTaskId(), mRomId, mTargetsSucceeded, mTargetsFailed);
            }
        });
    }

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((WipeRomTaskListener) listener).onMbtoolConnectionFailed(getTaskId(), reason);
            }
        });
    }
}
