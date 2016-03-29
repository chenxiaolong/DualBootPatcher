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

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.PackageCounts;

import java.io.IOException;

public final class GetRomDetailsTask extends BaseServiceTask {
    private static final String TAG = GetRomDetailsTask.class.getSimpleName();

    private final RomInformation mRomInfo;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private boolean mHaveSystemSize;
    private boolean mSystemSizeSuccess;
    private long mSystemSize;

    private boolean mHaveCacheSize;
    private boolean mCacheSizeSuccess;
    private long mCacheSize;

    private boolean mHaveDataSize;
    private boolean mDataSizeSuccess;
    private long mDataSize;

    private boolean mHavePackagesCounts;
    private boolean mPackagesCountsSuccess;
    private int mSystemPackages;
    private int mUpdatedPackages;
    private int mUserPackages;

    public interface GetRomDetailsTaskListener extends BaseServiceTaskListener {
        void onRomDetailsGotSystemSize(int taskId, RomInformation romInfo,
                                       boolean success, long size);

        void onRomDetailsGotCacheSize(int taskId, RomInformation romInfo,
                                      boolean success, long size);

        void onRomDetailsGotDataSize(int taskId, RomInformation romInfo,
                                     boolean success, long size);

        void onRomDetailsGotPackagesCounts(int taskId, RomInformation romInfo, boolean success,
                                           int systemPackages, int updatedPackages,
                                           int userPackages);

        void onRomDetailsFinished(int taskId, RomInformation romInfo);
    }

    public GetRomDetailsTask(int taskId, Context context, RomInformation romInfo) {
        super(taskId, context);
        mRomInfo = romInfo;
    }

    @Override
    public void execute() {
        MbtoolSocket socket = MbtoolSocket.getInstance();

        // Packages counts
        boolean packagesCountsSuccess = false;
        int systemPackages = 0;
        int updatedPackages = 0;
        int userPackages = 0;
        try {
            PackageCounts pc = socket.getPackagesCounts(getContext(), mRomInfo.getId());
            if (pc != null) {
                packagesCountsSuccess = true;
                systemPackages = pc.systemPackages;
                updatedPackages = pc.systemUpdatePackages;
                userPackages = pc.nonSystemPackages;
            }
        } catch (IOException e) {
            Log.e(TAG, "mbtool connection error", e);
        }

        synchronized (mStateLock) {
            mPackagesCountsSuccess = packagesCountsSuccess;
            mSystemPackages = systemPackages;
            mUpdatedPackages = updatedPackages;
            mUserPackages = userPackages;
            sendOnRomDetailsGotPackagesCounts();
            mHavePackagesCounts = true;
        }

        // System size
        boolean systemSizeSuccess = false;
        long systemSize = -1;
        try {
            systemSize = socket.pathGetDirectorySize(getContext(),
                    mRomInfo.getSystemPath(), new String[]{ "multiboot" });
            systemSizeSuccess = systemSize >= 0;
        } catch (IOException e) {
            Log.e(TAG, "mbtool connection error", e);
        }

        synchronized (mStateLock) {
            mSystemSizeSuccess = systemSizeSuccess;
            mSystemSize = systemSize;
            sendOnRomDetailsGotSystemSize();
            mHaveSystemSize = true;
        }

        // Cache size
        boolean cacheSizeSuccess = false;
        long cacheSize = -1;
        try {
            cacheSize = socket.pathGetDirectorySize(getContext(),
                    mRomInfo.getCachePath(), new String[]{ "multiboot" });
            cacheSizeSuccess = cacheSize >= 0;
        } catch (IOException e) {
            Log.e(TAG, "mbtool connection error", e);
        }

        synchronized (mStateLock) {
            mCacheSizeSuccess = cacheSizeSuccess;
            mCacheSize = cacheSize;
            sendOnRomDetailsGotCacheSize();
            mHaveCacheSize = true;
        }

        // Data size
        boolean dataSizeSuccess = false;
        long dataSize = -1;
        try {
            dataSize = socket.pathGetDirectorySize(getContext(),
                    mRomInfo.getDataPath(), new String[]{"multiboot", "media"});
            dataSizeSuccess = dataSize >= 0;
        } catch (IOException e) {
            Log.e(TAG, "mbtool connection error", e);
        }

        synchronized (mStateLock) {
            mDataSizeSuccess = dataSizeSuccess;
            mDataSize = dataSize;
            sendOnRomDetailsGotDataSize();
            mHaveDataSize = true;
        }

        // Finished
        synchronized (mStateLock) {
            sendOnRomDetailsFinished();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            if (mHavePackagesCounts) {
                sendOnRomDetailsGotPackagesCounts();
            }
            if (mHaveSystemSize) {
                sendOnRomDetailsGotSystemSize();
            }
            if (mHaveCacheSize) {
                sendOnRomDetailsGotCacheSize();
            }
            if (mHaveDataSize) {
                sendOnRomDetailsGotDataSize();
            }
            if (mFinished) {
                sendOnRomDetailsFinished();
            }
        }
    }

    private void sendOnRomDetailsGotSystemSize() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((GetRomDetailsTaskListener) listener).onRomDetailsGotSystemSize(
                        getTaskId(), mRomInfo, mSystemSizeSuccess, mSystemSize);
            }
        });
    }

    private void sendOnRomDetailsGotCacheSize() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((GetRomDetailsTaskListener) listener).onRomDetailsGotCacheSize(
                        getTaskId(), mRomInfo, mCacheSizeSuccess, mCacheSize);
            }
        });
    }

    private void sendOnRomDetailsGotDataSize() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((GetRomDetailsTaskListener) listener).onRomDetailsGotDataSize(
                        getTaskId(), mRomInfo, mDataSizeSuccess, mDataSize);
            }
        });
    }

    private void sendOnRomDetailsGotPackagesCounts() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((GetRomDetailsTaskListener) listener).onRomDetailsGotPackagesCounts(
                        getTaskId(), mRomInfo, mPackagesCountsSuccess, mSystemPackages,
                        mUpdatedPackages, mUserPackages);
            }
        });
    }

    private void sendOnRomDetailsFinished() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((GetRomDetailsTaskListener) listener).onRomDetailsFinished(getTaskId(), mRomInfo);
            }
        });
    }
}
