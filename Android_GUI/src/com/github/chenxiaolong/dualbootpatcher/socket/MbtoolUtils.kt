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

package com.github.chenxiaolong.dualbootpatcher.socket

import android.content.Context
import android.util.Log

import com.github.chenxiaolong.dualbootpatcher.BuildConfig
import com.github.chenxiaolong.dualbootpatcher.SystemPropertiesProxy
import com.github.chenxiaolong.dualbootpatcher.Version

import java.util.HashMap

object MbtoolUtils {
    private val TAG = MbtoolUtils::class.java.simpleName

    private val minVersionMap = HashMap<Feature, Version>()

    init {
        @Suppress("ConstantConditionIf")
        if (BuildConfig.BUILD_TYPE == "ci") {
            // Snapshot builds
            minVersionMap[Feature.DAEMON] = Version("9.1.0.r54")
            minVersionMap[Feature.APP_SHARING] = Version("8.0.0.r2155")
            minVersionMap[Feature.IN_APP_INSTALLATION] = Version("9.3.0.r768")
        } else {
            // Debug/release builds
            minVersionMap[Feature.DAEMON] = Version("9.1.0")
            minVersionMap[Feature.APP_SHARING] = Version("8.99.13")
            minVersionMap[Feature.IN_APP_INSTALLATION] = Version("8.99.15")
        }
    }

    fun getSystemMbtoolVersion(context: Context): Version {
        try {
            return Version(SystemPropertiesProxy[context, "ro.multiboot.version"])
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get system mbtool version", e)
        }

        return Version("0.0.0")
    }

    fun getMinimumRequiredVersion(feature: Feature): Version {
        return minVersionMap[feature]!!
    }

    enum class Feature {
        DAEMON,
        APP_SHARING,
        IN_APP_INSTALLATION
    }
}
