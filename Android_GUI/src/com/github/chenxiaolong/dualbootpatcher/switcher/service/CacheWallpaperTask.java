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

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.CacheWallpaperResult;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

public final class CacheWallpaperTask extends BaseServiceTask {
    private final RomInformation mRomInfo;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private CacheWallpaperResult mResult;

    public interface CacheWallpaperTaskListener extends BaseServiceTaskListener {
        void onCachedWallpaper(int taskId, RomInformation romInfo, CacheWallpaperResult result);
    }

    public CacheWallpaperTask(int taskId, Context context, RomInformation romInfo) {
        super(taskId, context);
        mRomInfo = romInfo;
    }

    @Override
    public void execute() {
        CacheWallpaperResult result = RomUtils.cacheWallpaper(getContext(), mRomInfo);

        synchronized (mStateLock) {
            mResult = result;
            sendOnCachedWallpaper();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            if (mFinished) {
                sendOnCachedWallpaper();
            }
        }
    }

    private void sendOnCachedWallpaper() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((CacheWallpaperTaskListener) listener).onCachedWallpaper(
                        getTaskId(), mRomInfo, mResult);
            }
        });
    }
}
