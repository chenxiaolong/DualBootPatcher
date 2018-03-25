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

package com.github.chenxiaolong.dualbootpatcher.switcher.service

import android.app.ActivityManager
import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.switcher.AutomatedSwitcherActivity

import java.io.File

class CreateLauncherTask(
        taskId: Int,
        context: Context,
        private val romInfo: RomInformation,
        private val reboot: Boolean
) : BaseServiceTask(taskId, context) {
    private val stateLock = Any()
    private var finished: Boolean = false

    interface CreateLauncherTaskListener : BaseServiceTask.BaseServiceTaskListener {
        fun onCreatedLauncher(taskId: Int, romInfo: RomInformation)
    }

    public override fun execute() {
        Log.d(TAG, "Creating launcher for ${romInfo.id}")

        val shortcutIntent = Intent(context, AutomatedSwitcherActivity::class.java)
        shortcutIntent.action = "com.github.chenxiaolong.dualbootpatcher.SWITCH_ROM"
        shortcutIntent.putExtra(AutomatedSwitcherActivity.EXTRA_ROM_ID, romInfo.id)
        shortcutIntent.putExtra(AutomatedSwitcherActivity.EXTRA_REBOOT, reboot)

        val addIntent = Intent("com.android.launcher.action.INSTALL_SHORTCUT")
        addIntent.putExtra("duplicate", false)
        addIntent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent)
        addIntent.putExtra(Intent.EXTRA_SHORTCUT_NAME, romInfo.name)

        val file = File(romInfo.thumbnailPath!!)
        if (file.exists() && file.canRead()) {
            val options = BitmapFactory.Options()
            options.inPreferredConfig = Bitmap.Config.ARGB_8888
            val bitmap = BitmapFactory.decodeFile(file.absolutePath, options)
            addIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON, createScaledIcon(bitmap))
        } else {
            addIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE,
                    Intent.ShortcutIconResource.fromContext(
                            context, romInfo.imageResId))
        }

        context.sendBroadcast(addIntent)

        synchronized(stateLock) {
            sendOnCreatedLauncher()
            finished = true
        }
    }

    private fun createScaledIcon(icon: Bitmap): Bitmap {
        val am = context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
        val iconSize = am.launcherLargeIconSize
        return Bitmap.createScaledBitmap(icon, iconSize, iconSize, true)
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        super.onListenerAdded(listener)

        synchronized(stateLock) {
            if (finished) {
                sendOnCreatedLauncher()
            }
        }
    }

    private fun sendOnCreatedLauncher() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as CreateLauncherTaskListener).onCreatedLauncher(taskId, romInfo)
            }
        })
    }

    companion object {
        private val TAG = CreateLauncherTask::class.java.simpleName
    }
}