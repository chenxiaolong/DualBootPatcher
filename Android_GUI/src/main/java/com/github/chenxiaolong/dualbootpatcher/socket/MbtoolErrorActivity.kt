/*
 * Copyright (C) 2016-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.content.ComponentName
import android.content.Intent
import android.content.ServiceConnection
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.DialogFragment
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog.GenericConfirmDialogListener
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog.GenericYesNoDialogListener
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherService
import com.github.chenxiaolong.dualbootpatcher.switcher.service.UpdateMbtoolWithRootTask.UpdateMbtoolWithRootTaskListener
import java.util.*

class MbtoolErrorActivity : AppCompatActivity(), ServiceConnection, GenericYesNoDialogListener,
        GenericConfirmDialogListener {
    private var taskIdUpdateMbtool = -1

    /** Task IDs to remove  */
    private var taskIdsToRemove = ArrayList<Int>()

    /** Switcher service  */
    private var service: SwitcherService? = null
    /** Callback for events from the service  */
    private val callback = SwitcherEventCallback()

    /** Handler for processing events from the service on the UI thread  */
    private val handler = Handler(Looper.getMainLooper())

    /** mbtool connection failure reason  */
    private var reason: Reason? = null

    private var isInitialRun: Boolean = false

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_mbtool_error)

        val intent = intent
        reason = intent.getSerializableExtra(EXTRA_REASON) as Reason
        if (reason === Reason.PROTOCOL_ERROR) {
            // This is a relatively "standard" error. Not really one that the user needs to know
            // about...
            finish()
            return
        }

        isInitialRun = savedInstanceState == null

        if (savedInstanceState != null) {
            taskIdUpdateMbtool = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_UPDATE_MBTOOL)
            taskIdsToRemove = savedInstanceState.getIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE)!!
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putInt(EXTRA_STATE_TASK_ID_UPDATE_MBTOOL, taskIdUpdateMbtool)
        outState.putIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE, taskIdsToRemove)
    }

    override fun onStart() {
        super.onStart()

        // Start and bind to the service
        val intent = Intent(this, SwitcherService::class.java)
        bindService(intent, this, BIND_AUTO_CREATE)
        startService(intent)
    }

    override fun onStop() {
        super.onStop()

        // If we connected to the service and registered the callback, now we unregister it
        if (service != null) {
            if (taskIdUpdateMbtool >= 0) {
                service!!.removeCallback(taskIdUpdateMbtool, callback)
            }
        }

        // Unbind from our service
        unbindService(this)
        service = null

        // At this point, the callback will not get called anymore by the service. Now we just need
        // to remove all pending Runnables that were posted to handler.
        handler.removeCallbacksAndMessages(null)
    }

    override fun onServiceConnected(name: ComponentName, service: IBinder) {
        // Save a reference to the service so we can interact with it
        val binder = service as ThreadPoolServiceBinder
        this.service = binder.service as SwitcherService

        // Remove old task IDs
        for (taskId in taskIdsToRemove) {
            this.service!!.removeCachedTask(taskId)
        }
        taskIdsToRemove.clear()

        if (taskIdUpdateMbtool >= 0) {
            this.service!!.addCallback(taskIdUpdateMbtool, callback)
        }

        onReady()
    }

    override fun onServiceDisconnected(name: ComponentName) {
        service = null
    }

    override fun onBackPressed() {
        // Ignore
    }

    private fun removeCachedTaskId(taskId: Int) {
        if (service != null) {
            service!!.removeCachedTask(taskId)
        } else {
            taskIdsToRemove.add(taskId)
        }
    }

    private fun onReady() {
        if (!isInitialRun) {
            return
        }

        val dialog: DialogFragment

        when (reason) {
            MbtoolException.Reason.DAEMON_NOT_RUNNING -> {
                val builder = GenericProgressDialog.Builder()
                builder.message(R.string.mbtool_dialog_starting_mbtool)
                dialog = builder.build()
                startMbtoolUpdate()
            }
            MbtoolException.Reason.SIGNATURE_CHECK_FAIL -> {
                val builder = GenericYesNoDialog.Builder()
                builder.message(R.string.mbtool_dialog_signature_check_fail)
                builder.positive(R.string.proceed)
                builder.negative(R.string.cancel)
                dialog = builder.buildFromActivity(DIALOG_TAG)
            }
            MbtoolException.Reason.INTERFACE_NOT_SUPPORTED, MbtoolException.Reason.VERSION_TOO_OLD -> {
                val builder = GenericYesNoDialog.Builder()
                builder.message(R.string.mbtool_dialog_version_too_old)
                builder.positive(R.string.proceed)
                builder.negative(R.string.cancel)
                dialog = builder.buildFromActivity(DIALOG_TAG)
            }
            else -> throw IllegalStateException("Invalid mbtool error reason: ${reason!!.name}")
        }

        dialog.show(supportFragmentManager, DIALOG_TAG)
    }

    private fun onMbtoolUpdateFinished(success: Boolean) {
        removeCachedTaskId(taskIdUpdateMbtool)
        taskIdUpdateMbtool = -1

        // Dismiss progress dialog
        var dialog = supportFragmentManager.findFragmentByTag(DIALOG_TAG) as DialogFragment?
        dialog?.dismiss()

        if (!success) {
            val builder = GenericConfirmDialog.Builder()
            builder.message(R.string.mbtool_dialog_update_failed)
            builder.buttonText(R.string.ok)
            dialog = builder.buildFromActivity(DIALOG_TAG)
            dialog.show(supportFragmentManager, DIALOG_TAG)
        } else {
            exit(true)
        }
    }

    private fun exit(success: Boolean) {
        val intent = Intent()
        intent.putExtra(EXTRA_RESULT_CAN_RETRY, success)
        setResult(RESULT_OK, intent)

        finish()
    }

    override fun onConfirmYesNo(tag: String, choice: Boolean) {
        if (DIALOG_TAG == tag && choice) {
            val builder = GenericProgressDialog.Builder()
            builder.message(R.string.mbtool_dialog_updating_mbtool)
            builder.build().show(supportFragmentManager, DIALOG_TAG)

            startMbtoolUpdate()
        } else {
            finish()
        }
    }

    override fun onConfirmOk(tag: String) {
        if (DIALOG_TAG == tag) {
            exit(false)
        } else {
            finish()
        }
    }

    private fun startMbtoolUpdate() {
        if (taskIdUpdateMbtool < 0) {
            taskIdUpdateMbtool = service!!.updateMbtoolWithRoot()
            service!!.addCallback(taskIdUpdateMbtool, callback)
            service!!.enqueueTaskId(taskIdUpdateMbtool)
        }
    }

    private inner class SwitcherEventCallback : UpdateMbtoolWithRootTaskListener {
        override fun onMbtoolUpdateSucceeded(taskId: Int) {
            if (taskId == taskIdUpdateMbtool) {
                handler.post { this@MbtoolErrorActivity.onMbtoolUpdateFinished(true) }
            }
        }

        override fun onMbtoolUpdateFailed(taskId: Int) {
            if (taskId == taskIdUpdateMbtool) {
                handler.post { this@MbtoolErrorActivity.onMbtoolUpdateFinished(false) }
            }
        }
    }

    companion object {
        private val DIALOG_TAG = "${MbtoolErrorActivity::class.java.name}.dialog"

        // Argument extras
        const val EXTRA_REASON = "reason"
        // Saved state extras
        private const val EXTRA_STATE_TASK_ID_UPDATE_MBTOOL = "state.update_mbtool"
        private const val EXTRA_STATE_TASK_IDS_TO_REMOVE = "state.task_ids_to_remove"
        // Result extras
        const val EXTRA_RESULT_CAN_RETRY = "result.can_retry"
    }
}
