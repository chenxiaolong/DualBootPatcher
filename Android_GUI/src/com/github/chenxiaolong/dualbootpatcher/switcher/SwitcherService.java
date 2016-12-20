/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.app.Notification;
import android.app.PendingIntent;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.support.v4.app.NotificationCompat;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask
        .BaseServiceTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BootUIActionTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BootUIActionTask.BootUIAction;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CacheWallpaperTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateLauncherTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomDetailsTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomsStateTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.MbtoolTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.UpdateMbtoolWithRootTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.UpdateRamdiskTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.VerifyZipTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.WipeRomTask;

import java.util.HashMap;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantReadWriteLock;

public class SwitcherService extends ThreadPoolService {
    private static final String TAG = SwitcherService.class.getSimpleName();

    private static final String THREAD_POOL_DEFAULT = "default";
    private static final int THREAD_POOL_DEFAULT_THREADS = 2;

    private static final int NOTIFICATION_ID = 1;

    public boolean addCallback(int taskId, BaseServiceTaskListener callback) {
        BaseServiceTask task = getTask(taskId);
        if (task == null) {
            Log.w(TAG, "Tried to add callback for non-existent task ID");
            return false;
        }

        return task.addListener(callback);
    }

    public boolean removeCallback(int taskId, BaseServiceTaskListener callback) {
        BaseServiceTask task = getTask(taskId);
        if (task == null) {
            Log.w(TAG, "Tried to remove callback for non-existent task ID");
            return false;
        }

        return task.removeListener(callback);
    }

    public void removeAllCallbacks(int taskId) {
        BaseServiceTask task = getTask(taskId);
        if (task == null) {
            Log.w(TAG, "Tried to remove all callbacks for non-existent task ID");
            return;
        }

        task.removeAllListeners();
    }

    private static final AtomicInteger sNewTaskId = new AtomicInteger(0);
    private static final HashMap<Integer, BaseServiceTask> sTaskCache = new HashMap<>();
    private static final ReentrantReadWriteLock sTaskCacheLock = new ReentrantReadWriteLock();

    private void addTask(int taskId, BaseServiceTask task) {
        try {
            sTaskCacheLock.writeLock().lock();
            sTaskCache.put(taskId, task);
        } finally {
            sTaskCacheLock.writeLock().unlock();
        }
    }

    private BaseServiceTask getTask(int taskId) {
        try {
            sTaskCacheLock.readLock().lock();
            return sTaskCache.get(taskId);
        } finally {
            sTaskCacheLock.readLock().unlock();
        }
    }

    private BaseServiceTask removeTask(int taskId) {
        try {
            sTaskCacheLock.writeLock().lock();
            return sTaskCache.remove(taskId);
        } finally {
            sTaskCacheLock.writeLock().unlock();
        }
    }

    public boolean enqueueTaskId(int taskId) {
        BaseServiceTask task = getTask(taskId);
        if (task != null) {
            enqueueOperation(THREAD_POOL_DEFAULT, task);
            return true;
        }
        return false;
    }

    public boolean removeCachedTask(int taskId) {
        BaseServiceTask task = removeTask(taskId);
        if (task != null) {
            task.removeAllListeners();
            return true;
        }
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onCreate() {
        super.onCreate();

        addThreadPool(THREAD_POOL_DEFAULT, THREAD_POOL_DEFAULT_THREADS);

        setForegroundNotification();
    }

    private void setForegroundNotification() {
        Intent startupIntent = new Intent(this, MainActivity.class);
        startupIntent.setAction(Intent.ACTION_MAIN);
        startupIntent.addCategory(Intent.CATEGORY_LAUNCHER);

        PendingIntent startupPendingIntent = PendingIntent.getActivity(
                this, 0,  startupIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this)
                .setSmallIcon(R.drawable.ic_launcher)
                .setContentIntent(startupPendingIntent)
                .setContentTitle(getString(R.string.switcher_service_background_service))
                .setPriority(Notification.PRIORITY_MIN);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            builder.setCategory(Notification.CATEGORY_SERVICE);
        }

        startForeground(NOTIFICATION_ID, builder.build());
    }

    // Get ROMs state

    public int getRomsState() {
        int taskId = sNewTaskId.getAndIncrement();
        GetRomsStateTask task = new GetRomsStateTask(taskId, this);
        addTask(taskId, task);
        return taskId;
    }

    // Switch ROM

    public int switchRom(String romId, boolean forceChecksumsUpdate) {
        int taskId = sNewTaskId.getAndIncrement();
        SwitchRomTask task = new SwitchRomTask(taskId, this, romId, forceChecksumsUpdate);
        addTask(taskId, task);
        return taskId;
    }

    // Set kernel

    public int setKernel(String romId) {
        int taskId = sNewTaskId.getAndIncrement();
        SetKernelTask task = new SetKernelTask(taskId, this, romId);
        addTask(taskId, task);
        return taskId;
    }

    // Update ramdisk

    public int updateRamdisk(RomInformation romInfo) {
        int taskId = sNewTaskId.getAndIncrement();
        UpdateRamdiskTask task = new UpdateRamdiskTask(taskId, this, romInfo);
        addTask(taskId, task);
        return taskId;
    }

    // Create launcher

    public int createLauncher(RomInformation romInfo, boolean reboot) {
        int taskId = sNewTaskId.getAndIncrement();
        CreateLauncherTask task = new CreateLauncherTask(taskId, this, romInfo, reboot);
        addTask(taskId, task);
        return taskId;
    }

    // Verify zip

    public int verifyZip(Uri uri) {
        int taskId = sNewTaskId.getAndIncrement();
        VerifyZipTask task = new VerifyZipTask(taskId, this, uri);
        addTask(taskId, task);
        return taskId;
    }

    // Wipe ROM

    public int wipeRom(String romId, short[] targets) {
        int taskId = sNewTaskId.getAndIncrement();
        WipeRomTask task = new WipeRomTask(taskId, this, romId, targets);
        addTask(taskId, task);
        return taskId;
    }

    // Cache wallpaper

    public int cacheWallpaper(RomInformation info) {
        int taskId = sNewTaskId.getAndIncrement();
        CacheWallpaperTask task = new CacheWallpaperTask(taskId, this, info);
        addTask(taskId, task);
        return taskId;
    }

    // Get ROM details

    public int getRomDetails(RomInformation romInfo) {
        int taskId = sNewTaskId.getAndIncrement();
        GetRomDetailsTask task = new GetRomDetailsTask(taskId, this, romInfo);
        addTask(taskId, task);
        return taskId;
    }

    // Update mbtool with root

    public int updateMbtoolWithRoot() {
        int taskId = sNewTaskId.getAndIncrement();
        UpdateMbtoolWithRootTask task = new UpdateMbtoolWithRootTask(taskId, this);
        addTask(taskId, task);
        return taskId;
    }

    // Do stuff with the boot UI

    public int bootUIAction(BootUIAction action) {
        int taskId = sNewTaskId.getAndIncrement();
        BootUIActionTask task = new BootUIActionTask(taskId, this, action);
        addTask(taskId, task);
        return taskId;
    }

    // Perform mbtool command operations

    public int mbtoolActions(MbtoolAction[] actions) {
        int taskId = sNewTaskId.getAndIncrement();
        MbtoolTask task = new MbtoolTask(taskId, this, actions);
        addTask(taskId, task);
        return taskId;
    }
}
