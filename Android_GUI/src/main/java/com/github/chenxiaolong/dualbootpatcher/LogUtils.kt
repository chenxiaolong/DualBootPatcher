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

package com.github.chenxiaolong.dualbootpatcher

import android.os.Environment
import android.util.Log

import java.io.File
import java.io.IOException

object LogUtils {
    private val TAG = LogUtils::class.java.simpleName

    fun getPath(logFile: String): String {
        val fileName = File(logFile).name

        return Environment.getExternalStorageDirectory().toString() +
                File.separator + "MultiBoot" +
                File.separator + "logs" +
                File.separator + fileName
    }

    fun dump(logFile: String) {
        val path = File(getPath(logFile))
        path.parentFile.mkdirs()
        try {
            val command = arrayOf(
                    "logcat",
                    "-d",
                    "-f", path.toString(),
                    "-v", "threadtime",
                    "*:V"
            )
            Log.v(TAG, "Dumping logcat: ${command.contentToString()}")
            Runtime.getRuntime().exec(command).waitFor()
        } catch (e: InterruptedException) {
            Log.e(TAG, "Failed to wait for logcat to exit", e)
        } catch (e: IOException) {
            Log.e(TAG, "Failed to run logcat", e)
        }
    }
}
