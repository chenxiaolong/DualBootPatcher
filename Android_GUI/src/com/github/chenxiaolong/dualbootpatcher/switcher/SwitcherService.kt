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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.support.v4.app.NotificationCompat
import android.util.Log
import android.util.SparseArray
import com.github.chenxiaolong.dualbootpatcher.MainActivity
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask.BaseServiceTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BootUIActionTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BootUIActionTask.BootUIAction
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CacheWallpaperTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateLauncherTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateRamdiskUpdaterTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomDetailsTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomsStateTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.MbtoolTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.UpdateMbtoolWithRootTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.VerifyZipTask
import com.github.chenxiaolong.dualbootpatcher.switcher.service.WipeRomTask
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write

class SwitcherService : ThreadPoolService() {
    fun addCallback(taskId: Int, callback: BaseServiceTaskListener): Boolean {
        val task = getTask(taskId)
        if (task == null) {
            Log.w(TAG, "Tried to add callback for non-existent task ID")
            return false
        }

        return task.addListener(callback)
    }

    fun removeCallback(taskId: Int, callback: BaseServiceTaskListener): Boolean {
        val task = getTask(taskId)
        if (task == null) {
            Log.w(TAG, "Tried to remove callback for non-existent task ID")
            return false
        }

        return task.removeListener(callback)
    }

    fun removeAllCallbacks(taskId: Int) {
        val task = getTask(taskId)
        if (task == null) {
            Log.w(TAG, "Tried to remove all callbacks for non-existent task ID")
            return
        }

        task.removeAllListeners()
    }

    private fun addTask(taskId: Int, task: BaseServiceTask) {
        taskCacheLock.write {
            // append() is optimized for inserting keys that are larger than all existing keys
            taskCache.append(taskId, task)
        }
    }

    private fun getTask(taskId: Int): BaseServiceTask? {
        taskCacheLock.read {
            return taskCache[taskId]
        }
    }

    private fun removeTask(taskId: Int): BaseServiceTask? {
        taskCacheLock.write {
            val task = taskCache[taskId]
            taskCache.remove(taskId)
            return task
        }
    }

    fun enqueueTaskId(taskId: Int): Boolean {
        val task = getTask(taskId)
        if (task != null) {
            enqueueOperation(THREAD_POOL_DEFAULT, task)
            return true
        }
        return false
    }

    fun removeCachedTask(taskId: Int): Boolean {
        val task = removeTask(taskId)
        if (task != null) {
            task.removeAllListeners()
            return true
        }
        return false
    }

    /**
     * {@inheritDoc}
     */
    override fun onCreate() {
        super.onCreate()

        addThreadPool(THREAD_POOL_DEFAULT, THREAD_POOL_DEFAULT_THREADS)

        setForegroundNotification()
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(CHANNEL_ID,
                    getString(R.string.notification_channel_background_services_name),
                    NotificationManager.IMPORTANCE_MIN)
            channel.description = getString(R.string.notification_channel_background_services_desc)

            val nm = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
            nm.createNotificationChannel(channel)
        }
    }

    private fun setForegroundNotification() {
        val startupIntent = Intent(this, MainActivity::class.java)
        startupIntent.action = Intent.ACTION_MAIN
        startupIntent.addCategory(Intent.CATEGORY_LAUNCHER)

        val startupPendingIntent = PendingIntent.getActivity(
                this, 0, startupIntent, PendingIntent.FLAG_UPDATE_CURRENT)

        createNotificationChannel()

        val builder = NotificationCompat.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.ic_launcher)
                .setContentIntent(startupPendingIntent)
                .setContentTitle(getString(R.string.switcher_service_background_service))
                .setPriority(NotificationCompat.PRIORITY_MIN)
                .setCategory(NotificationCompat.CATEGORY_SERVICE)

        startForeground(NOTIFICATION_ID, builder.build())
    }

    // Switch ROM

    fun switchRom(romId: String, forceChecksumsUpdate: Boolean): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, SwitchRomTask(taskId, this, romId, forceChecksumsUpdate))
        return taskId
    }

    // Get ROMs state

    fun getRomsState(): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, GetRomsStateTask(taskId, this))
        return taskId
    }

    // Set kernel

    fun setKernel(romId: String): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, SetKernelTask(taskId, this, romId))
        return taskId
    }

    // Create ramdisk updater

    fun createRamdiskUpdater(romInfo: RomInformation): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, CreateRamdiskUpdaterTask(taskId, this, romInfo))
        return taskId
    }

    // Create launcher

    fun createLauncher(romInfo: RomInformation, reboot: Boolean): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, CreateLauncherTask(taskId, this, romInfo, reboot))
        return taskId
    }

    // Verify zip

    fun verifyZip(uri: Uri): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, VerifyZipTask(taskId, this, uri))
        return taskId
    }

    // Wipe ROM

    fun wipeRom(romId: String, targets: ShortArray): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, WipeRomTask(taskId, this, romId, targets))
        return taskId
    }

    // Cache wallpaper

    fun cacheWallpaper(info: RomInformation): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, CacheWallpaperTask(taskId, this, info))
        return taskId
    }

    // Get ROM details

    fun getRomDetails(romInfo: RomInformation): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, GetRomDetailsTask(taskId, this, romInfo))
        return taskId
    }

    // Update mbtool with root

    fun updateMbtoolWithRoot(): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, UpdateMbtoolWithRootTask(taskId, this))
        return taskId
    }

    // Do stuff with the boot UI

    fun bootUIAction(action: BootUIAction): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, BootUIActionTask(taskId, this, action))
        return taskId
    }

    // Perform mbtool command operations

    fun mbtoolActions(actions: Array<MbtoolAction>): Int {
        val taskId = newTaskId.getAndIncrement()
        addTask(taskId, MbtoolTask(taskId, this, actions))
        return taskId
    }

    companion object {
        private val TAG = SwitcherService::class.java.simpleName

        private const val THREAD_POOL_DEFAULT = "default"
        private const val THREAD_POOL_DEFAULT_THREADS = 2

        private const val NOTIFICATION_ID = 1

        private const val CHANNEL_ID = "background_service"

        private val newTaskId = AtomicInteger(0)
        private val taskCache = SparseArray<BaseServiceTask>()
        private val taskCacheLock = ReentrantReadWriteLock()
    }
}