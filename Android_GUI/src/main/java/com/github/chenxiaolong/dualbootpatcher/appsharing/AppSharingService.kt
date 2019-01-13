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

import android.app.IntentService
import android.content.Intent
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.widget.Toast
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomConfig
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection

class AppSharingService : IntentService(TAG) {
    private fun onPackageRemoved(pkg: String) {
        var info: RomInformation? = null

        try {
            MbtoolConnection(this).use {
                val iface = it.`interface`!!

                info = RomUtils.getCurrentRom(this, iface)
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to determine current ROM. App sharing status was NOT updated", e)
            return
        }

        if (info == null) {
            Log.e(TAG, "Failed to determine current ROM. App sharing status was NOT updated")
            return
        }

        // Unshare package if explicitly removed. This only exists to keep the config file clean.
        // Mbtool will not touch any app that's not listed in the package database.
        val config = RomConfig.getConfig(info!!.configPath!!)
        val sharedPkgs = config.indivAppSharingPackages
        if (sharedPkgs.containsKey(pkg)) {
            sharedPkgs.remove(pkg)
            config.indivAppSharingPackages = sharedPkgs
            config.apply()

            Handler(Looper.getMainLooper()).post {
                val message = getString(R.string.indiv_app_sharing_app_no_longer_shared)
                Toast.makeText(this@AppSharingService, String.format(message, pkg),
                        Toast.LENGTH_LONG).show()
            }
        }
    }

    override fun onHandleIntent(intent: Intent?) {
        val action = intent!!.getStringExtra(ACTION)

        if (ACTION_PACKAGE_REMOVED == action) {
            onPackageRemoved(intent.getStringExtra(EXTRA_PACKAGE))
        }
    }

    companion object {
        private val TAG = AppSharingService::class.java.simpleName

        const val ACTION = "action"
        const val ACTION_PACKAGE_REMOVED = "package_removed"

        const val EXTRA_PACKAGE = "package"
    }
}
