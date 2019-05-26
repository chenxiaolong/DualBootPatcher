/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

class UpdateMbtoolWithRootTask(taskId: Int, context: Context) : BaseServiceTask(taskId, context) {
    private val stateLock = Any()
    private var finished: Boolean = false

    private var success: Boolean = false

    interface UpdateMbtoolWithRootTaskListener : BaseServiceTask.BaseServiceTaskListener {
        fun onMbtoolUpdateSucceeded(taskId: Int)

        fun onMbtoolUpdateFailed(taskId: Int)
    }

    public override fun execute() {
        // If we've reached this point, then the SignedExec upgrade should have failed, but just in
        // case, we'll retry that method.

        var ret = MbtoolConnection.replaceViaSignedExec(context)
        if (ret) {
            Log.v(TAG, "Successfully updated mbtool with SignedExec method")
        } else {
            Log.w(TAG, "Failed to update mbtool with SignedExec method")

            ret = MbtoolConnection.replaceViaRoot(context)
            if (ret) {
                Log.v(TAG, "Successfully updated mbtool with root method")
            } else {
                Log.w(TAG, "Failed to update mbtool with root method")
            }
        }

        Log.d(TAG, "Attempting mbtool connection after update")

        if (ret) {
            try {
                MbtoolConnection(context).use { conn ->
                    val iface = conn.`interface`!!
                    Log.d(TAG, "mbtool was updated to ${iface.getVersion()}")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Failed to connect to mbtool", e)
                ret = false
            }
        }

        synchronized(stateLock) {
            success = ret
            sendReply()
            finished = true
        }
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        super.onListenerAdded(listener)

        synchronized(stateLock) {
            if (finished) {
                sendReply()
            }
        }
    }

    private fun sendReply() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                val l = listener as UpdateMbtoolWithRootTaskListener
                if (success) {
                    l.onMbtoolUpdateSucceeded(taskId)
                } else {
                    l.onMbtoolUpdateFailed(taskId)
                }
            }
        })
    }

    companion object {
        private val TAG = UpdateMbtoolWithRootTask::class.java.simpleName
    }
}