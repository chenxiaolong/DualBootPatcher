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
import android.net.Uri;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.LogUtils;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult;

public final class VerifyZipTask extends BaseServiceTask {
    private static final String TAG = VerifyZipTask.class.getSimpleName();

    private final Uri mUri;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private VerificationResult mResult;
    private String mRomId;

    public interface VerifyZipTaskListener extends BaseServiceTaskListener {
        void onVerifiedZip(int taskId, Uri uri, VerificationResult result, String romId);
    }

    public VerifyZipTask(int taskId, Context context, Uri uri) {
        super(taskId, context);
        mUri = uri;
    }

    @Override
    public void execute() {
        Log.d(TAG, "Verifying zip file: " + mUri);

        VerificationResult result = SwitcherUtils.verifyZipMbtoolVersion(getContext(), mUri);
        String romId = SwitcherUtils.getTargetInstallLocation(getContext(), mUri);

        LogUtils.dump("verify-zip.log");

        synchronized (mStateLock) {
            mResult = result;
            mRomId = romId;
            sendOnVerifiedZip();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            if (mFinished) {
                sendOnVerifiedZip();
            }
        }
    }

    private void sendOnVerifiedZip() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((VerifyZipTaskListener) listener).onVerifiedZip(
                        getTaskId(), mUri, mResult, mRomId);
            }
        });
    }
}
