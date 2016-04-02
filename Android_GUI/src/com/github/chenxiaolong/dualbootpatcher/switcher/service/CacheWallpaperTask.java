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
import com.github.chenxiaolong.dualbootpatcher.RomUtils.CacheWallpaperResult;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;

import org.apache.commons.io.IOUtils;

import java.io.IOException;

public final class CacheWallpaperTask extends BaseServiceTask {
    private static final String TAG = CacheWallpaperTask.class.getSimpleName();

    private final RomInformation mRomInfo;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private CacheWallpaperResult mResult;

    public interface CacheWallpaperTaskListener extends BaseServiceTaskListener,
            MbtoolErrorListener {
        void onCachedWallpaper(int taskId, RomInformation romInfo, CacheWallpaperResult result);
    }

    public CacheWallpaperTask(int taskId, Context context, RomInformation romInfo) {
        super(taskId, context);
        mRomInfo = romInfo;
    }

    @Override
    public void execute() {
        MbtoolConnection conn = null;

        CacheWallpaperResult result = CacheWallpaperResult.FAILED;

        try {
            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            result = RomUtils.cacheWallpaper(getContext(), mRomInfo, iface);
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

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((CacheWallpaperTaskListener) listener).onMbtoolConnectionFailed(getTaskId(), reason);
            }
        });
    }
}
