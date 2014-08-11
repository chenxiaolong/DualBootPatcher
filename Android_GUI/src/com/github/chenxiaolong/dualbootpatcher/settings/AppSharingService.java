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

import android.app.IntentService;
import android.content.Intent;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.MiscUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

import java.util.Properties;

public class AppSharingService extends IntentService {
    private static final String TAG = AppSharingService.class.getSimpleName();
    public static final String BROADCAST_INTENT = "com.chenxiaolong.github.dualbootpatcher.BROADCAST_APP_SHARING_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_PACKAGE_REMOVED = "package_removed";
    public static final String ACTION_UPDATE_RAMDISK = "update_ramdisk";
    public static final String ACTION_GET_INFO = "get_info";

    public static final String STATE = "state";
    public static final String STATE_UPDATED_RAMDISK = "updated_ramdisk";
    public static final String STATE_GOT_INFO = "got_info";

    public static final String RESULT_FAILED = "failed";
    public static final String RESULT_ROMINFO = "rominfo";
    public static final String RESULT_VERSION = "version";

    public static final String EXTRA_PACKAGE = "package";

    public AppSharingService() {
        super(TAG);
    }

    private void onPackageRemoved(String pkg) {
        AppSharingConfigFile config = AppSharingConfigFile.getInstance();
        RomInformation info = RomUtils.getCurrentRom(AppSharingService.this);

        if (info == null) {
            Log.e(TAG, "Failed to determine current ROM. App sharing status was NOT updated");
            return;
        }

        // Unsync package if explicitly removed
        if (config.isRomSynced(pkg, info)) {
            config.setRomSynced(pkg, info, false);
        }

        config.save();
    }

    private void updateRamdisk() {
        boolean failed;

        try {
            AppSharingUtils.updateRamdisk(this);
            failed = false;
        } catch (Exception e) {
            e.printStackTrace();
            failed = true;
        }

        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATED_RAMDISK);
        i.putExtra(RESULT_FAILED, failed);
        sendBroadcast(i);
    }

    private void getSyncdaemonVersion() {
        String version = null;

        // If the daemon is not running, assume that it's not installed
        int pid = CommandUtils.getPid(this, "syncdaemon");

        if (pid > 0) {
            Properties prop = MiscUtils.getProperties("/data/local/tmp/syncdaemon.pid");

            if (prop.containsKey("version") && prop.containsKey("pid")) {
                String propPid = prop.getProperty("pid");
                String propVersion = prop.getProperty("version");

                Log.d(TAG, "syncdaemon pid: " + propPid);
                Log.d(TAG, "syncdaemon version: " + propVersion);

                int pid2 = -1;

                try {
                    pid2 = Integer.parseInt(propPid);
                } catch (NumberFormatException e) {
                    e.printStackTrace();
                }

                // Only check the version in the properties file if it corresponds to the currently
                // running syncdaemon
                if (pid == pid2) {
                    version = propVersion;
                }
            }
        }

        RomInformation curRom = RomUtils.getCurrentRom(this);

        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_GOT_INFO);
        i.putExtra(RESULT_ROMINFO, curRom);
        i.putExtra(RESULT_VERSION, version);
        sendBroadcast(i);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_PACKAGE_REMOVED.equals(action)) {
            onPackageRemoved(intent.getStringExtra(EXTRA_PACKAGE));
        } else if (ACTION_UPDATE_RAMDISK.equals(action)) {
            updateRamdisk();
        } else if (ACTION_GET_INFO.equals(action)) {
            getSyncdaemonVersion();
        }
    }
}
