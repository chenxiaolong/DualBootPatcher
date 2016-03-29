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

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.KernelStatus;

import java.io.File;

public final class GetRomsStateTask extends BaseServiceTask {
    private static final String TAG = GetRomsStateTask.class.getSimpleName();

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private RomInformation[] mRoms;
    private RomInformation mCurrentRom;
    private String mActiveRomId;
    private KernelStatus mKernelStatus;

    public interface GetRomsStateTaskListener extends BaseServiceTaskListener {
        void onGotRomsState(int taskId, RomInformation[] roms, RomInformation currentRom,
                            String activeRomId, KernelStatus kernelStatus);
    }

    public GetRomsStateTask(int taskId, Context context) {
        super(taskId, context);
    }

    @Override
    public void execute() {
        RomInformation[] roms = RomUtils.getRoms(getContext());
        RomInformation currentRom = RomUtils.getCurrentRom(getContext());

        long start = System.currentTimeMillis();

        String activeRomId = null;
        KernelStatus kernelStatus = KernelStatus.UNKNOWN;

        File tmpImageFile = new File(getContext().getCacheDir() + File.separator + "boot.img");
        if (SwitcherUtils.copyBootPartition(getContext(), tmpImageFile)) {
            try {
                activeRomId = SwitcherUtils.getBootImageRomId(getContext(), tmpImageFile);
                kernelStatus = SwitcherUtils.compareRomBootImage(mCurrentRom, tmpImageFile);
            } finally {
                tmpImageFile.delete();
            }
        }

        long end = System.currentTimeMillis();

        Log.d(TAG, "It took " + (end - start) + " milliseconds to complete boot image checks");
        Log.d(TAG, "Current boot partition ROM ID: " + activeRomId);
        Log.d(TAG, "Kernel status: " + kernelStatus.name());

        synchronized (mStateLock) {
            mRoms = roms;
            mCurrentRom = currentRom;
            mActiveRomId = activeRomId;
            mKernelStatus = kernelStatus;
            sendOnGotRomsState();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            if (mFinished) {
                sendOnGotRomsState();
            }
        }
    }

    private void sendOnGotRomsState() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((GetRomsStateTaskListener) listener).onGotRomsState(
                        getTaskId(), mRoms, mCurrentRom, mActiveRomId, mKernelStatus);
            }
        });
    }
}
