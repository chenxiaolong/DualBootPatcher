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

import android.util.Log
import com.google.gson.Gson
import com.google.gson.JsonParseException
import com.google.gson.annotations.SerializedName
import com.google.gson.stream.JsonReader
import com.google.gson.stream.JsonWriter
import java.io.File
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.io.FileReader
import java.io.OutputStreamWriter
import java.util.ArrayList
import java.util.HashMap
import kotlin.concurrent.thread

class RomConfig private constructor(private val filename: String) {
    var id: String? = null
    var name: String? = null
    var isIndivAppSharingEnabled: Boolean = false
    private var sharedPkgs = HashMap<String, SharedItems>()

    var indivAppSharingPackages: HashMap<String, SharedItems>
        get() {
            val result = HashMap<String, SharedItems>()
            for ((key, value) in sharedPkgs) {
                result[key] = SharedItems(value)
            }
            return result
        }
        set(pkgs) {
            sharedPkgs = HashMap()
            for ((key, value) in pkgs) {
                sharedPkgs[key] = SharedItems(value)
            }
        }

    class SharedItems {
        var sharedData: Boolean = false

        constructor(sharedData: Boolean) {
            this.sharedData = sharedData
        }

        constructor(si: SharedItems) {
            this.sharedData = si.sharedData
        }
    }

    @Synchronized
    @Throws(FileNotFoundException::class)
    fun commit() {
        saveFile()
    }

    fun apply() {
        thread(start = true) {
            try {
                commit()
            } catch (e: FileNotFoundException) {
                Log.e(TAG, "Asynchronous commit() failed", e)
            }
        }
    }

    private fun serialize(): RawRoot {
        val root = RawRoot()

        root.id = id
        root.name = name

        root.appSharing = RawAppSharing()
        root.appSharing!!.individual = isIndivAppSharingEnabled

        if (!sharedPkgs.isEmpty()) {
            val packages = ArrayList<RawPackage>()

            for ((key, value) in sharedPkgs) {
                val rp = RawPackage()
                rp.pkgId = key
                rp.shareData = value.sharedData
                packages.add(rp)
            }

            root.appSharing!!.packages = packages.toTypedArray()
        }

        return root
    }

    private fun deserialize(root: RawRoot?) {
        if (root == null) {
            return
        }

        id = root.id
        name = root.name

        if (root.appSharing != null) {
            isIndivAppSharingEnabled = root.appSharing!!.individual

            if (root.appSharing!!.packages != null) {
                root.appSharing!!.packages!!
                        .filter { it.pkgId != null }
                        .forEach { sharedPkgs[it.pkgId!!] = SharedItems(it.shareData) }
            }
        }
    }

    private class RawRoot {
        @SerializedName("id")
        internal var id: String? = null
        @SerializedName("name")
        internal var name: String? = null
        @SerializedName("app_sharing")
        internal var appSharing: RawAppSharing? = null
    }

    private class RawAppSharing {
        @SerializedName("individual")
        internal var individual: Boolean = false
        @SerializedName("packages")
        internal var packages: Array<RawPackage>? = null
    }

    private class RawPackage {
        @SerializedName("pkg_id")
        internal var pkgId: String? = null
        @SerializedName("share_data")
        internal var shareData: Boolean = false
    }

    @Throws(FileNotFoundException::class)
    private fun loadFile() {
        try {
            JsonReader(FileReader(filename)).use {
                val root = Gson().fromJson<RawRoot>(it, RawRoot::class.java)
                deserialize(root)
            }
        } catch (e: JsonParseException) {
            Log.w(TAG, "Failed to parse $filename", e)
        }
    }

    @Throws(FileNotFoundException::class)
    private fun saveFile() {
        val configFile = File(filename)
        configFile.parentFile.mkdirs()

        JsonWriter(OutputStreamWriter(FileOutputStream(configFile), Charsets.UTF_8)).use {
            it.setIndent("    ")
            val root = serialize()
            Gson().toJson(root, RawRoot::class.java, it)
        }
    }

    companion object {
        private val TAG = RomConfig::class.java.simpleName

        private val instances = HashMap<String, RomConfig>()

        fun getConfig(filename: String): RomConfig {
            val file = File(filename)

            // Return existing instance if it exists
            if (instances.containsKey(file.absolutePath)) {
                return instances[file.absolutePath]!!
            }

            val config = RomConfig(file.absolutePath)
            try {
                config.loadFile()
            } catch (e: FileNotFoundException) {
                Log.e(TAG, "Failed to load $file", e)
            }

            instances[file.absolutePath] = config

            return config
        }
    }
}
