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

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.KernelStatus;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.IOException;

public final class GetRomsStateTask extends BaseServiceTask {
    private static final String TAG = GetRomsStateTask.class.getSimpleName();

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private RomInformation[] mRoms;
    private RomInformation mCurrentRom;
    private String mActiveRomId;
    private KernelStatus mKernelStatus;

    public interface GetRomsStateTaskListener extends BaseServiceTaskListener, MbtoolErrorListener {
        void onGotRomsState(int taskId, RomInformation[] roms, RomInformation currentRom,
                            String activeRomId, KernelStatus kernelStatus);
    }

    public GetRomsStateTask(int taskId, Context context) {
        super(taskId, context);
    }

    @Override
    public void execute() {
        RomInformation[] roms = null;
        RomInformation currentRom = null;
        String activeRomId = null;
        KernelStatus kernelStatus = KernelStatus.UNKNOWN;

        MbtoolConnection conn = null;

        try {
            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            roms = RomUtils.getRoms(getContext(), iface);
            currentRom = RomUtils.getCurrentRom(getContext(), iface);

            long start = System.currentTimeMillis();

            File tmpImageFile = new File(getContext().getCacheDir() + File.separator + "boot.img");
            if (SwitcherUtils.copyBootPartition(getContext(), iface, tmpImageFile)) {
                try {
                    Log.d(TAG, "Getting active ROM ID");
                    activeRomId = SwitcherUtils.getBootImageRomId(tmpImageFile);
                    Log.d(TAG, "Comparing saved boot image to current boot image");
                    kernelStatus = SwitcherUtils.compareRomBootImage(currentRom, tmpImageFile);
                } finally {
                    tmpImageFile.delete();
                }
            }

            long end = System.currentTimeMillis();

            Log.d(TAG, "It took " + (end - start) + " milliseconds to complete boot image checks");
            Log.d(TAG, "Current boot partition ROM ID: " + activeRomId);
            Log.d(TAG, "Kernel status: " + kernelStatus.name());
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

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((GetRomsStateTaskListener) listener).onMbtoolConnectionFailed(getTaskId(), reason);
            }
        });
    }
}
