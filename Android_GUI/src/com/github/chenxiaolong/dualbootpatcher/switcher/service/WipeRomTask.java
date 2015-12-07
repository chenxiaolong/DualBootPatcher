/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.WipeResult;

import java.io.IOException;

public final class WipeRomTask extends BaseServiceTask {
    private static final String TAG = WipeRomTask.class.getSimpleName();

    public final String mRomId;
    private final short[] mTargets;
    private final WipeRomTaskListener mListener;

    public short[] mTargetsSucceeded;
    public short[] mTargetsFailed;

    public interface WipeRomTaskListener {
        void onWipedRom(int taskId, String romId, short[] succeeded, short[] failed);
    }

    public WipeRomTask(int taskId, Context context, String romId, short[] targets,
                       WipeRomTaskListener listener) {
        super(taskId, context);
        mRomId = romId;
        mTargets = targets;
        mListener = listener;
    }

    @Override
    public void execute() {
        try {
            WipeResult result = MbtoolSocket.getInstance().wipeRom(
                    getContext(), mRomId, mTargets);
            mTargetsSucceeded = result.succeeded;
            mTargetsFailed = result.failed;
        } catch (IOException e) {
            Log.e(TAG, "mbtool communication error", e);
        }

        mListener.onWipedRom(getTaskId(), mRomId, mTargetsSucceeded, mTargetsFailed);
    }
}
