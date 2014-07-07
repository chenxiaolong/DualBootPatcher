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

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

public class AppSharingService extends IntentService {
    private static final String TAG = "AppSharingService";

    public static final String ACTION = "action";
    public static final String ACTION_SPAWN_DAEMON = "spawn_daemon";
    public static final String ACTION_PACKAGE_ADDED = "package_added";
    public static final String ACTION_PACKAGE_UPGRADED = "package_upgraded";
    public static final String ACTION_PACKAGE_REMOVED = "package_removed";

    public static final String EXTRA_PACKAGE = "package";

    public AppSharingService() {
        super(TAG);
    }

    private void spawnDaemon() {
        if (!AppSharingUtils.isSyncDaemonRunning(this)) {
            try {
                RunSyncDaemonDaemon task = new RunSyncDaemonDaemon();
                task.start();
                task.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private void onPackageAddedOrUpgraded() {
        if (ConfigFile.isExistsConfigFile() && !AppSharingUtils.isSyncDaemonRunning(this)) {
            try {
                RunSyncDaemonOnce task = new RunSyncDaemonOnce();
                task.start();
                task.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private void onPackageRemoved(String pkg) {
        try {
            PackageRemovedTask task = new PackageRemovedTask(pkg);
            task.start();
            task.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_SPAWN_DAEMON.equals(action)) {
            spawnDaemon();
        } else if (ACTION_PACKAGE_ADDED.equals(action)) {
            onPackageAddedOrUpgraded();
        } else if (ACTION_PACKAGE_UPGRADED.equals(action)) {
            onPackageAddedOrUpgraded();
        } else if (ACTION_PACKAGE_REMOVED.equals(action)) {
            onPackageRemoved(intent.getStringExtra(EXTRA_PACKAGE));
        }
    }

    private class RunSyncDaemonDaemon extends Thread {
        @Override
        public void run() {
            if (AppSharingUtils.isSyncDaemonRunning(AppSharingService.this)) {
                Log.v(TAG, "syncdaemon is already running. Will not respawn");
                return;
            }

            AppSharingUtils.runSyncDaemonDaemon(AppSharingService.this);
        }
    }

    private class RunSyncDaemonOnce extends Thread {
        @Override
        public void run() {
            if (ConfigFile.isExistsConfigFile()
                    && !AppSharingUtils.isSyncDaemonRunning(AppSharingService.this)) {
                AppSharingUtils.runSyncDaemonOnce(AppSharingService.this);
            }
        }
    }

    private class PackageRemovedTask extends Thread {
        private String mPackage;

        public PackageRemovedTask(String pkg) {
            mPackage = pkg;
        }

        @Override
        public void run() {
            ConfigFile config = new ConfigFile();
            RomInformation info = RomUtils.getCurrentRom();

            if (info == null) {
                Log.e(TAG, "Failed to determine current ROM. App sharing status was NOT updated");
                return;
            }

            // Unsync package if explicitly removed
            if (config.isRomSynced(mPackage, info)) {
                config.setRomSynced(mPackage, info, false);
            }

            config.save();

            if (ConfigFile.isExistsConfigFile()
                    && !AppSharingUtils.isSyncDaemonRunning(AppSharingService.this)) {
                AppSharingUtils.runSyncDaemonOnce(AppSharingService.this);
            }
        }
    }
}
