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

package com.github.chenxiaolong.dualbootpatcher.settings

import android.annotation.SuppressLint
import android.app.Activity
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.provider.DocumentsContract
import android.widget.Toast
import androidx.core.content.edit
import androidx.preference.Preference
import androidx.preference.Preference.OnPreferenceChangeListener
import androidx.preference.Preference.OnPreferenceClickListener
import androidx.preference.PreferenceFragmentCompat
import com.github.chenxiaolong.dualbootpatcher.BuildConfig
import com.github.chenxiaolong.dualbootpatcher.Constants
import com.github.chenxiaolong.dualbootpatcher.FileUtils
import com.github.chenxiaolong.dualbootpatcher.MainApplication
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder
import com.github.chenxiaolong.dualbootpatcher.Version
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherService
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolErrorActivity
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherService
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BootUIActionTask.BootUIAction
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BootUIActionTask.BootUIActionTaskListener
import java.util.*

class RomSettingsFragment : PreferenceFragmentCompat(), OnPreferenceChangeListener,
        ServiceConnection, OnPreferenceClickListener {
    private lateinit var backupDirectoryPref: Preference
    private lateinit var bootUIInstallPref: Preference
    private lateinit var bootUIUninstallPref: Preference
    private lateinit var parallelPatchingPref: Preference
    private lateinit var useDarkThemePref: Preference

    private var taskIdCheckSupported = -1
    private var taskIdGetVersion = -1
    private var taskIdInstall = -1
    private var taskIdUninstall = -1

    /** Task IDs to remove  */
    private val taskIdsToRemove = ArrayList<Int>()

    /** Switcher service  */
    private var service: SwitcherService? = null
    /** Callback for events from the service  */
    private val callback = ServiceEventCallback()

    /** Handler for processing events from the service on the UI thread  */
    private val handler = Handler(Looper.getMainLooper())

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        preferenceManager.sharedPreferencesName = "settings"

        addPreferencesFromResource(R.xml.rom_settings)

        val threads = preferenceManager.sharedPreferences.getInt(
                KEY_PARALLEL_PATCHING, PatcherService.DEFAULT_PATCHING_THREADS)

        backupDirectoryPref = findPreference(KEY_BACKUP_DIRECTORY)
        backupDirectoryPref.onPreferenceClickListener = this
        updateBackupDirectorySummary()

        bootUIInstallPref = findPreference(KEY_BOOT_UI_INSTALL)
        bootUIInstallPref.isEnabled = false
        bootUIInstallPref.setSummary(R.string.please_wait)
        bootUIInstallPref.onPreferenceClickListener = this

        bootUIUninstallPref = findPreference(KEY_BOOT_UI_UNINSTALL)
        bootUIUninstallPref.isEnabled = false
        bootUIUninstallPref.setSummary(R.string.please_wait)
        bootUIUninstallPref.onPreferenceClickListener = this

        parallelPatchingPref = findPreference(KEY_PARALLEL_PATCHING)
        parallelPatchingPref.setDefaultValue(Integer.toString(threads))
        parallelPatchingPref.onPreferenceChangeListener = this
        updateParallelPatchingSummary(threads)

        useDarkThemePref = findPreference(KEY_USE_DARK_THEME)
        useDarkThemePref.onPreferenceChangeListener = this

        if (savedInstanceState != null) {
            taskIdCheckSupported = savedInstanceState.getInt(EXTRA_TASK_ID_CHECK_SUPPORTED)
            taskIdGetVersion = savedInstanceState.getInt(EXTRA_TASK_ID_GET_VERSION)
            taskIdInstall = savedInstanceState.getInt(EXTRA_TASK_ID_INSTALL)
            taskIdUninstall = savedInstanceState.getInt(EXTRA_TASK_ID_UNINSTALL)
        }
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putInt(EXTRA_TASK_ID_CHECK_SUPPORTED, taskIdCheckSupported)
        outState.putInt(EXTRA_TASK_ID_GET_VERSION, taskIdGetVersion)
        outState.putInt(EXTRA_TASK_ID_INSTALL, taskIdInstall)
        outState.putInt(EXTRA_TASK_ID_UNINSTALL, taskIdUninstall)
    }

    override fun onStart() {
        super.onStart()

        // Start and bind to the service
        val intent = Intent(activity, SwitcherService::class.java)
        activity!!.bindService(intent, this, Context.BIND_AUTO_CREATE)
        activity!!.startService(intent)
    }

    override fun onStop() {
        super.onStop()

        if (activity!!.isFinishing) {
            if (taskIdCheckSupported >= 0) {
                removeCachedTaskId(taskIdCheckSupported)
                taskIdCheckSupported = -1
            }
            if (taskIdGetVersion >= 0) {
                removeCachedTaskId(taskIdGetVersion)
                taskIdGetVersion = -1
            }
        }

        // If we connected to the service and registered the callback, now we unregister it
        if (service != null) {
            if (taskIdCheckSupported >= 0) {
                service!!.removeCallback(taskIdCheckSupported, callback)
            }
            if (taskIdGetVersion >= 0) {
                service!!.removeCallback(taskIdGetVersion, callback)
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

        if (taskIdCheckSupported < 0) {
            checkSupported()
        } else {
            this.service!!.addCallback(taskIdCheckSupported, callback)
        }

        if (taskIdGetVersion >= 0) {
            this.service!!.addCallback(taskIdGetVersion, callback)
        }

        if (taskIdInstall >= 0) {
            this.service!!.addCallback(taskIdInstall, callback)
        }

        if (taskIdUninstall >= 0) {
            this.service!!.addCallback(taskIdUninstall, callback)
        }
    }

    override fun onServiceDisconnected(componentName: ComponentName) {
        service = null
    }

    private fun removeCachedTaskId(taskId: Int) {
        if (service != null) {
            service!!.removeCachedTask(taskId)
        } else {
            taskIdsToRemove.add(taskId)
        }
    }

    private fun checkSupported() {
        taskIdCheckSupported = service!!.bootUIAction(BootUIAction.CHECK_SUPPORTED)
        service!!.addCallback(taskIdCheckSupported, callback)
        service!!.enqueueTaskId(taskIdCheckSupported)
    }

    private fun onCheckedSupported(supported: Boolean) {
        removeCachedTaskId(taskIdCheckSupported)
        taskIdCheckSupported = -1

        if (supported) {
            if (taskIdGetVersion < 0) {
                taskIdGetVersion = service!!.bootUIAction(BootUIAction.GET_VERSION)
                service!!.addCallback(taskIdGetVersion, callback)
                service!!.enqueueTaskId(taskIdGetVersion)
            }
        } else {
            bootUIInstallPref.isEnabled = false
            bootUIInstallPref.setSummary(R.string.rom_settings_boot_ui_not_supported)
            bootUIUninstallPref.isEnabled = false
            bootUIUninstallPref.setSummary(R.string.rom_settings_boot_ui_not_supported)
        }
    }

    private fun onHaveVersion(version: Version?) {
        removeCachedTaskId(taskIdGetVersion)
        taskIdGetVersion = -1

        val installEnabled: Boolean
        val installTitle: String
        var installSummary: String? = null
        val uninstallEnabled: Boolean
        var uninstallSummary: String? = null

        bootUIInstallPref.summary = null
        bootUIUninstallPref.summary = null

        if (version == null) {
            installEnabled = true
            installTitle = getString(R.string.rom_settings_boot_ui_install_title)
            uninstallEnabled = false
            uninstallSummary = getString(R.string.rom_settings_boot_ui_not_installed)
        } else {
            val newest = Version.from(BuildConfig.VERSION_NAME) ?: throw IllegalStateException(
                    "App has invalid version number: ${BuildConfig.VERSION_NAME}")

            if (version < newest) {
                installSummary = getString(
                        R.string.rom_settings_boot_ui_update_available, newest.toString())
                installEnabled = true
            } else {
                installSummary = getString(R.string.rom_settings_boot_ui_up_to_date)
                installEnabled = false
            }

            installTitle = getString(R.string.rom_settings_boot_ui_update_title)

            uninstallEnabled = true
        }

        bootUIInstallPref.isEnabled = installEnabled
        bootUIInstallPref.title = installTitle
        bootUIInstallPref.summary = installSummary
        bootUIUninstallPref.isEnabled = uninstallEnabled
        bootUIUninstallPref.summary = uninstallSummary
    }

    private fun onInstalled(success: Boolean) {
        removeCachedTaskId(taskIdInstall)
        taskIdInstall = -1

        val d = fragmentManager!!.findFragmentByTag(PROGRESS_DIALOG_BOOT_UI) as GenericProgressDialog?
        d?.dismiss()

        checkSupported()

        Toast.makeText(activity,
                if (success)
                    R.string.rom_settings_boot_ui_install_success
                else
                    R.string.rom_settings_boot_ui_install_failure, Toast.LENGTH_LONG).show()

        if (success) {
            val builder = GenericConfirmDialog.Builder()
            builder.message(R.string.rom_settings_boot_ui_update_ramdisk_msg)
            builder.buttonText(R.string.ok)
            builder.build().show(fragmentManager!!, CONFIRM_DIALOG_BOOT_UI)
        }
    }

    private fun onUninstalled(success: Boolean) {
        removeCachedTaskId(taskIdUninstall)
        taskIdUninstall = -1

        val d = fragmentManager!!.findFragmentByTag(PROGRESS_DIALOG_BOOT_UI) as GenericProgressDialog?
        d?.dismiss()

        checkSupported()

        Toast.makeText(activity,
                if (success)
                    R.string.rom_settings_boot_ui_uninstall_success
                else
                    R.string.rom_settings_boot_ui_uninstall_failure, Toast.LENGTH_LONG).show()
    }

    override fun onPreferenceClick(preference: Preference): Boolean {
        when {
            preference === backupDirectoryPref -> {
                val intent = FileUtils.getFileTreeOpenIntent(activity!!)
                startActivityForResult(intent, REQUEST_BACKUP_DIRECTORY)
                return true
            }
            preference === bootUIInstallPref -> {
                taskIdInstall = service!!.bootUIAction(BootUIAction.INSTALL)
                service!!.addCallback(taskIdInstall, callback)
                service!!.enqueueTaskId(taskIdInstall)

                val builder = GenericProgressDialog.Builder()
                builder.message(R.string.please_wait)
                builder.build().show(fragmentManager!!, PROGRESS_DIALOG_BOOT_UI)
                return true
            }
            preference === bootUIUninstallPref -> {
                taskIdUninstall = service!!.bootUIAction(BootUIAction.UNINSTALL)
                service!!.addCallback(taskIdUninstall, callback)
                service!!.enqueueTaskId(taskIdUninstall)

                val builder = GenericProgressDialog.Builder()
                builder.message(R.string.please_wait)
                builder.build().show(fragmentManager!!, PROGRESS_DIALOG_BOOT_UI)
                return true
            }
            else -> return false
        }
    }

    override fun onPreferenceChange(preference: Preference, newValue: Any): Boolean {
        when {
            preference === parallelPatchingPref -> try {
                val threads = newValue.toString().toInt()
                if (threads >= 1) {
                    // Do this instead of using EditTextPreference's built-in persisting since we
                    // want it saved as an integer
                    preferenceManager.sharedPreferences.edit {
                        putInt(KEY_PARALLEL_PATCHING, threads)
                    }

                    updateParallelPatchingSummary(threads)
                    return true
                }
            } catch (e: NumberFormatException) {
            }
            preference === useDarkThemePref -> {
                // Apply dark theme and recreate activity
                MainApplication.useDarkTheme = newValue as Boolean
                activity!!.recreate()
                return true
            }
        }
        return false
    }

    override fun onActivityResult(request: Int, result: Int, data: Intent?) {
        when (request) {
            REQUEST_BACKUP_DIRECTORY -> if (data != null && result == Activity.RESULT_OK) {
                val uri = data.data

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT
                        && FileUtils.SAF_SCHEME == uri!!.scheme
                        && FileUtils.SAF_AUTHORITY == uri.scheme) {
                    val cr = activity!!.contentResolver
                    cr.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
                }

                val prefs = preferenceManager.sharedPreferences
                prefs.edit {
                    putString(Constants.Preferences.BACKUP_DIRECTORY_URI, uri!!.toString())
                }

                updateBackupDirectorySummary()
            }
            else -> super.onActivityResult(request, result, data)
        }
    }

    @SuppressLint("NewApi")
    private fun updateBackupDirectorySummary() {
        val prefs = preferenceManager.sharedPreferences
        val savedUri = prefs.getString(Constants.Preferences.BACKUP_DIRECTORY_URI,
                Constants.Defaults.BACKUP_DIRECTORY_URI)
        val uri = Uri.parse(savedUri)

        var name: String? = null

        if (FileUtils.FILE_SCHEME == uri.scheme) {
            name = uri.path
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP
                && FileUtils.SAF_SCHEME == uri.scheme
                && FileUtils.SAF_AUTHORITY == uri.authority) {
            val documentId = DocumentsContract.getTreeDocumentId(uri)
            val parts = documentId.split(":".toRegex()).dropLastWhile { it.isEmpty() }.toTypedArray()

            name = if (parts.size == 2) {
                "[${parts[0]}] /${parts[1]}"
            } else {
                documentId
            }
        }

        backupDirectoryPref.summary = name
    }

    private fun updateParallelPatchingSummary(threads: Int) {
        val summary = getString(R.string.rom_settings_parallel_patching_desc, threads)
        parallelPatchingPref.summary = summary
    }

    private inner class ServiceEventCallback : BootUIActionTaskListener {
        override fun onBootUICheckedSupported(taskId: Int, supported: Boolean) {
            if (taskId == taskIdCheckSupported) {
                handler.post { onCheckedSupported(supported) }
            }
        }

        override fun onBootUIHaveVersion(taskId: Int, version: Version?) {
            if (taskId == taskIdGetVersion) {
                handler.post { onHaveVersion(version) }
            }
        }

        override fun onBootUIInstalled(taskId: Int, success: Boolean) {
            if (taskId == taskIdInstall) {
                handler.post { onInstalled(success) }
            }
        }

        override fun onBootUIUninstalled(taskId: Int, success: Boolean) {
            if (taskId == taskIdUninstall) {
                handler.post { onUninstalled(success) }
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
        private const val PROGRESS_DIALOG_BOOT_UI = "boot_ui_progress_dialog"
        private const val CONFIRM_DIALOG_BOOT_UI = "boot_ui_confirm_dialog"

        private const val EXTRA_TASK_ID_CHECK_SUPPORTED = "task_id_check_supported"
        private const val EXTRA_TASK_ID_GET_VERSION = "task_id_get_version"
        private const val EXTRA_TASK_ID_INSTALL = "task_id_install"
        private const val EXTRA_TASK_ID_UNINSTALL = "task_id_uninstall"

        private const val KEY_BACKUP_DIRECTORY = "backup_directory"
        private const val KEY_BOOT_UI_INSTALL = "boot_ui_install"
        private const val KEY_BOOT_UI_UNINSTALL = "boot_ui_uninstall"
        private const val KEY_PARALLEL_PATCHING = "parallel_patching_threads"
        private const val KEY_USE_DARK_THEME = "use_dark_theme"

        private const val REQUEST_BACKUP_DIRECTORY = 1000
    }
}
