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
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.KernelStatus
import java.io.File
import java.io.IOException

class GetRomsStateTask(taskId: Int, context: Context) : BaseServiceTask(taskId, context) {
    private val stateLock = Any()
    private var finished: Boolean = false

    private var roms: Array<RomInformation>? = null
    private var currentRom: RomInformation? = null
    private var activeRomId: String? = null
    private lateinit var kernelStatus: KernelStatus

    interface GetRomsStateTaskListener : BaseServiceTask.BaseServiceTaskListener,
            MbtoolErrorListener {
        fun onGotRomsState(taskId: Int, roms: Array<RomInformation>?, currentRom: RomInformation?,
                           activeRomId: String?, kernelStatus: KernelStatus)
    }

    public override fun execute() {
        var roms: Array<RomInformation>? = null
        var currentRom: RomInformation? = null
        var activeRomId: String? = null
        var kernelStatus = KernelStatus.UNKNOWN

        try {
            MbtoolConnection(context).use { conn ->
                val iface = conn.`interface`!!

                roms = RomUtils.getRoms(context, iface)
                currentRom = RomUtils.getCurrentRom(context, iface)

                val start = System.currentTimeMillis()

                val tmpImageFile = File(context.cacheDir, "boot.img")
                if (SwitcherUtils.copyBootPartition(context, iface, tmpImageFile)) {
                    try {
                        Log.d(TAG, "Getting active ROM ID")
                        activeRomId = SwitcherUtils.getBootImageRomId(tmpImageFile)
                        Log.d(TAG, "Comparing saved boot image to current boot image")
                        kernelStatus = SwitcherUtils.compareRomBootImage(currentRom, tmpImageFile)
                    } finally {
                        tmpImageFile.delete()
                    }
                }

                val end = System.currentTimeMillis()

                Log.d(TAG, "It took ${end - start} milliseconds to complete boot image checks")
                Log.d(TAG, "Current boot partition ROM ID: $activeRomId")
                Log.d(TAG, "Kernel status: ${kernelStatus.name}")
            }
        } catch (e: IOException) {
            Log.e(TAG, "mbtool communication error", e)
        } catch (e: MbtoolException) {
            Log.e(TAG, "mbtool error", e)
            sendOnMbtoolError(e.reason)
        } catch (e: MbtoolCommandException) {
            Log.w(TAG, "mbtool command error", e)
        }

        synchronized(stateLock) {
            this.roms = roms
            this.currentRom = currentRom
            this.activeRomId = activeRomId
            this.kernelStatus = kernelStatus
            sendOnGotRomsState()
            finished = true
        }
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        super.onListenerAdded(listener)

        synchronized(stateLock) {
            if (finished) {
                sendOnGotRomsState()
            }
        }
    }

    private fun sendOnGotRomsState() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as GetRomsStateTaskListener).onGotRomsState(
                        taskId, roms, currentRom, activeRomId, kernelStatus)
            }
        })
    }

    private fun sendOnMbtoolError(reason: Reason) {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as GetRomsStateTaskListener).onMbtoolConnectionFailed(taskId, reason)
            }
        })
    }

    companion object {
        private val TAG = GetRomsStateTask::class.java.simpleName
    }
}
