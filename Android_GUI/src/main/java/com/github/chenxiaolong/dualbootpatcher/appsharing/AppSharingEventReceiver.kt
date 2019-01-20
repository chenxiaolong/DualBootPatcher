/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent

class AppSharingEventReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        val action = intent.action
        val data = intent.data

        if (Intent.ACTION_PACKAGE_FULLY_REMOVED == action) {
            val serviceIntent = Intent(context, AppSharingService::class.java)
            serviceIntent.putExtra(AppSharingService.ACTION,
                    AppSharingService.ACTION_PACKAGE_REMOVED)
            serviceIntent.putExtra(AppSharingService.EXTRA_PACKAGE, data!!.schemeSpecificPart)
            context.startService(serviceIntent)
        }
    }
}