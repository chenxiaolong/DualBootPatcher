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

package com.github.chenxiaolong.dualbootpatcher.switcher.service

import android.content.Context
import android.util.Log
import com.github.chenxiaolong.dualbootpatcher.BuildConfig
import com.github.chenxiaolong.dualbootpatcher.LogUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.FileInfo
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.Patcher
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.PatcherConfig
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SetKernelResult
import java.io.File
import java.io.IOException

class CreateRamdiskUpdaterTask(
        taskId: Int,
        context: Context,
        private val romInfo: RomInformation
) : BaseServiceTask(taskId, context) {
    private val stateLock = Any()
    private var finished: Boolean = false

    private var path: String? = null

    interface CreateRamdiskUpdaterTaskListener : BaseServiceTask.BaseServiceTaskListener,
            MbtoolErrorListener {
        fun onCreatedRamdiskUpdater(taskId: Int, romInfo: RomInformation, path: String?)
    }

    /**
     * Create zip for updating the ramdisk
     *
     * @return Whether the ramdisk was successfully updated
     */
    private fun createZip(path: File): Boolean {
        var pc: PatcherConfig? = null
        var patcher: Patcher? = null
        var fi: FileInfo? = null

        try {
            fi = FileInfo()
            pc = PatcherUtils.newPatcherConfig(context)
            patcher = pc.createPatcher(LIBMBPATCHER_RAMDISK_UPDATER)
            if (patcher == null) {
                Log.e(TAG, "Bundled libmbpatcher does not support $LIBMBPATCHER_RAMDISK_UPDATER")
                return false
            }

            fi.outputPath = path.absolutePath

            val device = PatcherUtils.getCurrentDevice(context)
            val codename = RomUtils.getDeviceCodename(context)
            if (device == null) {
                Log.e(TAG, "Current device $codename does not appear to be supported")
                return false
            }
            fi.device = device

            patcher.setFileInfo(fi)

            if (!patcher.patchFile(null)) {
                logLibMbPatcherError(patcher.error)
                return false
            }

            return true
        } finally {
            if (patcher != null) {
                pc!!.destroyPatcher(patcher)
            }
            pc?.destroy()
            fi?.destroy()
        }
    }

    /**
     * Set the kernel if we're updating the ramdisk for the currently booted ROM
     *
     * @return True if the operation succeeded or was not needed
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    private fun setKernelIfNeeded(iface: MbtoolInterface): Boolean {
        val currentRom = RomUtils.getCurrentRom(context, iface)
        if (currentRom != null && currentRom.id == romInfo.id) {
            val result = iface.setKernel(context, romInfo.id!!)
            if (result !== SetKernelResult.SUCCEEDED) {
                Log.e(TAG, "Failed to reflash boot image")
                return false
            }
        }
        return true
    }

    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    private fun createRamdiskUpdater(iface: MbtoolInterface): String? {
        synchronized(CreateRamdiskUpdaterTask::class.java) {
            Log.d(TAG, "Creating ramdisk updater zip for ${romInfo.id} (version: "
                    + "${BuildConfig.VERSION_NAME})")

            try {
                PatcherUtils.initializePatcher(context)

                val bootImageFile = File(RomUtils.getBootImagePath(romInfo.id!!))

                // Make sure the boot image exists
                if (!bootImageFile.exists() && !setKernelIfNeeded(iface)) {
                    Log.e(TAG, "The kernel has not been backed up")
                    return null
                }

                // Back up existing boot image
                val bootImageBackupFile = File(bootImageFile.toString() + BOOT_IMAGE_BACKUP_SUFFIX)
                try {
                    bootImageFile.copyTo(bootImageBackupFile, overwrite = true)
                } catch (e: IOException) {
                    Log.w(TAG, "Failed to copy $bootImageFile to $bootImageBackupFile", e)
                }

                // Create ramdisk updater zip
                val zipFile = File(context.cacheDir, "ramdisk-updater.zip")

                // Run libmbpatcher's RamdiskUpdater on the boot image
                if (!createZip(zipFile)) {
                    Log.e(TAG, "Failed to create ramdisk updater zip")
                    return null
                }

                Log.v(TAG, "Successfully created ramdisk updater zip")

                return zipFile.absolutePath
            } finally {
                // Save whatever is in the logcat buffer to /sdcard/MultiBoot/ramdisk-update.log
                LogUtils.dump("ramdisk-update.log")
            }
        }
    }

    public override fun execute() {
        var path: String? = null

        try {
            MbtoolConnection(context).use { conn ->
                val iface = conn.`interface`!!
                path = createRamdiskUpdater(iface)
            }
        } catch (e: IOException) {
            Log.e(TAG, "mbtool communication error", e)
        } catch (e: MbtoolException) {
            Log.e(TAG, "mbtool error", e)
            sendOnMbtoolError(e.reason)
        } catch (e: MbtoolCommandException) {
            Log.w(TAG, "mbtool command error", e)
        }

        synchronized(stateLock) {
            this.path = path
            sendOnCreatedRamdiskUpdater()
            finished = true
        }
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        super.onListenerAdded(listener)

        synchronized(stateLock) {
            if (finished) {
                sendOnCreatedRamdiskUpdater()
            }
        }
    }

    private fun sendOnCreatedRamdiskUpdater() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as CreateRamdiskUpdaterTaskListener).onCreatedRamdiskUpdater(
                        taskId, romInfo, path)
            }
        })
    }

    private fun sendOnMbtoolError(reason: Reason) {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as CreateRamdiskUpdaterTaskListener).onMbtoolConnectionFailed(
                        taskId, reason)
            }
        })
    }

    companion object {
        private val TAG = CreateRamdiskUpdaterTask::class.java.simpleName

        /** [Patcher] ID for the libmbpatcher patcher  */
        private const val LIBMBPATCHER_RAMDISK_UPDATER = "RamdiskUpdater"
        /** Suffix for boot image backup  */
        private const val BOOT_IMAGE_BACKUP_SUFFIX = ".before-ramdisk-update.img"

        /**
         * Log the libmbpatcher error and destoy the PatcherError object
         *
         * @param error PatcherError
         */
        private fun logLibMbPatcherError(error: Int) {
            Log.e(TAG, "libmbpatcher error code: $error")
        }
    }
}