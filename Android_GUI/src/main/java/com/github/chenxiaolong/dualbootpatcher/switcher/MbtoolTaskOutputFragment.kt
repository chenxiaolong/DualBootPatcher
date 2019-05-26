/*
 * Copyright (C) 2014-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.util.DisplayMetrics
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import com.github.chenxiaolong.dualbootpatcher.NullOutputStream
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolErrorActivity
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction
import com.github.chenxiaolong.dualbootpatcher.switcher.service.MbtoolTask.MbtoolTaskListener
import jackpal.androidterm.emulatorview.EmulatorView
import jackpal.androidterm.emulatorview.TermSession
import java.io.IOException
import java.io.PipedInputStream
import java.io.PipedOutputStream
import java.nio.charset.Charset
import java.util.*
import kotlin.concurrent.thread

class MbtoolTaskOutputFragment : Fragment(), ServiceConnection {
    private lateinit var actions: Array<MbtoolAction>

    private lateinit var emulatorView: EmulatorView
    private var session: TermSession? = null
    private lateinit var pos: PipedOutputStream

    var isRunning = true

    private var taskId = -1

    /** Task IDs to remove  */
    private val taskIdsToRemove = ArrayList<Int>()

    /** Switcher service  */
    private var service: SwitcherService? = null
    /** Callback for events from the service  */
    private val callback = SwitcherEventCallback()

    /** Handler for processing events from the service on the UI thread  */
    private val handler = Handler(Looper.getMainLooper())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (savedInstanceState != null) {
            taskId = savedInstanceState.getInt(EXTRA_TASK_ID)
        }
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_mbtool_tasks_output, container, false)
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        emulatorView = activity!!.findViewById(R.id.terminal)
        val metrics = DisplayMetrics()
        activity!!.windowManager.defaultDisplay.getMetrics(metrics)
        emulatorView.setDensity(metrics)

        if (savedInstanceState != null) {
            isRunning = savedInstanceState.getBoolean(EXTRA_IS_RUNNING)
        }

        val parcelables = arguments!!.getParcelableArray(PARAM_ACTIONS)
        actions = Arrays.copyOf(parcelables!!, parcelables.size, Array<MbtoolAction>::class.java)

        var titleResId = -1

        var countBackup = 0
        var countRestore = 0
        var countFlash = 0

        for (a in actions) {
            when (a.type) {
                MbtoolAction.Type.ROM_INSTALLER -> countFlash++
                MbtoolAction.Type.BACKUP_RESTORE -> when (a.backupRestoreParams!!.action) {
                    BackupRestoreParams.Action.BACKUP -> countBackup++
                    BackupRestoreParams.Action.RESTORE -> countRestore++
                }
            }
        }

        when {
            countBackup + countRestore + countFlash > 1 ->
                titleResId = R.string.in_app_flashing_title_flash_files
            countBackup > 0 ->
                titleResId = R.string.in_app_flashing_title_backup_rom
            countRestore > 0 ->
                titleResId = R.string.in_app_flashing_title_restore_rom
            countFlash > 0 ->
                titleResId = R.string.in_app_flashing_title_flash_file
        }

        if (titleResId >= 0) {
            activity!!.setTitle(titleResId)
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putBoolean(EXTRA_IS_RUNNING, isRunning)
        outState.putInt(EXTRA_TASK_ID, taskId)
    }

    override fun onStart() {
        super.onStart()

        // Create terminal
        session = TermSession()
        // We don't care about any input because this is kind of a "dumb" terminal output, not
        // a proper interactive one
        session!!.termOut = NullOutputStream()

        pos = PipedOutputStream()
        try {
            session!!.termIn = PipedInputStream(pos)
        } catch (e: IOException) {
            throw IllegalStateException("Failed to set terminal input stream to pipe", e)
        }

        emulatorView.attachSession(session)

        // Start and bind to the service
        val intent = Intent(activity, SwitcherService::class.java)
        activity!!.bindService(intent, this, Context.BIND_AUTO_CREATE)
        activity!!.startService(intent)
    }

    override fun onStop() {
        super.onStop()

        pos.use {}

        // Destroy session
        session!!.finish()
        session = null

        if (activity!!.isFinishing) {
            if (taskId >= 0) {
                removeCachedTaskId(taskId)
                taskId = -1
            }
        }

        // If we connected to the service and registered the callback, now we unregister it
        if (service != null) {
            if (taskId >= 0) {
                service!!.removeCallback(taskId, callback)
            }
        }

        // Unbind from our service
        activity!!.unbindService(this)
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

        if (taskId < 0) {
            taskId = this.service!!.mbtoolActions(actions)
            this.service!!.addCallback(taskId, callback)
            this.service!!.enqueueTaskId(taskId)
        } else {
            // FIXME: Hack so callback.onCommandOutput() is not called on the main thread, which
            //        would result in a deadlock
            thread(start = true,  name = "Add service callback for $this") {
                this@MbtoolTaskOutputFragment.service!!.addCallback(taskId, callback)
            }
        }
    }

    override fun onServiceDisconnected(name: ComponentName) {
        service = null
    }

    private fun removeCachedTaskId(taskId: Int) {
        if (service != null) {
            service!!.removeCachedTask(taskId)
        } else {
            taskIdsToRemove.add(taskId)
        }
    }

    private fun onFinishedTasks() {
        isRunning = false
    }

    private inner class SwitcherEventCallback : MbtoolTaskListener {
        override fun onMbtoolTasksCompleted(taskId: Int, actions: Array<MbtoolAction>,
                                            attempted: Int, succeeded: Int) {
            if (taskId == this@MbtoolTaskOutputFragment.taskId) {
                handler.post { onFinishedTasks() }
            }
        }

        override fun onCommandOutput(taskId: Int, line: String) {
            // This must *NOT* be called on the main thread or else it'll result in a dead lock
            ThreadUtils.enforceExecutionOnNonMainThread()

            if (taskId == this@MbtoolTaskOutputFragment.taskId) {
                try {
                    val crlf = line.replace("\n", "\r\n")
                    pos.write(crlf.toByteArray(Charset.forName("UTF-8")))
                    pos.flush()
                } catch (e: IOException) {
                    // Ignore. This will happen if the pipe is closed (eg. on configuration change)
                }
            }
        }

        override fun onMbtoolConnectionFailed(taskId: Int, reason: Reason) {
            handler.post {
                val intent = Intent(activity, MbtoolErrorActivity::class.java)
                intent.putExtra(MbtoolErrorActivity.EXTRA_REASON, reason)
                startActivity(intent)
            }
        }
    }

    companion object {
        val FRAGMENT_TAG: String = MbtoolTaskOutputFragment::class.java.name

        private val EXTRA_IS_RUNNING = "$FRAGMENT_TAG.is_running"
        private val EXTRA_TASK_ID = "$FRAGMENT_TAG.task_id"

        const val PARAM_ACTIONS = "actions"
    }
}
