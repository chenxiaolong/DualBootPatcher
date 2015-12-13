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

    private final GetRomsStateTaskListener mListener;

    public RomInformation[] mRoms;
    public RomInformation mCurrentRom;
    public String mActiveRomId;
    public KernelStatus mKernelStatus;

    public interface GetRomsStateTaskListener extends BaseServiceTaskListener {
        void onGotRomsState(int taskId, RomInformation[] roms, RomInformation currentRom,
                            String activeRomId, KernelStatus kernelStatus);
    }

    public GetRomsStateTask(int taskId, Context context, GetRomsStateTaskListener listener) {
        super(taskId, context);
        mListener = listener;
    }

    @Override
    public void execute() {
        mRoms = RomUtils.getRoms(getContext());
        mCurrentRom = RomUtils.getCurrentRom(getContext());

        long start = System.currentTimeMillis();
        obtainBootPartitionInfo();
        long end = System.currentTimeMillis();

        Log.d(TAG, "It took " + (end - start) + " milliseconds to complete boot image checks");
        Log.d(TAG, "Current boot partition ROM ID: " + mActiveRomId);
        Log.d(TAG, "Kernel status: " + mKernelStatus.name());

        mListener.onGotRomsState(getTaskId(), mRoms, mCurrentRom, mActiveRomId, mKernelStatus);
    }

    private void obtainBootPartitionInfo() {
        mActiveRomId = null;
        mKernelStatus = KernelStatus.UNKNOWN;

        File tmpImageFile = new File(getContext().getCacheDir() + File.separator + "boot.img");
        if (SwitcherUtils.copyBootPartition(getContext(), tmpImageFile)) {
            try {
                mActiveRomId = SwitcherUtils.getBootImageRomId(getContext(), tmpImageFile);
                mKernelStatus = SwitcherUtils.compareRomBootImage(mCurrentRom, tmpImageFile);
            } finally {
                tmpImageFile.delete();
            }
        }
    }
}
