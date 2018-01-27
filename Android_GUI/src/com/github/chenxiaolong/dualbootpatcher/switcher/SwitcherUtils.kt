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

import android.content.Context
import android.net.Uri
import android.util.Log
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils
import com.github.chenxiaolong.dualbootpatcher.Version
import com.github.chenxiaolong.dualbootpatcher.Version.VersionParseException
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMiniZip.MiniZipInputFile
import com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff.LibMiscStuff
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface
import mbtool.daemon.v3.FileOpenFlag
import java.io.File
import java.io.FileNotFoundException
import java.io.IOException
import java.util.*
import kotlin.concurrent.thread

object SwitcherUtils {
    private val TAG = SwitcherUtils::class.java.simpleName

    private const val ZIP_MULTIBOOT_DIR = "multiboot/"
    private const val ZIP_INFO_PROP = "${ZIP_MULTIBOOT_DIR}info.prop"
    private const val PROP_INSTALLER_VERSION = "mbtool.installer.version"
    private const val PROP_INSTALL_LOCATION = "mbtool.installer.install-location"

    @Throws(IOException::class, MbtoolException::class)
    private fun pathExists(iface: MbtoolInterface, path: String): Boolean {
        var id = -1

        try {
            id = iface.fileOpen(path, shortArrayOf(FileOpenFlag.RDONLY), 0)
            return true
        } catch (e: MbtoolCommandException) {
            // Ignore
        } finally {
            if (id >= 0) {
                try {
                    iface.fileClose(id)
                } catch (e: Exception) {
                    // Ignore
                }

            }
        }

        return false
    }

    @Throws(IOException::class, MbtoolException::class)
    fun getBootPartition(context: Context, iface: MbtoolInterface): String? {
        val device = PatcherUtils.getCurrentDevice(context)
        return device?.bootBlockDevs!!.firstOrNull { pathExists(iface, it) }
    }

    fun getBlockDevSearchDirs(context: Context): Array<String>? {
        val device = PatcherUtils.getCurrentDevice(context)
        return device?.blockDevBaseDirs
    }

    fun verifyZipMbtoolVersion(context: Context, uri: Uri): VerificationResult {
        ThreadUtils.enforceExecutionOnNonMainThread()

        try {
            val pfd = context.contentResolver.openFileDescriptor(uri, "r")
                    ?: return VerificationResult.ERROR_ZIP_READ_FAIL
            pfd.use { ps ->
                MiniZipInputFile("/proc/self/fd/${ps.fd}").use { mzif ->
                    var isMultiboot = false
                    var prop: Properties? = null

                    loop@ while (true) {
                        val entry = mzif.nextEntry() ?: break
                        if (entry.name!!.startsWith(ZIP_MULTIBOOT_DIR)) {
                            isMultiboot = true
                        }
                        if (entry.name == ZIP_INFO_PROP) {
                            val p = Properties()
                            p.load(mzif.inputStream)
                            prop = p
                            break@loop
                        }
                    }

                    if (!isMultiboot) {
                        return VerificationResult.ERROR_NOT_MULTIBOOT
                    }

                    if (prop == null) {
                        return VerificationResult.ERROR_VERSION_TOO_OLD
                    }

                    val version = Version(prop.getProperty(PROP_INSTALLER_VERSION, "0.0.0"))
                    val minVersion = MbtoolUtils.getMinimumRequiredVersion(Feature.IN_APP_INSTALLATION)

                    return if (version < minVersion) VerificationResult.ERROR_VERSION_TOO_OLD
                    else VerificationResult.NO_ERROR
                }
            }
        } catch (e: FileNotFoundException) {
            Log.w(TAG, "Failed to verify zip mbtool version", e)
            return VerificationResult.ERROR_ZIP_NOT_FOUND
        } catch (e: IOException) {
            Log.w(TAG, "Failed to verify zip mbtool version", e)
            return VerificationResult.ERROR_ZIP_READ_FAIL
        } catch (e: VersionParseException) {
            Log.w(TAG, "Failed to verify zip mbtool version", e)
            return VerificationResult.ERROR_VERSION_TOO_OLD
        }
    }

    fun getTargetInstallLocation(context: Context, uri: Uri): String? {
        ThreadUtils.enforceExecutionOnNonMainThread()

        try {
            val pfd = context.contentResolver.openFileDescriptor(uri, "r") ?: return null
            pfd.use { ps ->
                MiniZipInputFile("/proc/self/fd/${ps.fd}").use { mzif ->
                    var prop: Properties? = null

                    loop@ while (true) {
                        val entry = mzif.nextEntry()
                        when {
                            entry == null -> break@loop
                            entry.name == ZIP_INFO_PROP -> {
                                prop = Properties()
                                prop.load(mzif.inputStream)
                                break@loop
                            }
                        }
                    }

                    return prop?.getProperty(PROP_INSTALL_LOCATION, null)
                }
            }
        } catch (e: FileNotFoundException) {
            Log.w(TAG, "Failed to get target install location", e)
            return null
        } catch (e: IOException) {
            Log.w(TAG, "Failed to get target install location", e)
            return null
        }
    }

    /**
     * Get ROM ID from a boot image file
     *
     * This will first attempt to read /romid from the ramdisk (new-style). If /romid does not
     * exist, the "romid=..." parameter in the kernel command line will be read (old-style). If
     * neither are present, the boot image is assumed to be associated with an unpatched primary ROM
     * and thus "primary" will be returned.
     *
     * @param file Boot image file
     * @return String containing the ROM ID or null if an error occurs.
     */
    fun getBootImageRomId(file: File): String? {
        return try {
            LibMiscStuff.getBootImageRomId(file.absolutePath) ?: "primary"
        } catch (e: IOException) {
            Log.e(TAG, "Failed to get ROM ID from $file", e)
            null
        }
    }

    enum class VerificationResult {
        NO_ERROR,
        ERROR_ZIP_NOT_FOUND,
        ERROR_ZIP_READ_FAIL,
        ERROR_NOT_MULTIBOOT,
        ERROR_VERSION_TOO_OLD
    }

    fun reboot(context: Context) {
        val prefs = context.getSharedPreferences("settings", 0)
        val confirm = prefs.getBoolean("confirm_reboot", false)

        thread(start = true) {
            try {
                MbtoolConnection(context).use { conn ->
                    val iface = conn.`interface`!!
                    iface.rebootViaFramework(confirm)
                }
            } catch (e: Exception) {
                Log.e(TAG, "Failed to reboot via framework", e)
            }
        }
    }

    enum class KernelStatus {
        UNSET,
        DIFFERENT,
        SET,
        UNKNOWN
    }

    fun compareRomBootImage(rom: RomInformation?, bootImageFile: File): KernelStatus {
        if (rom == null) {
            Log.w(TAG, "Could not get boot image status due to null RomInformation")
            return KernelStatus.UNKNOWN
        }

        val savedImageFile = File(RomUtils.getBootImagePath(rom.id!!))
        if (!savedImageFile.isFile) {
            Log.d(TAG, "Boot image is not set for ROM ID: ${rom.id}")
            return KernelStatus.UNSET
        }

        return try {
            if (LibMiscStuff.bootImagesEqual(savedImageFile.absolutePath,
                    bootImageFile.absolutePath)) {
                KernelStatus.SET
            } else {
                KernelStatus.DIFFERENT
            }
        } catch (e: IOException) {
            Log.e(TAG, "Failed to compare $savedImageFile with $bootImageFile", e)
            KernelStatus.UNKNOWN
        }
    }

    @Throws(IOException::class, MbtoolException::class)
    fun copyBootPartition(context: Context, iface: MbtoolInterface, targetFile: File): Boolean {
        val bootPartition = getBootPartition(context, iface)
        if (bootPartition == null) {
            Log.e(TAG, "Failed to determine boot partition")
            return false
        }

        try {
            iface.pathCopy(bootPartition, targetFile.absolutePath)
        } catch (e: MbtoolCommandException) {
            Log.e(TAG, "Failed to copy boot partition to $targetFile", e)
            return false
        }

        try {
            iface.pathChmod(targetFile.absolutePath, 420)
        } catch (e: MbtoolCommandException) {
            Log.e(TAG, "Failed to chmod $targetFile", e)
            return false
        }

        // Ensure SELinux label doesn't prevent reading from the file
        val parent = targetFile.parentFile
        var label: String? = null
        try {
            label = iface.pathSelinuxGetLabel(parent.absolutePath, false)
        } catch (e: MbtoolCommandException) {
            Log.w(TAG, "Failed to get SELinux label of $parent", e)
            // Ignore errors and hope for the best
        }

        if (label != null) {
            try {
                iface.pathSelinuxSetLabel(targetFile.absolutePath, label, false)
            } catch (e: MbtoolCommandException) {
                Log.w(TAG, "Failed to set SELinux label of $targetFile", e)
                // Ignore errors and hope for the best
            }
        }

        return true
    }
}