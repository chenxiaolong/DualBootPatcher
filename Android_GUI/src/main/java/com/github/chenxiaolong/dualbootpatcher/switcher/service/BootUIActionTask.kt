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
import android.os.Build
import android.util.Log
import com.github.chenxiaolong.dualbootpatcher.LogUtils
import com.github.chenxiaolong.dualbootpatcher.Version
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface
import mbtool.daemon.v3.FileOpenFlag
import mbtool.daemon.v3.PathDeleteFlag
import java.io.File
import java.io.IOException
import java.util.*
import java.util.zip.ZipFile

class BootUIActionTask(
        taskId: Int,
        context: Context,
        private val action: BootUIAction
) : BaseServiceTask(taskId, context) {
    private val stateLock = Any()
    private var finished: Boolean = false

    private var supported: Boolean = false
    private var version: Version? = null
    private var success: Boolean = false

    private class FileMapping internal constructor(
            internal var source: String,
            internal var target: String,
            internal var mode: Int
    )

    enum class BootUIAction {
        CHECK_SUPPORTED,
        GET_VERSION,
        INSTALL,
        UNINSTALL
    }

    interface BootUIActionTaskListener : BaseServiceTask.BaseServiceTaskListener, MbtoolErrorListener {
        fun onBootUICheckedSupported(taskId: Int, supported: Boolean)

        fun onBootUIHaveVersion(taskId: Int, version: Version?)

        fun onBootUIInstalled(taskId: Int, success: Boolean)

        fun onBootUIUninstalled(taskId: Int, success: Boolean)
    }

    /**
     * Get path to the cache partition's mount point
     *
     * @param iface Active mbtool interface
     * @return Either "/cache" or "/raw/cache"
     */
    @Throws(IOException::class, MbtoolException::class)
    private fun getCacheMountPoint(iface: MbtoolInterface): String {
        var mountPoint = CACHE_MOUNT_POINT

        if (pathExists(iface, RAW_CACHE_MOUNT_POINT)) {
            mountPoint = RAW_CACHE_MOUNT_POINT
        }

        return mountPoint
    }

    private fun checkSupported(): Boolean {
        val device = PatcherUtils.getCurrentDevice(context)
        return device != null && device.isTwSupported
    }

    /**
     * Get currently installed version of boot UI
     *
     * @param iface Mbtool interface
     * @return The [Version] installed or null if an error occurs or the version number is
     * invalid
     * @throws IOException
     * @throws MbtoolException
     */
    @Throws(IOException::class, MbtoolException::class)
    private fun getCurrentVersion(iface: MbtoolInterface): Version? {
        val mountPoint = getCacheMountPoint(iface)
        val zipPath = mountPoint + MAPPINGS[0].target

        val tempZip = File(context.cacheDir, "/bootui.zip")
        tempZip.delete()

        try {
            iface.pathCopy(zipPath, tempZip.absolutePath)
            iface.pathChmod(tempZip.absolutePath, 420 /* 0644 */)

            // Set SELinux label
            try {
                val label = iface.pathSelinuxGetLabel(tempZip.parent, false)
                iface.pathSelinuxSetLabel(tempZip.absolutePath, label, false)
            } catch (e: MbtoolCommandException) {
                Log.w(TAG, "$tempZip: Failed to set SELinux label", e)
            }
        } catch (e: MbtoolCommandException) {
            return null
        }

        return try {
            ZipFile(tempZip).use { zf ->
                var versionStr: String? = null
                var gitVersionStr: String? = null

                val entries = zf.entries()
                while (entries.hasMoreElements()) {
                    val ze = entries.nextElement()

                    if (ze.name == PROPERTIES_FILE) {
                        val prop = Properties()
                        prop.load(zf.getInputStream(ze))
                        versionStr = prop.getProperty(PROP_VERSION)
                        gitVersionStr = prop.getProperty(PROP_GIT_VERSION)
                        break
                    }
                }

                Log.d(TAG, "Boot UI version: $versionStr")
                Log.d(TAG, "Boot UI git version: $gitVersionStr")

                if (versionStr != null) {
                    Version.from(versionStr)
                } else {
                    null
                }
            }
        } catch (e: IOException) {
            Log.e(TAG, "Failed to read bootui.zip", e)
            null
        } finally {
            tempZip.delete()
        }
    }

    /**
     * Install or update boot UI
     *
     * @param iface Mbtool interface
     * @return Whether the installation was successful
     * @throws IOException
     * @throws MbtoolException
     */
    @Throws(IOException::class, MbtoolException::class)
    private fun install(iface: MbtoolInterface): Boolean {
        // Need to grab latest boot UI from the data archive
        PatcherUtils.initializePatcher(context)

        val device = PatcherUtils.getCurrentDevice(context)
        if (device == null) {
            Log.e(TAG, "Failed to determine current device")
            return false
        }

        // Uninstall first, so we don't get any leftover files
        Log.d(TAG, "Uninstalling before installing/updating")
        if (!uninstall(iface)) {
            return false
        }

        @Suppress("DEPRECATION")
        val abi = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            Build.SUPPORTED_ABIS[0]
        } else {
            Build.CPU_ABI
        }

        val sourceDir = PatcherUtils.getTargetDirectory(context)
        val mountPoint = getCacheMountPoint(iface)

        for (mapping in MAPPINGS) {
            val source = String.format(sourceDir.toString() + mapping.source, abi)
            val target = mountPoint + mapping.target
            val parent = File(target).parentFile

            try {
                iface.pathMkdir(parent.absolutePath, 493 /* 0755 */, true)
                iface.pathCopy(source, target)
                iface.pathChmod(target, mapping.mode)
            } catch (e: MbtoolCommandException) {
                Log.e(TAG, "Failed to install $source -> $target", e)
                return false
            }
        }

        return true
    }

    /**
     * Uninstall boot UI
     *
     * @param iface Mbtool interface
     * @return Whether the uninstallation was successful
     * @throws IOException
     * @throws MbtoolException
     */
    @Throws(IOException::class, MbtoolException::class)
    private fun uninstall(iface: MbtoolInterface): Boolean {
        val mountPoint = getCacheMountPoint(iface)

        MAPPINGS.map { mountPoint + it.target }
                .forEach {
                    // Ignore errors for now since mbtool doesn't expose the errno value for us to
                    // check for ENOENT
                    try {
                        iface.pathDelete(it, PathDeleteFlag.UNLINK)
                    } catch (e: MbtoolCommandException) {
                        // Ignore
                    }
                }

        return true
    }

    public override fun execute() {
        var success = false
        var supported = false
        var version: Version? = null

        try {
            MbtoolConnection(context).use { conn ->
                val iface = conn.`interface`!!

                synchronized(BootUIAction::class.java) {
                    when (action) {
                        BootUIActionTask.BootUIAction.CHECK_SUPPORTED -> supported = checkSupported()
                        BootUIActionTask.BootUIAction.GET_VERSION -> version = getCurrentVersion(iface)
                        BootUIActionTask.BootUIAction.INSTALL -> {
                            success = install(iface)
                            if (!success) {
                                uninstall(iface)
                            }
                        }
                        BootUIActionTask.BootUIAction.UNINSTALL -> success = uninstall(iface)
                    }
                }
            }
        } catch (e: IOException) {
            Log.e(TAG, "mbtool communication error", e)
        } catch (e: MbtoolException) {
            Log.e(TAG, "mbtool error", e)
            sendOnMbtoolError(e.reason)
        } finally {
            // Save log
            if (action != BootUIAction.GET_VERSION) {
                LogUtils.dump("boot-ui-action.log")
            }
        }

        synchronized(stateLock) {
            this.success = success
            this.supported = supported
            this.version = version
            sendResult()
            finished = true
        }
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        super.onListenerAdded(listener)

        synchronized(stateLock) {
            if (finished) {
                sendResult()
            }
        }
    }

    private fun sendResult() {
        when (action) {
            BootUIActionTask.BootUIAction.CHECK_SUPPORTED -> sendOnCheckedSupported()
            BootUIActionTask.BootUIAction.GET_VERSION -> sendOnHaveVersion()
            BootUIActionTask.BootUIAction.INSTALL -> sendOnInstalled()
            BootUIActionTask.BootUIAction.UNINSTALL -> sendOnUninstalled()
        }
    }

    private fun sendOnCheckedSupported() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as BootUIActionTaskListener).onBootUICheckedSupported(
                        taskId, supported)
            }
        })
    }

    private fun sendOnHaveVersion() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as BootUIActionTaskListener).onBootUIHaveVersion(
                        taskId, version)
            }
        })
    }

    private fun sendOnInstalled() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as BootUIActionTaskListener).onBootUIInstalled(
                        taskId, success)
            }
        })
    }

    private fun sendOnUninstalled() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as BootUIActionTaskListener).onBootUIUninstalled(
                        taskId, success)
            }
        })
    }

    private fun sendOnMbtoolError(reason: Reason) {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as BootUIActionTaskListener).onMbtoolConnectionFailed(
                        taskId, reason)
            }
        })
    }

    companion object {
        private val TAG = BootUIActionTask::class.java.simpleName

        private const val CACHE_MOUNT_POINT = "/cache"
        private const val RAW_CACHE_MOUNT_POINT = "/raw/cache"

        private val MAPPINGS = arrayOf(
                FileMapping("/bootui/%s/bootui.zip", "/multiboot/bootui.zip", 420 /* 0644 */),
                FileMapping("/bootui/%s/bootui.zip.sig", "/multiboot/bootui.zip.sig", 420 /* 0644 */))

        private const val PROPERTIES_FILE = "info.prop"
        private const val PROP_VERSION = "bootui.version"
        private const val PROP_GIT_VERSION = "bootui.git-version"

        @Throws(IOException::class, MbtoolException::class)
        private fun pathExists(iface: MbtoolInterface?, path: String): Boolean {
            var id = -1

            return try {
                id = iface!!.fileOpen(path, shortArrayOf(FileOpenFlag.RDONLY), 0)
                true
            } catch (e: MbtoolCommandException) {
                // Ignore
                false
            } finally {
                if (id >= 0) {
                    try {
                        iface!!.fileClose(id)
                    } catch (e: Exception) {
                        // Ignore
                    }
                }
            }
        }
    }
}
