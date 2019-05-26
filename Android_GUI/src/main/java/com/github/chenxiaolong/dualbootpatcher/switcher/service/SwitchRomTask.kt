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
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SwitchRomResult
import java.io.IOException

class SwitchRomTask(
        taskId: Int,
        context: Context,
        private val romId: String,
        private val forceChecksumsUpdate: Boolean
) : BaseServiceTask(taskId, context) {
    private val stateLock = Any()
    private var finished: Boolean = false

    private lateinit var result: SwitchRomResult

    interface SwitchRomTaskListener : BaseServiceTask.BaseServiceTaskListener, MbtoolErrorListener {
        fun onSwitchedRom(taskId: Int, romId: String, result: SwitchRomResult)
    }

    public override fun execute() {
        Log.d(TAG, "Switching to $romId (force=$forceChecksumsUpdate)")

        var result = SwitchRomResult.FAILED

        try {
            MbtoolConnection(context).use { conn ->
                val iface = conn.`interface`!!
                result = iface.switchRom(context, romId, forceChecksumsUpdate)
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
            this.result = result
            sendOnSwitchedRom()
            finished = true
        }
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        super.onListenerAdded(listener)

        synchronized(stateLock) {
            if (finished) {
                sendOnSwitchedRom()
            }
        }
    }

    private fun sendOnSwitchedRom() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as SwitchRomTaskListener).onSwitchedRom(taskId, romId, result)
            }
        })
    }

    private fun sendOnMbtoolError(reason: Reason) {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as SwitchRomTaskListener).onMbtoolConnectionFailed(taskId, reason)
            }
        })
    }

    companion object {
        private val TAG = SwitchRomTask::class.java.simpleName
    }
}