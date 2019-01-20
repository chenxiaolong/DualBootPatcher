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

package com.github.chenxiaolong.dualbootpatcher.switcher.service

import android.content.Context
import android.net.Uri
import android.util.Log

import com.github.chenxiaolong.dualbootpatcher.LogUtils
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult

class VerifyZipTask(
        taskId: Int,
        context: Context,
        private val uri: Uri
) : BaseServiceTask(taskId, context) {
    private val stateLock = Any()
    private var finished: Boolean = false

    private lateinit var result: VerificationResult
    private var romId: String? = null

    interface VerifyZipTaskListener : BaseServiceTask.BaseServiceTaskListener {
        fun onVerifiedZip(taskId: Int, uri: Uri, result: VerificationResult, romId: String?)
    }

    public override fun execute() {
        Log.d(TAG, "Verifying zip file: $uri")

        val result = SwitcherUtils.verifyZipMbtoolVersion(context, uri)
        val romId = SwitcherUtils.getTargetInstallLocation(context, uri)

        LogUtils.dump("verify-zip.log")

        synchronized(stateLock) {
            this.result = result
            this.romId = romId
            sendOnVerifiedZip()
            finished = true
        }
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        super.onListenerAdded(listener)

        synchronized(stateLock) {
            if (finished) {
                sendOnVerifiedZip()
            }
        }
    }

    private fun sendOnVerifiedZip() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as VerifyZipTaskListener).onVerifiedZip(
                        taskId, uri, result, romId)
            }
        })
    }

    companion object {
        private val TAG = VerifyZipTask::class.java.simpleName
    }
}