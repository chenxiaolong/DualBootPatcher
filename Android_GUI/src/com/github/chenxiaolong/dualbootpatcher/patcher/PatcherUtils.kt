/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.patcher

import android.content.Context
import android.os.Environment
import android.util.Log

import com.github.chenxiaolong.dualbootpatcher.BuildConfig
import com.github.chenxiaolong.dualbootpatcher.FileUtils
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.PatcherConfig
import com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff.LibMiscStuff

import org.apache.commons.io.Charsets

import java.io.File
import java.io.IOException
import java.util.ArrayList

object PatcherUtils {
    private val TAG = PatcherUtils::class.java.simpleName
    private val FILENAME = "data-%s.tar.xz"
    private val DIRNAME = "data-%s"

    private val PREFIX_DATA_SLOT = "data-slot-"
    private val PREFIX_EXTSD_SLOT = "extsd-slot-"

    val PATCHER_ID_ZIPPATCHER = "ZipPatcher"
    val PATCHER_ID_ODINPATCHER = "OdinPatcher"

    private var initialized: Boolean = false

    private var devices: Array<Device>? = null
    private var currentDevice: Device? = null

    private var targetFile: String
    private var targetDir: String

    private var installLocations: Array<InstallLocation>? = null

    init {
        val version = BuildConfig.VERSION_NAME.split("-")[0]
        targetFile = String.format(FILENAME, version)
        targetDir = String.format(DIRNAME, version)
    }

    private fun getTargetFile(context: Context): File {
        return File(context.cacheDir, targetFile)
    }

    fun getTargetDirectory(context: Context): File {
        return File(context.filesDir, targetDir)
    }

    @Synchronized
    fun initializePatcher(context: Context) {
        if (!initialized) {
            extractPatcher(context)
            initialized = true
        }
    }

    fun newPatcherConfig(context: Context): PatcherConfig {
        val pc = PatcherConfig()
        pc.dataDirectory = getTargetDirectory(context).absolutePath
        pc.tempDirectory = context.cacheDir.absolutePath
        return pc
    }

    @Synchronized
    fun getDevices(context: Context): Array<Device>? {
        ThreadUtils.enforceExecutionOnNonMainThread()
        initializePatcher(context)

        if (devices == null) {
            val path = File(getTargetDirectory(context), "devices.json")
            try {
                val json = org.apache.commons.io.FileUtils.readFileToString(path, Charsets.UTF_8)

                val validDevices = ArrayList<Device>()
                Device.newListFromJson(json)?.filterTo(validDevices) { it.validate() == 0L }

                if (!validDevices.isEmpty()) {
                    this.devices = validDevices.toTypedArray()
                }
            } catch (e: IOException) {
                Log.w(TAG, "Failed to read $path", e)
            }

        }

        return devices
    }

    @Synchronized
    fun getCurrentDevice(context: Context): Device? {
        ThreadUtils.enforceExecutionOnNonMainThread()

        if (currentDevice == null) {
            val realCodename = RomUtils.getDeviceCodename(context)
            val devices = getDevices(context)

            if (devices != null) {
                outer@ for (d in devices) {
                    for (codename in d.codenames!!) {
                        if (realCodename == codename) {
                            currentDevice = d
                            break@outer
                        }
                    }
                }
            }
        }

        return currentDevice
    }

    @Synchronized
    fun extractPatcher(context: Context) {
        context.cacheDir.listFiles()
                .filter {
                    it.name.startsWith("DualBootPatcherAndroid")
                            || it.name.startsWith("tmp")
                            || it.name.startsWith("data-")
                }
                .forEach { org.apache.commons.io.FileUtils.deleteQuietly(it) }
        context.filesDir.listFiles()
                .filter { it.isDirectory }
                .forEach {
                    it.listFiles()
                            .filter { it.name.contains("tmp") }
                            .forEach { org.apache.commons.io.FileUtils.deleteQuietly(it) }
                }

        val targetFile = getTargetFile(context)
        val targetDir = getTargetDirectory(context)

        if (BuildConfig.BUILD_TYPE == "debug" || !targetDir.exists()) {
            try {
                FileUtils.extractAsset(context, this.targetFile, targetFile)
            } catch (e: IOException) {
                throw IllegalStateException("Failed to extract data archive from assets", e)
            }

            // Remove all previous files
            context.filesDir.listFiles().forEach {
                org.apache.commons.io.FileUtils.deleteQuietly(it)
            }

            try {
                LibMiscStuff.extractArchive(targetFile.absolutePath, context.filesDir.absolutePath)
            } catch (e: IOException) {
                throw IllegalStateException("Failed to extract data archive", e)
            }

            // Delete archive
            targetFile.delete()
        }
    }

    class InstallLocation(
            val id: String,
            val name: String,
            val description: String
    )

    fun getInstallLocations(context: Context): Array<InstallLocation> {
        if (installLocations == null) {
            val locations = ArrayList<InstallLocation>()

            locations.add(InstallLocation("primary",
                    context.getString(R.string.install_location_primary_upgrade),
                    context.getString(R.string.install_location_primary_upgrade_desc)))

            locations.add(InstallLocation("dual",
                    context.getString(R.string.secondary),
                    String.format(context.getString(R.string.install_location_desc),
                            "/system/multiboot/dual")))

            (1..3).mapTo(locations) {
                InstallLocation("multi-slot-$it",
                        String.format(context.getString(R.string.multislot), it),
                        String.format(context.getString(R.string.install_location_desc),
                                "/cache/multiboot/multi-slot-$it"))
            }

            installLocations = locations.toTypedArray()
        }

        return installLocations!!
    }

    fun getNamedInstallLocations(context: Context): Array<InstallLocation> {
        ThreadUtils.enforceExecutionOnNonMainThread()

        Log.d(TAG, "Looking for named ROMs")

        val dir = File(Environment.getExternalStorageDirectory(), "MultiBoot")

        val locations = ArrayList<InstallLocation>()

        val files = dir.listFiles()
        if (files != null) {
            files.map { it.name }.forEach {
                if (it.startsWith("data-slot-") && it != "data-slot-") {
                    Log.d(TAG, "- Found data-slot: ${it.substring(10)}")
                    locations.add(getDataSlotInstallLocation(context, it.substring(10)))
                } else if (it.startsWith("extsd-slot-") && it != "extsd-slot-") {
                    Log.d(TAG, "- Found extsd-slot: ${it.substring(11)}")
                    locations.add(getExtsdSlotInstallLocation(context, it.substring(11)))
                }
            }
        } else {
            Log.e(TAG, "Failed to list files in: $dir")
        }

        return locations.toTypedArray()
    }

    fun getDataSlotInstallLocation(context: Context, dataSlotId: String): InstallLocation {
        return InstallLocation(getDataSlotRomId(dataSlotId),
                String.format(context.getString(R.string.dataslot), dataSlotId),
                context.getString(R.string.install_location_desc,
                        "/data/multiboot/data-slot-$dataSlotId"))
    }

    fun getExtsdSlotInstallLocation(context: Context, extsdSlotId: String): InstallLocation {
        return InstallLocation(getExtsdSlotRomId(extsdSlotId),
                String.format(context.getString(R.string.extsdslot), extsdSlotId),
                context.getString(R.string.install_location_desc,
                        "[External SD]/multiboot/extsd-slot-$extsdSlotId"))
    }

    fun getDataSlotRomId(dataSlotId: String): String {
        return PREFIX_DATA_SLOT + dataSlotId
    }

    fun getExtsdSlotRomId(extsdSlotId: String): String {
        return PREFIX_EXTSD_SLOT + extsdSlotId
    }

    fun isDataSlotRomId(romId: String): Boolean {
        return romId.startsWith(PREFIX_DATA_SLOT)
    }

    fun isExtsdSlotRomId(romId: String): Boolean {
        return romId.startsWith(PREFIX_EXTSD_SLOT)
    }

    fun getDataSlotIdFromRomId(romId: String): String? {
        return if (isDataSlotRomId(romId)) {
            romId.substring(PREFIX_DATA_SLOT.length)
        } else {
            null
        }
    }

    fun getExtsdSlotIdFromRomId(romId: String): String? {
        return if (isExtsdSlotRomId(romId)) {
            romId.substring(PREFIX_EXTSD_SLOT.length)
        } else {
            null
        }
    }
}
