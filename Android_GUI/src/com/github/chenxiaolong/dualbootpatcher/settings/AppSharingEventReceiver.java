/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.util.Log;

public class AppSharingEventReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        Uri data = intent.getData();

        if (Intent.ACTION_BOOT_COMPLETED.equals(action)) {
            Intent serviceIntent = new Intent(context, AppSharingService.class);
            serviceIntent.putExtra(AppSharingService.ACTION,
                    AppSharingService.ACTION_SPAWN_DAEMON);
            context.startService(serviceIntent);
        } else if (Intent.ACTION_PACKAGE_ADDED.equals(action)) {
            Intent serviceIntent = new Intent(context, AppSharingService.class);
            serviceIntent.putExtra(AppSharingService.ACTION,
                    AppSharingService.ACTION_PACKAGE_ADDED);
            context.startService(serviceIntent);
        } else if (Intent.ACTION_PACKAGE_FULLY_REMOVED.equals(action)) {
            Intent serviceIntent = new Intent(context, AppSharingService.class);
            serviceIntent.putExtra(AppSharingService.ACTION,
                    AppSharingService.ACTION_PACKAGE_REMOVED);
            serviceIntent.putExtra(AppSharingService.EXTRA_PACKAGE, data.getSchemeSpecificPart());
            context.startService(serviceIntent);
        } else if (Intent.ACTION_PACKAGE_REPLACED.equals(action)) {
            Intent serviceIntent = new Intent(context, AppSharingService.class);
            serviceIntent.putExtra(AppSharingService.ACTION,
                    AppSharingService.ACTION_PACKAGE_UPGRADED);
            context.startService(serviceIntent);
        }
    }


}
