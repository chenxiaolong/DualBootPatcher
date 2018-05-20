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
import java.io.File
import java.io.IOException
import java.util.ArrayList

object PatcherUtils {
    private val TAG = PatcherUtils::class.java.simpleName
    private const val FILENAME = "data-%s.tar.xz"
    private const val DIRNAME = "data-%s"

    const val PATCHER_ID_ZIPPATCHER = "ZipPatcher"
    const val PATCHER_ID_ODINPATCHER = "OdinPatcher"

    private val PRIMARY_SLOT = InstallLocation("primary",
            R.string.primary, emptyArray(),
            R.string.install_location_desc, arrayOf("/system"))

    private val SECONDARY_SLOT = InstallLocation("dual",
            R.string.secondary, emptyArray(),
            R.string.install_location_desc, arrayOf("/system/multiboot/dual"))

    private val MULTI_SLOT_TEMPLATE = TemplateLocation("multi-slot-",
            0, emptyArray(),
            R.string.multislot, arrayOf(TemplateLocation.PLACEHOLDER_SUFFIX),
            R.string.install_location_desc,
                    arrayOf("/cache/multiboot/${TemplateLocation.PLACEHOLDER_ID}"))

    private val DATA_SLOT_TEMPLATE = TemplateLocation("data-slot-",
            R.string.install_location_data_slot, emptyArray(),
            R.string.dataslot, arrayOf(TemplateLocation.PLACEHOLDER_SUFFIX),
            R.string.install_location_desc,
                    arrayOf("/data/multiboot/${TemplateLocation.PLACEHOLDER_ID}"))

    private val EXTSD_SLOT_TEMPLATE = TemplateLocation("extsd-slot-",
            R.string.install_location_extsd_slot, emptyArray(),
            R.string.extsdslot, arrayOf(TemplateLocation.PLACEHOLDER_SUFFIX),
            R.string.install_location_desc,
                    arrayOf("[External SD]/multiboot/${TemplateLocation.PLACEHOLDER_ID}"))

    private var initialized: Boolean = false

    private var devices: List<Device>? = null
    private var currentDevice: Device? = null

    private var targetFile: String
    private var targetDir: String

    var installLocations: Array<InstallLocation>
    var templateLocations: Array<TemplateLocation>

    init {
        val version = BuildConfig.VERSION_NAME.split("-")[0]
        targetFile = String.format(FILENAME, version)
        targetDir = String.format(DIRNAME, version)

        installLocations = arrayOf(
                PRIMARY_SLOT,
                SECONDARY_SLOT,
                MULTI_SLOT_TEMPLATE.toInstallLocation("1"),
                MULTI_SLOT_TEMPLATE.toInstallLocation("2"),
                MULTI_SLOT_TEMPLATE.toInstallLocation("3"))

        templateLocations = arrayOf(DATA_SLOT_TEMPLATE, EXTSD_SLOT_TEMPLATE)
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
    fun getDevices(context: Context): List<Device>? {
        ThreadUtils.enforceExecutionOnNonMainThread()
        initializePatcher(context)

        if (devices == null) {
            val path = File(getTargetDirectory(context), "devices.json")
            try {
                val json = path.readText()

                val validDevices = ArrayList<Device>()
                Device.newListFromJson(json)?.filterTo(validDevices) { it.validate() == 0L }

                if (!validDevices.isEmpty()) {
                    this.devices = validDevices
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
            val devices = getDevices(context)
            if (devices != null) {
                currentDevice = getCurrentDevice(context, devices)
            }
        }

        return currentDevice
    }

    fun getCurrentDevice(context: Context, devices: Collection<Device>): Device? {
        val realCodename = RomUtils.getDeviceCodename(context)

        for (d in devices) {
            for (codename in d.codenames!!) {
                if (realCodename == codename) {
                    return d
                }
            }
        }

        return null
    }

    @Synchronized
    fun extractPatcher(context: Context) {
        context.cacheDir.listFiles()
                .filter {
                    it.name.startsWith("DualBootPatcherAndroid")
                            || it.name.startsWith("tmp")
                            || it.name.startsWith("data-")
                }
                .forEach { it.deleteRecursively() }
        context.filesDir.listFiles()
                .filter { it.isDirectory }
                .forEach {
                    it.listFiles()
                            .filter { it.name.contains("tmp") }
                            .forEach { it.deleteRecursively() }
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
                it.deleteRecursively()
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

    fun getInstalledTemplateLocations(): Array<InstallLocation> {
        ThreadUtils.enforceExecutionOnNonMainThread()

        Log.d(TAG, "Looking for existing template install locations")

        val dir = File(Environment.getExternalStorageDirectory(), "MultiBoot")

        val locations = ArrayList<InstallLocation>()

        val files = dir.listFiles()
        if (files != null) {
            files.map { it.name }.forEach { filename ->
                for (template in templateLocations) {
                    if (filename.startsWith(template.prefix) && filename != template.prefix) {
                        val loc = template.toInstallLocation(filename.substring(template.prefix.length))
                        Log.d(TAG, "- Found ${loc.id}")
                        locations.add(loc)
                        break
                    }
                }
            }
        } else {
            Log.e(TAG, "Failed to list files in: $dir")
        }

        return locations.toTypedArray()
    }

    fun getInstallLocationFromRomId(romId: String): InstallLocation? {
        return installLocations.firstOrNull { it.id == romId }
                ?: templateLocations
                        .firstOrNull { romId.startsWith(it.prefix) }
                        ?.let { it.toInstallLocation(romId.substring(it.prefix.length)) }
    }
}
