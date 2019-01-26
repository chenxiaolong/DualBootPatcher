/*
 * Copyright (C) 2015-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.content.Intent
import android.content.ServiceConnection
import android.content.SharedPreferences
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.edit
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolErrorActivity
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SwitchRomResult
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmAutomatedSwitchRomDialog.ConfirmAutomatedSwitchRomDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask.SwitchRomTaskListener
import java.util.*

class AutomatedSwitcherActivity : AppCompatActivity(), ConfirmAutomatedSwitchRomDialogListener,
        ServiceConnection {
    private lateinit var prefs: SharedPreferences

    private var initialRun = false

    private var taskIdSwitchRom = -1

    /** Task IDs to remove  */
    private val taskIdsToRemove = ArrayList<Int>()

    /** Switcher service  */
    private var service: SwitcherService? = null
    /** Callback for events from the service  */
    private val callback = SwitcherEventCallback()

    /** Handler for processing events from the service on the UI thread  */
    private val handler = Handler(Looper.getMainLooper())

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.automated_switcher_layout)

        prefs = getSharedPreferences("settings", 0)

        if (!prefs.getBoolean(PREF_ALLOW_3RD_PARTY_INTENTS, false)) {
            Toast.makeText(this, R.string.third_party_intents_not_allowed, Toast.LENGTH_LONG).show()
            finish()
            return
        }

        val args = intent.extras
        if (args == null || intent.getStringExtra(EXTRA_ROM_ID) == null) {
            finish()
            return
        }

        if (savedInstanceState == null) {
            initialRun = true
        } else {
            taskIdSwitchRom = savedInstanceState.getInt(EXTRA_TASK_ID_SWITCH_ROM)
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putInt(EXTRA_TASK_ID_SWITCH_ROM, taskIdSwitchRom)
    }

    public override fun onStart() {
        super.onStart()

        // Start and bind to the service
        val intent = Intent(this, SwitcherService::class.java)
        bindService(intent, this, BIND_AUTO_CREATE)
        startService(intent)
    }

    public override fun onStop() {
        super.onStop()

        // If we connected to the service and registered the callback, now we unregister it
        if (service != null) {
            if (taskIdSwitchRom >= 0) {
                service!!.removeCallback(taskIdSwitchRom, callback)
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

        if (taskIdSwitchRom >= 0) {
            this.service!!.addCallback(taskIdSwitchRom, callback)
        }

        if (initialRun) {
            val shouldShow = prefs.getBoolean(PREF_SHOW_CONFIRM_DIALOG, true)
            if (shouldShow) {
                val d = ConfirmAutomatedSwitchRomDialog.newInstance(
                        intent.getStringExtra(EXTRA_ROM_ID))
                d.show(supportFragmentManager, CONFIRM_DIALOG_AUTOMATED)
            } else {
                switchRom()
            }
        }
    }

    override fun onServiceDisconnected(name: ComponentName) {
        service = null
    }

    public override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when (requestCode) {
            REQUEST_MBTOOL_ERROR -> if (data != null && resultCode == RESULT_OK) {
                finish()
            }
            else -> super.onActivityResult(requestCode, resultCode, data)
        }
    }

    private fun removeCachedTaskId(taskId: Int) {
        if (service != null) {
            service!!.removeCachedTask(taskId)
        } else {
            taskIdsToRemove.add(taskId)
        }
    }

    private fun onSwitchedRom(romId: String, result: SwitchRomResult) {
        removeCachedTaskId(taskIdSwitchRom)
        taskIdSwitchRom = -1

        val reboot = intent.getBooleanExtra(EXTRA_REBOOT, false)
        if (result === SwitchRomResult.SUCCEEDED && reboot) {
            // Don't return if we're rebooting
            SwitcherUtils.reboot(this)
            return
        }

        val d = supportFragmentManager.findFragmentByTag("automated_switch_rom_waiting")
                as GenericProgressDialog?
        d?.dismiss()

        val intent = Intent()

        when (result) {
            SwitchRomResult.SUCCEEDED -> {
                intent.putExtra(RESULT_CODE, "SWITCHING_SUCCEEDED")
                Toast.makeText(this, R.string.choose_rom_success, Toast.LENGTH_LONG).show()
            }
            SwitchRomResult.FAILED -> {
                intent.putExtra(RESULT_CODE, "SWITCHING_FAILED")
                intent.putExtra(RESULT_MESSAGE,
                        String.format("Failed to switch to %s", romId))
                Toast.makeText(this, R.string.choose_rom_failure, Toast.LENGTH_LONG).show()
            }
            SwitchRomResult.CHECKSUM_INVALID -> {
                intent.putExtra(RESULT_CODE, "SWITCHING_FAILED")
                intent.putExtra(RESULT_MESSAGE,
                        String.format("Mismatched checksums for %s's images", romId))
                Toast.makeText(this, R.string.choose_rom_checksums_invalid, Toast.LENGTH_LONG).show()
            }
            SwitchRomResult.CHECKSUM_NOT_FOUND -> {
                intent.putExtra(RESULT_CODE, "SWITCHING_FAILED")
                intent.putExtra(RESULT_MESSAGE,
                        String.format("Missing checksums for %s's images", romId))
                Toast.makeText(this, R.string.choose_rom_checksums_missing, Toast.LENGTH_LONG).show()
            }
            SwitchRomResult.UNKNOWN_BOOT_PARTITION -> {
                intent.putExtra(RESULT_CODE, "UNKNOWN_BOOT_PARTITION")
                intent.putExtra(RESULT_MESSAGE, "Failed to determine boot partition")
                val codename = RomUtils.getDeviceCodename(this)
                Toast.makeText(this, getString(R.string.unknown_boot_partition, codename),
                        Toast.LENGTH_SHORT).show()
            }
        }

        setResult(RESULT_OK, intent)
        finish()
    }

    override fun onConfirmSwitchRom(dontShowAgain: Boolean) {
        prefs.edit {
            putBoolean(PREF_SHOW_CONFIRM_DIALOG, !dontShowAgain)
        }

        switchRom()
    }

    override fun onCancelSwitchRom() {
        finish()
    }

    private fun switchRom() {
        val builder = GenericProgressDialog.Builder()
        builder.title(R.string.switching_rom)
        builder.message(R.string.please_wait)
        builder.build().show(supportFragmentManager, "automated_switch_rom_waiting")

        taskIdSwitchRom = service!!.switchRom(intent.getStringExtra(EXTRA_ROM_ID), false)
        service!!.addCallback(taskIdSwitchRom, callback)
        service!!.enqueueTaskId(taskIdSwitchRom)
    }

    private inner class SwitcherEventCallback : SwitchRomTaskListener {
        override fun onSwitchedRom(taskId: Int, romId: String, result: SwitchRomResult) {
            if (taskId == taskIdSwitchRom) {
                handler.post { this@AutomatedSwitcherActivity.onSwitchedRom(romId, result) }
            }
        }

        override fun onMbtoolConnectionFailed(taskId: Int, reason: Reason) {
            handler.post {
                val intent = Intent(this@AutomatedSwitcherActivity,
                        MbtoolErrorActivity::class.java)
                intent.putExtra(MbtoolErrorActivity.EXTRA_REASON, reason)
                startActivityForResult(intent, REQUEST_MBTOOL_ERROR)
            }
        }
    }

    companion object {
        private const val PREF_SHOW_CONFIRM_DIALOG = "show_confirm_automated_switcher_dialog"
        private const val PREF_ALLOW_3RD_PARTY_INTENTS = "allow_3rd_party_intents"

        private const val EXTRA_TASK_ID_SWITCH_ROM = "task_id_switch_rom"

        private val CONFIRM_DIALOG_AUTOMATED =
                "${AutomatedSwitcherActivity::class.java.name}.confirm.automated"

        const val EXTRA_ROM_ID = "rom_id"
        const val EXTRA_REBOOT = "reboot"

        const val RESULT_CODE = "code"
        const val RESULT_MESSAGE = "message"

        private const val REQUEST_MBTOOL_ERROR = 1234
    }
}