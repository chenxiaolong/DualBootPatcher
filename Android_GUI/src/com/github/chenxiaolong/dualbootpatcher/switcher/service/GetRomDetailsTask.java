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
import java.util.concurrent.atomic.AtomicBoolean;

public final class GetRomDetailsTask extends BaseServiceTask {
    private static final String TAG = GetRomDetailsTask.class.getSimpleName();

    public final RomInformation mRomInfo;
    private final GetRomDetailsTaskListener mListener;

    public AtomicBoolean mHaveSystemSize = new AtomicBoolean(false);
    public AtomicBoolean mHaveCacheSize = new AtomicBoolean(false);
    public AtomicBoolean mHaveDataSize = new AtomicBoolean(false);
    public AtomicBoolean mHavePackagesCounts = new AtomicBoolean(false);

    public boolean mSystemSizeSuccess;
    public long mSystemSize;
    public boolean mCacheSizeSuccess;
    public long mCacheSize;
    public boolean mDataSizeSuccess;
    public long mDataSize;
    public boolean mPackagesCountsSuccess;
    public int mSystemPackages;
    public int mUpdatedPackages;
    public int mUserPackages;

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

    public GetRomDetailsTask(int taskId, Context context, RomInformation romInfo,
                             GetRomDetailsTaskListener listener) {
        super(taskId, context);
        mRomInfo = romInfo;
        mListener = listener;
    }

    @Override
    public void execute() {
        MbtoolSocket socket = MbtoolSocket.getInstance();

        // Packages counts
        mPackagesCountsSuccess = false;
        mSystemPackages = 0;
        mUpdatedPackages = 0;
        mUserPackages = 0;
        try {
            PackageCounts pc = socket.getPackagesCounts(getContext(), mRomInfo.getId());
            if (pc != null) {
                mPackagesCountsSuccess = true;
                mSystemPackages = pc.systemPackages;
                mUpdatedPackages = pc.systemUpdatePackages;
                mUserPackages = pc.nonSystemPackages;
            }
        } catch (IOException e) {
            Log.e(TAG, "mbtool connection error", e);
        }
        mListener.onRomDetailsGotPackagesCounts(getTaskId(), mRomInfo, mPackagesCountsSuccess,
                mSystemPackages, mUpdatedPackages, mUserPackages);
        mHavePackagesCounts.set(true);

        // System size
        mSystemSizeSuccess = false;
        mSystemSize = -1;
        try {
            mSystemSize = socket.pathGetDirectorySize(getContext(),
                    mRomInfo.getSystemPath(), new String[]{ "multiboot" });
            mSystemSizeSuccess = mSystemSize >= 0;
        } catch (IOException e) {
            Log.e(TAG, "mbtool connection error", e);
        }
        mListener.onRomDetailsGotSystemSize(getTaskId(), mRomInfo, mSystemSizeSuccess, mSystemSize);
        mHaveSystemSize.set(true);

        // Cache size
        mCacheSizeSuccess = false;
        mCacheSize = -1;
        try {
            mCacheSize = socket.pathGetDirectorySize(getContext(),
                    mRomInfo.getCachePath(), new String[]{ "multiboot" });
            mCacheSizeSuccess = mCacheSize >= 0;
        } catch (IOException e) {
            Log.e(TAG, "mbtool connection error", e);
        }
        mListener.onRomDetailsGotCacheSize(getTaskId(), mRomInfo, mCacheSizeSuccess, mCacheSize);
        mHaveCacheSize.set(true);

        // Data size
        mDataSizeSuccess = false;
        mDataSize = -1;
        try {
            mDataSize = socket.pathGetDirectorySize(getContext(),
                    mRomInfo.getDataPath(), new String[]{"multiboot", "media"});
            mDataSizeSuccess = mDataSize >= 0;
        } catch (IOException e) {
            Log.e(TAG, "mbtool connection error", e);
        }
        mListener.onRomDetailsGotDataSize(getTaskId(), mRomInfo, mDataSizeSuccess, mDataSize);
        mHaveDataSize.set(true);

        // Finished
        mListener.onRomDetailsFinished(getTaskId(), mRomInfo);
    }
}
