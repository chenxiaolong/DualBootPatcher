/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.content.Intent
import android.net.Uri
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device

object RickRollDevices {
    private val devNull = arrayOf("/dev/null")

    val youtubeIntent: Intent
        get() = Intent(Intent.ACTION_VIEW,
                Uri.parse("https://www.youtube.com/watch?v=dQw4w9WgXcQ"))

    val hero2qlte: Device by lazy(LazyThreadSafetyMode.PUBLICATION) {
        val d = Device()
        d.id = "hero2qlte"
        d.codenames = arrayOf("hero2qlte", "hero2qlteatt", "hero2qltespr", "hero2qltetmo",
                "hero2qltevzw")
        d.name = "Samsung Galaxy S7 Edge (Qcom)"
        d.architecture = "arm64-v8a"
        d.systemBlockDevs = devNull
        d.cacheBlockDevs = devNull
        d.dataBlockDevs = devNull
        d.bootBlockDevs = devNull
        d.extraBlockDevs = devNull
        d
    }

    fun addRickRollDevices(devices: MutableList<Device>): Int {
        var count = 0

        val iter = devices.listIterator()
        while (iter.hasNext()) {
            val device = iter.next()
            if (device.id == "hero2lte" || device.id == "herolte") {
                iter.add(hero2qlte)
                ++count
            }
        }

        return count
    }

    fun isRickRollDevice(device: Device): Boolean {
        return device === hero2qlte
    }
}