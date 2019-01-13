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
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import java.io.IOException

class WipeRomTask(
        taskId: Int,
        context: Context,
        private val romId: String,
        private val targets: ShortArray
) : BaseServiceTask(taskId, context) {
    private val stateLock = Any()
    private var finished: Boolean = false

    private lateinit var targetsSucceeded: ShortArray
    private lateinit var targetsFailed: ShortArray

    interface WipeRomTaskListener : BaseServiceTask.BaseServiceTaskListener, MbtoolErrorListener {
        fun onWipedRom(taskId: Int, romId: String, succeeded: ShortArray, failed: ShortArray)
    }

    public override fun execute() {
        try {
            MbtoolConnection(context).use { conn ->
                val iface = conn.`interface`!!

                val result = iface.wipeRom(romId, targets)
                synchronized(stateLock) {
                    targetsSucceeded = result.succeeded
                    targetsFailed = result.failed
                }
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
            sendOnWipedRom()
            finished = true
        }
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        super.onListenerAdded(listener)

        // Resend result if completed
        synchronized(stateLock) {
            if (finished) {
                sendOnWipedRom()
            }
        }
    }

    private fun sendOnWipedRom() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as WipeRomTaskListener).onWipedRom(
                        taskId, romId, targetsSucceeded, targetsFailed)
            }
        })
    }

    private fun sendOnMbtoolError(reason: Reason) {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as WipeRomTaskListener).onMbtoolConnectionFailed(taskId, reason)
            }
        })
    }

    companion object {
        private val TAG = WipeRomTask::class.java.simpleName
    }
}