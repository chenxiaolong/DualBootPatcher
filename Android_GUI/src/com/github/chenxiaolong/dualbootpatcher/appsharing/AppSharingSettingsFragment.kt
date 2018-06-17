/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.appsharing

import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.support.v4.app.LoaderManager
import android.support.v4.content.AsyncTaskLoader
import android.support.v4.content.Loader
import android.support.v7.preference.CheckBoxPreference
import android.support.v7.preference.Preference
import android.support.v7.preference.Preference.OnPreferenceChangeListener
import android.support.v7.preference.Preference.OnPreferenceClickListener
import android.support.v7.preference.PreferenceFragmentCompat
import android.widget.Toast
import com.github.chenxiaolong.dualbootpatcher.PermissionUtils
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomConfig
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppSharingSettingsFragment.NeededInfo
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog.GenericConfirmDialogListener
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog.GenericYesNoDialogListener
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature

class AppSharingSettingsFragment : PreferenceFragmentCompat(), OnPreferenceChangeListener,
        OnPreferenceClickListener, LoaderManager.LoaderCallbacks<NeededInfo>,
        GenericYesNoDialogListener, GenericConfirmDialogListener {
    private var config: RomConfig? = null

    private lateinit var shareIndivApps: CheckBoxPreference
    private lateinit var manageIndivApps: Preference

    private var loading = true

    override fun onCreatePreferences(savedInstanceState: Bundle?, rootKey: String?) {
        preferenceManager.sharedPreferencesName = "settings"

        addPreferencesFromResource(R.xml.app_sharing_settings)

        shareIndivApps = findPreference(KEY_SHARE_INDIV_APPS) as CheckBoxPreference
        shareIndivApps.onPreferenceChangeListener = this

        manageIndivApps = findPreference(KEY_MANAGE_INDIV_APPS)
        manageIndivApps.onPreferenceClickListener = this
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        updateState()

        if (PermissionUtils.supportsRuntimePermissions()) {
            if (savedInstanceState == null) {
                requestPermissions()
            } else if (PermissionUtils.hasPermissions(
                    activity!!, PermissionUtils.STORAGE_PERMISSIONS)) {
                onPermissionsGranted()
            }
        } else {
            startLoading()
        }
    }

    override fun onStop() {
        super.onStop()

        if (config == null) {
            // Destroyed before onLoadFinished ran
            return
        }

        // Write changes to config file
        config!!.apply()
    }

    private fun updateState() {
        val shareResId: Int
        val manageResId: Int

        if (loading) {
            shareResId = R.string.please_wait
            manageResId = R.string.please_wait
        } else {
            shareResId = R.string.a_s_settings_indiv_app_sharing_desc
            manageResId = R.string.a_s_settings_manage_shared_apps_desc
        }

        shareIndivApps.setSummary(shareResId)
        manageIndivApps.setSummary(manageResId)

        val enableShareIndiv = !loading
        var enableManageIndiv = !loading
        if (enableManageIndiv && config != null) {
            enableManageIndiv = config!!.isIndivAppSharingEnabled
        }
        var checkShareIndiv = false
        if (config != null) {
            checkShareIndiv = config!!.isIndivAppSharingEnabled
        }

        shareIndivApps.isEnabled = enableShareIndiv
        manageIndivApps.isEnabled = enableManageIndiv
        shareIndivApps.isChecked = checkShareIndiv
    }

    private fun startLoading() {
        loaderManager.initLoader(0, null, this)
    }

    private fun requestPermissions() {
        requestPermissions(PermissionUtils.STORAGE_PERMISSIONS, PERMISSIONS_REQUEST_STORAGE)
    }

    private fun showPermissionsRationaleDialogYesNo() {
        var dialog = fragmentManager!!.findFragmentByTag(YES_NO_DIALOG_PERMISSIONS) as GenericYesNoDialog?
        if (dialog == null) {
            val builder = GenericYesNoDialog.Builder()
            builder.message(R.string.a_s_settings_storage_permission_required)
            builder.positive(R.string.try_again)
            builder.negative(R.string.cancel)
            dialog = builder.buildFromFragment(YES_NO_DIALOG_PERMISSIONS, this)
            dialog.show(fragmentManager!!, YES_NO_DIALOG_PERMISSIONS)
        }
    }

    private fun showPermissionsRationaleDialogConfirm() {
        var dialog = fragmentManager!!.findFragmentByTag(CONFIRM_DIALOG_PERMISSIONS) as GenericConfirmDialog?
        if (dialog == null) {
            val builder = GenericConfirmDialog.Builder()
            builder.message(R.string.a_s_settings_storage_permission_required)
            builder.buttonText(R.string.ok)
            dialog = builder.buildFromFragment(CONFIRM_DIALOG_PERMISSIONS, this)
            dialog.show(fragmentManager!!, CONFIRM_DIALOG_PERMISSIONS)
        }
    }

    private fun onPermissionsGranted() {
        startLoading()
    }

    private fun onPermissionsDenied() {
        activity!!.finish()
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>,
                                            grantResults: IntArray) {
        if (requestCode == PERMISSIONS_REQUEST_STORAGE) {
            if (PermissionUtils.verifyPermissions(grantResults)) {
                onPermissionsGranted()
            } else {
                if (PermissionUtils.shouldShowRationales(this, permissions)) {
                    showPermissionsRationaleDialogYesNo()
                } else {
                    showPermissionsRationaleDialogConfirm()
                }
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        }
    }


    override fun onConfirmYesNo(tag: String, choice: Boolean) {
        if (YES_NO_DIALOG_PERMISSIONS == tag) {
            if (choice) {
                requestPermissions()
            } else {
                onPermissionsDenied()
            }
        }
    }

    override fun onConfirmOk(tag: String) {
        if (CONFIRM_DIALOG_PERMISSIONS == tag) {
            onPermissionsDenied()
        }
    }

    override fun onPreferenceChange(preference: Preference, objValue: Any): Boolean {
        val key = preference.key

        if (KEY_SHARE_INDIV_APPS == key) {
            config!!.isIndivAppSharingEnabled = objValue as Boolean
            updateState()
        }

        return true
    }

    override fun onPreferenceClick(preference: Preference): Boolean {
        val key = preference.key

        if (KEY_MANAGE_INDIV_APPS == key) {
            startActivity(Intent(activity, AppListActivity::class.java))
        }

        return true
    }

    override fun onCreateLoader(id: Int, args: Bundle?): Loader<NeededInfo> {
        return GetInfo(activity!!)
    }

    override fun onLoadFinished(loader: Loader<NeededInfo>, result: NeededInfo?) {
        if (result == null) {
            activity!!.finish()
            return
        }

        if (!result.haveRequiredVersion) {
            Toast.makeText(activity, R.string.a_s_settings_version_too_old,
                    Toast.LENGTH_LONG).show()
            activity!!.finish()
            return
        }

        config = result.config

        loading = false
        updateState()
    }

    override fun onLoaderReset(loader: Loader<NeededInfo>) {}

    class NeededInfo(
        internal val config: RomConfig,
        internal val haveRequiredVersion: Boolean
    )

    private class GetInfo(context: Context) : AsyncTaskLoader<NeededInfo>(context) {
        private var result: NeededInfo? = null

        init {
            onContentChanged()
        }

        override fun onStartLoading() {
            if (result != null) {
                deliverResult(result)
            } else if (takeContentChanged()) {
                forceLoad()
            }
        }

        override fun loadInBackground(): NeededInfo? {
            var currentRom: RomInformation? = null

            try {
                MbtoolConnection(context).use {
                    val iface = it.`interface`!!

                    currentRom = RomUtils.getCurrentRom(context, iface)
                }
            } catch (e: Exception) {
                return null
            }

            if (currentRom == null) {
                return null
            }

            val systemMbtoolVersion = MbtoolUtils.getSystemMbtoolVersion(context)

            return NeededInfo(RomConfig.getConfig(currentRom!!.configPath!!),
                    systemMbtoolVersion >=
                            MbtoolUtils.getMinimumRequiredVersion(Feature.APP_SHARING))
        }
    }

    companion object {
        private const val KEY_SHARE_INDIV_APPS = "share_indiv_apps"
        private const val KEY_MANAGE_INDIV_APPS = "manage_indiv_apps"

        private val YES_NO_DIALOG_PERMISSIONS =
                "${AppSharingSettingsFragment::class.java.canonicalName}.yes_no.permissions"
        private val CONFIRM_DIALOG_PERMISSIONS =
                "${AppSharingSettingsFragment::class.java.canonicalName}.confirm.permissions"

        /**
         * Request code for storage permissions request
         * (used in [.onRequestPermissionsResult])
         */
        private const val PERMISSIONS_REQUEST_STORAGE = 1
    }
}
