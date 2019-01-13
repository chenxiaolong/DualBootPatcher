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

package com.github.chenxiaolong.dualbootpatcher

import android.content.Context
import android.os.Build
import android.os.Environment
import android.os.Parcel
import android.os.Parcelable
import android.util.Log
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface
import com.squareup.picasso.Picasso
import mbtool.daemon.v3.FileOpenFlag
import java.io.File
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.io.IOException

object RomUtils {
    private val TAG = RomUtils::class.java.simpleName

    private const val UNKNOWN_ID = "unknown"

    class RomInformation : Parcelable {
        // Mount points
        var systemPath: String? = null
        var cachePath: String? = null
        var dataPath: String? = null

        // Identifiers
        var id: String? = null

        var version: String? = null
        var build: String? = null

        var thumbnailPath: String? = null
        var wallpaperPath: String? = null
        var configPath: String? = null

        var defaultName: String? = null
        private var _name: String? = null
        var imageResId: Int = 0

        var name: String?
            get() = _name ?: defaultName
            set(name) {
                _name = name
            }

        constructor()

        private constructor(p: Parcel) {
            systemPath = p.readString()
            cachePath = p.readString()
            dataPath = p.readString()
            id = p.readString()
            version = p.readString()
            build = p.readString()
            thumbnailPath = p.readString()
            wallpaperPath = p.readString()
            configPath = p.readString()
            defaultName = p.readString()
            _name = p.readString()
            imageResId = p.readInt()
        }

        override fun describeContents(): Int {
            return 0
        }

        override fun writeToParcel(dest: Parcel, flags: Int) {
            dest.writeString(systemPath)
            dest.writeString(cachePath)
            dest.writeString(dataPath)
            dest.writeString(id)
            dest.writeString(version)
            dest.writeString(build)
            dest.writeString(thumbnailPath)
            dest.writeString(wallpaperPath)
            dest.writeString(configPath)
            dest.writeString(defaultName)
            dest.writeString(_name)
            dest.writeInt(imageResId)
        }

        override fun toString(): String {
            return "id=$id" +
                    ", system=$systemPath" +
                    ", cache=$cachePath" +
                    ", data=$dataPath" +
                    ", version=$version" +
                    ", build=$build" +
                    ", thumbnailPath=$thumbnailPath" +
                    ", wallpaperPath=$wallpaperPath" +
                    ", configPath=$configPath" +
                    ", defaultName=$defaultName" +
                    ", name=$_name" +
                    ", imageResId=$imageResId"
        }

        companion object {
            @JvmField val CREATOR = object : Parcelable.Creator<RomInformation> {
                override fun createFromParcel(p: Parcel): RomInformation {
                    return RomInformation(p)
                }

                override fun newArray(size: Int): Array<RomInformation?> {
                    return arrayOfNulls(size)
                }
            }
        }
    }

    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun getCurrentRom(context: Context, iface: MbtoolInterface): RomInformation? {
        val id = iface.getBootedRomId()
        Log.d(TAG, "mbtool says current ROM ID is: $id")

        return getRoms(context, iface).firstOrNull { it.id == id }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun getRoms(context: Context, iface: MbtoolInterface): Array<RomInformation> {
        val roms = iface.getInstalledRoms()

        for (rom in roms) {
            rom.thumbnailPath = (Environment.getExternalStorageDirectory().toString()
                    + "/MultiBoot/${rom.id}/thumbnail.webp")
            rom.wallpaperPath = (Environment.getExternalStorageDirectory().toString()
                    + "/MultiBoot/${rom.id}/wallpaper.webp")
            rom.configPath = (Environment.getExternalStorageDirectory().toString()
                    + "/MultiBoot/${rom.id}/config.json")
            rom.imageResId = R.drawable.rom_android

            val loc = PatcherUtils.getInstallLocationFromRomId(rom.id!!)
            rom.defaultName = loc?.getDisplayName(context) ?: UNKNOWN_ID

            loadConfig(rom)
        }

        return roms
    }

    fun loadConfig(info: RomInformation) {
        val config = RomConfig.getConfig(info.configPath!!)
        info.name = config.name
    }

    fun saveConfig(info: RomInformation) {
        val config = RomConfig.getConfig(info.configPath!!)

        config.id = info.id
        config.name = info.name

        try {
            config.commit()
        } catch (e: FileNotFoundException) {
            Log.e(TAG, "Failed to save ROM config", e)
        }
    }

    fun getBootImagePath(romId: String): String {
        return Environment.getExternalStorageDirectory().toString() +
                File.separator + "MultiBoot" +
                File.separator + romId +
                File.separator + "boot.img"
    }

    fun getDeviceCodename(context: Context): String {
        return SystemPropertiesProxy[context, "ro.patcher.device", Build.DEVICE]
    }

    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    private fun usesLiveWallpaper(info: RomInformation, iface: MbtoolInterface): Boolean {
        val wallpaperInfoPath = "${info.dataPath}/system/users/0/wallpaper_info.xml"

        var id = -1

        try {
            id = iface.fileOpen(wallpaperInfoPath, shortArrayOf(FileOpenFlag.RDONLY), 0)

            val sb = iface.fileStat(id)

            // Check file size
            if (sb.st_size < 0 || sb.st_size > 1024) {
                return false
            }

            // Read file into memory
            val data = ByteArray(sb.st_size.toInt())
            var nWritten = 0
            while (nWritten < data.size) {
                val newData = iface.fileRead(id, 10240)

                val nRead = newData.limit() - newData.position()
                newData.get(data, nWritten, nRead)
                nWritten += nRead
            }

            iface.fileClose(id)
            id = -1

            val xml = String(data, Charsets.UTF_8)
            return xml.contains("component=")
        } finally {
            if (id >= 0) {
                try {
                    iface.fileClose(id)
                } catch (e: IOException) {
                    // Ignore
                }
            }
        }
    }

    enum class CacheWallpaperResult {
        UP_TO_DATE,
        UPDATED,
        FAILED,
        USES_LIVE_WALLPAPER
    }

    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun cacheWallpaper(info: RomInformation, iface: MbtoolInterface):
            CacheWallpaperResult {
        if (usesLiveWallpaper(info, iface)) {
            // We can't render a snapshot of a live wallpaper
            return CacheWallpaperResult.USES_LIVE_WALLPAPER
        }

        val wallpaperPath = "${info.dataPath}/system/users/0/wallpaper"
        val wallpaperCacheFile = File(info.wallpaperPath!!)

        var id = -1

        try {
            id = iface.fileOpen(wallpaperPath, shortArrayOf(), 0)

            // Check if we need to re-cache the file
            val sb = iface.fileStat(id)

            if (wallpaperCacheFile.exists()
                    && wallpaperCacheFile.lastModified() / 1000 > sb.st_mtime) {
                Log.d(TAG, "Wallpaper for ${info.id} has not been changed")
                return CacheWallpaperResult.UP_TO_DATE
            }

            // Ignore large wallpapers
            if (sb.st_size < 0 || sb.st_size > 20 * 1024 * 1024) {
                return CacheWallpaperResult.FAILED
            }

            // Read file into memory
            val data = ByteArray(sb.st_size.toInt())
            var nWritten = 0
            while (nWritten < data.size) {
                val newData = iface.fileRead(id, 10240)

                val nRead = newData.limit() - newData.position()
                newData.get(data, nWritten, nRead)
                nWritten += nRead
            }

            iface.fileClose(id)
            id = -1

            FileOutputStream(wallpaperCacheFile).use { fos ->
                // Compression can be very slow (more than 10 seconds) for a large wallpaper, so
                // we'll just cache the actual file instead
                fos.write(data)
            }

            // Load into bitmap
            //Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length)
            //if (bitmap == null) {
            //    return false
            //}
            //bitmap.compress(Bitmap.CompressFormat.WEBP, 100, fos)
            //bitmap.recycle()

            // Invalidate picasso cache
            Picasso.get().invalidate(wallpaperCacheFile)

            Log.d(TAG, "Wallpaper for ${info.id} has been cached")
            return CacheWallpaperResult.UPDATED
        } finally {
            if (id >= 0) {
                try {
                    iface.fileClose(id)
                } catch (e: IOException) {
                    // Ignore
                }
            }
        }
    }
}
