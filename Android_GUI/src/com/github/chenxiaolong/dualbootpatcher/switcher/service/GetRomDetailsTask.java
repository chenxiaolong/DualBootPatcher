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

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.PackageCounts;

import org.apache.commons.io.IOUtils;

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

    public interface GetRomDetailsTaskListener extends BaseServiceTaskListener,
            MbtoolErrorListener {
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
        MbtoolConnection conn = null;

        try {
            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            // Packages counts
            try {
                getPackagesCounts(iface);
            } catch (MbtoolCommandException e) {
                Log.w(TAG, "mbtool command error", e);
            }

            // System size
            try {
                getSystemSize(iface);
            } catch (MbtoolCommandException e) {
                Log.w(TAG, "mbtool command error", e);
            }

            // Cache size
            try {
                getCacheSize(iface);
            } catch (MbtoolCommandException e) {
                Log.w(TAG, "mbtool command error", e);
            }

            // Data size
            try {
                getDataSize(iface);
            } catch (MbtoolCommandException e) {
                Log.w(TAG, "mbtool command error", e);
            }
        } catch (IOException e) {
            Log.e(TAG, "mbtool communication error", e);
        } catch (MbtoolException e) {
            Log.e(TAG, "mbtool error", e);
            sendOnMbtoolError(e.getReason());
        } finally {
            IOUtils.closeQuietly(conn);
        }

        // Finished
        synchronized (mStateLock) {
            sendOnRomDetailsFinished();
            mFinished = true;
        }
    }

    private void getPackagesCounts(MbtoolInterface iface)
            throws MbtoolException, IOException, MbtoolCommandException {
        PackageCounts pc = iface.getPackagesCounts(mRomInfo.getId());
        int systemPackages = pc.systemPackages;
        int updatedPackages = pc.systemUpdatePackages;
        int userPackages = pc.nonSystemPackages;

        synchronized (mStateLock) {
            mPackagesCountsSuccess = true;
            mSystemPackages = systemPackages;
            mUpdatedPackages = updatedPackages;
            mUserPackages = userPackages;
            sendOnRomDetailsGotPackagesCounts();
            mHavePackagesCounts = true;
        }
    }

    private void getSystemSize(MbtoolInterface iface)
            throws MbtoolException, IOException, MbtoolCommandException {
        long systemSize = iface.pathGetDirectorySize(
                mRomInfo.getSystemPath(), new String[]{ "multiboot" });
        boolean systemSizeSuccess = systemSize >= 0;

        synchronized (mStateLock) {
            mSystemSizeSuccess = systemSizeSuccess;
            mSystemSize = systemSize;
            sendOnRomDetailsGotSystemSize();
            mHaveSystemSize = true;
        }
    }

    private void getCacheSize(MbtoolInterface iface)
            throws MbtoolException, IOException, MbtoolCommandException {
        long cacheSize = iface.pathGetDirectorySize(
                mRomInfo.getCachePath(), new String[]{ "multiboot" });
        boolean cacheSizeSuccess = cacheSize >= 0;

        synchronized (mStateLock) {
            mCacheSizeSuccess = cacheSizeSuccess;
            mCacheSize = cacheSize;
            sendOnRomDetailsGotCacheSize();
            mHaveCacheSize = true;
        }
    }

    private void getDataSize(MbtoolInterface iface)
            throws MbtoolException, IOException, MbtoolCommandException {
        long dataSize = iface.pathGetDirectorySize(
                mRomInfo.getDataPath(), new String[]{"multiboot", "media"});
        boolean dataSizeSuccess = dataSize >= 0;

        synchronized (mStateLock) {
            mDataSizeSuccess = dataSizeSuccess;
            mDataSize = dataSize;
            sendOnRomDetailsGotDataSize();
            mHaveDataSize = true;
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

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((GetRomDetailsTaskListener) listener).onMbtoolConnectionFailed(getTaskId(), reason);
            }
        });
    }
}
