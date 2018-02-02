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
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface
import java.io.IOException

class GetRomDetailsTask(
        taskId: Int,
        context: Context,
        private val romInfo: RomInformation
) : BaseServiceTask(taskId, context) {
    private val stateLock = Any()
    private var finished: Boolean = false

    private var haveSystemSize: Boolean = false
    private var systemSizeSuccess: Boolean = false
    private var systemSize: Long = 0

    private var haveCacheSize: Boolean = false
    private var cacheSizeSuccess: Boolean = false
    private var cacheSize: Long = 0

    private var haveDataSize: Boolean = false
    private var dataSizeSuccess: Boolean = false
    private var dataSize: Long = 0

    private var havePackagesCounts: Boolean = false
    private var packagesCountsSuccess: Boolean = false
    private var systemPackages: Int = 0
    private var updatedPackages: Int = 0
    private var sserPackages: Int = 0

    interface GetRomDetailsTaskListener : BaseServiceTask.BaseServiceTaskListener, MbtoolErrorListener {
        fun onRomDetailsGotSystemSize(taskId: Int, romInfo: RomInformation,
                                      success: Boolean, size: Long)

        fun onRomDetailsGotCacheSize(taskId: Int, romInfo: RomInformation,
                                     success: Boolean, size: Long)

        fun onRomDetailsGotDataSize(taskId: Int, romInfo: RomInformation,
                                    success: Boolean, size: Long)

        fun onRomDetailsGotPackagesCounts(taskId: Int, romInfo: RomInformation, success: Boolean,
                                          systemPackages: Int, updatedPackages: Int,
                                          userPackages: Int)

        fun onRomDetailsFinished(taskId: Int, romInfo: RomInformation)
    }

    public override fun execute() {
        try {
            MbtoolConnection(context).use { conn ->
                val iface = conn.`interface`!!

                // Packages counts
                try {
                    getPackagesCounts(iface)
                } catch (e: MbtoolCommandException) {
                    Log.w(TAG, "mbtool command error", e)
                }

                // System size
                try {
                    getSystemSize(iface)
                } catch (e: MbtoolCommandException) {
                    Log.w(TAG, "mbtool command error", e)
                }

                // Cache size
                try {
                    getCacheSize(iface)
                } catch (e: MbtoolCommandException) {
                    Log.w(TAG, "mbtool command error", e)
                }

                // Data size
                try {
                    getDataSize(iface)
                } catch (e: MbtoolCommandException) {
                    Log.w(TAG, "mbtool command error", e)
                }
            }
        } catch (e: IOException) {
            Log.e(TAG, "mbtool communication error", e)
        } catch (e: MbtoolException) {
            Log.e(TAG, "mbtool error", e)
            sendOnMbtoolError(e.reason)
        }

        // Finished
        synchronized(stateLock) {
            sendOnRomDetailsFinished()
            finished = true
        }
    }

    @Throws(MbtoolException::class, IOException::class, MbtoolCommandException::class)
    private fun getPackagesCounts(iface: MbtoolInterface) {
        val pc = iface.getPackagesCounts(romInfo.id!!)
        val systemPackages = pc.systemPackages
        val updatedPackages = pc.systemUpdatePackages
        val userPackages = pc.nonSystemPackages

        synchronized(stateLock) {
            packagesCountsSuccess = true
            this.systemPackages = systemPackages
            this.updatedPackages = updatedPackages
            sserPackages = userPackages
            sendOnRomDetailsGotPackagesCounts()
            havePackagesCounts = true
        }
    }

    @Throws(MbtoolException::class, IOException::class, MbtoolCommandException::class)
    private fun getSystemSize(iface: MbtoolInterface) {
        val systemSize = iface.pathGetDirectorySize(
                romInfo.systemPath!!, arrayOf("multiboot"))
        val systemSizeSuccess = systemSize >= 0

        synchronized(stateLock) {
            this.systemSizeSuccess = systemSizeSuccess
            this.systemSize = systemSize
            sendOnRomDetailsGotSystemSize()
            haveSystemSize = true
        }
    }

    @Throws(MbtoolException::class, IOException::class, MbtoolCommandException::class)
    private fun getCacheSize(iface: MbtoolInterface) {
        val cacheSize = iface.pathGetDirectorySize(
                romInfo.cachePath!!, arrayOf("multiboot"))
        val cacheSizeSuccess = cacheSize >= 0

        synchronized(stateLock) {
            this.cacheSizeSuccess = cacheSizeSuccess
            this.cacheSize = cacheSize
            sendOnRomDetailsGotCacheSize()
            haveCacheSize = true
        }
    }

    @Throws(MbtoolException::class, IOException::class, MbtoolCommandException::class)
    private fun getDataSize(iface: MbtoolInterface) {
        val dataSize = iface.pathGetDirectorySize(
                romInfo.dataPath!!, arrayOf("multiboot", "media"))
        val dataSizeSuccess = dataSize >= 0

        synchronized(stateLock) {
            this.dataSizeSuccess = dataSizeSuccess
            this.dataSize = dataSize
            sendOnRomDetailsGotDataSize()
            haveDataSize = true
        }
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        super.onListenerAdded(listener)

        synchronized(stateLock) {
            if (havePackagesCounts) {
                sendOnRomDetailsGotPackagesCounts()
            }
            if (haveSystemSize) {
                sendOnRomDetailsGotSystemSize()
            }
            if (haveCacheSize) {
                sendOnRomDetailsGotCacheSize()
            }
            if (haveDataSize) {
                sendOnRomDetailsGotDataSize()
            }
            if (finished) {
                sendOnRomDetailsFinished()
            }
        }
    }

    private fun sendOnRomDetailsGotSystemSize() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as GetRomDetailsTaskListener).onRomDetailsGotSystemSize(
                        taskId, romInfo, systemSizeSuccess, systemSize)
            }
        })
    }

    private fun sendOnRomDetailsGotCacheSize() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as GetRomDetailsTaskListener).onRomDetailsGotCacheSize(
                        taskId, romInfo, cacheSizeSuccess, cacheSize)
            }
        })
    }

    private fun sendOnRomDetailsGotDataSize() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as GetRomDetailsTaskListener).onRomDetailsGotDataSize(
                        taskId, romInfo, dataSizeSuccess, dataSize)
            }
        })
    }

    private fun sendOnRomDetailsGotPackagesCounts() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as GetRomDetailsTaskListener).onRomDetailsGotPackagesCounts(
                        taskId, romInfo, packagesCountsSuccess, systemPackages,
                        updatedPackages, sserPackages)
            }
        })
    }

    private fun sendOnRomDetailsFinished() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as GetRomDetailsTaskListener).onRomDetailsFinished(taskId, romInfo)
            }
        })
    }

    private fun sendOnMbtoolError(reason: Reason) {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as GetRomDetailsTaskListener).onMbtoolConnectionFailed(taskId, reason)
            }
        })
    }

    companion object {
        private val TAG = GetRomDetailsTask::class.java.simpleName
    }
}