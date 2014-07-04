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

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.util.Log;

import java.io.File;
import java.util.ArrayList;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandRunner;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

public class AppSharingUtils {
    public static final String TAG = "AppSharingUtils";

    // If we're running without a daemon, then we'll fire up syncdaemon when a package is
    // installed, upgraded, and removed. The issue with this is that it depends on Android's
    // PackageManager sending the events and this app receiving them. Applications installed by
    // manually copying the apk files or by flashing from recovery will not get synced. Also,
    // if the patcher is synced, it may or may not catch the PACKAGE_REPLACED broadcast for itself.
    //
    // The downside of using the daemon is that it determines when apps are installed, upgraded,
    // or removed by monitoring /data/app with inotify. The issue here is that during an upgrade,
    // Android puts the new apk file in /data/app and removes the old one when it's done. Also,
    // there is no guarantee that the patcher will receive any PACKAGE_* broadcasts when itself
    // is upgraded. To avoid issues, syncdaemon will delay the syncing by 30 seconds when it
    // receives an inotify event. In this mode, syncdaemon will be spawned if it isn't already
    // running by the time Android sends BOOT_COMPLETED.
    public static final boolean NO_DAEMON = false;

    private static final String SHARE_APPS_PATH = "/data/patcher.share-app";
    private static final String SHARE_PAID_APPS_PATH = "/data/patcher.share-app-asec";

    public static boolean setShareAppsEnabled(boolean enable) {
        if (enable) {
            CommandUtils.runRootCommand("touch " + SHARE_APPS_PATH);
            return isShareAppsEnabled();
        } else {
            CommandUtils.runRootCommand("rm -f " + SHARE_APPS_PATH);
            return !isShareAppsEnabled();
        }
    }

    public static boolean setSharePaidAppsEnabled(boolean enable) {
        if (enable) {
            CommandUtils.runRootCommand("touch " + SHARE_PAID_APPS_PATH);
            return isSharePaidAppsEnabled();
        } else {
            CommandUtils.runRootCommand("rm -f " + SHARE_PAID_APPS_PATH);
            return !isSharePaidAppsEnabled();
        }
    }

    public static boolean isShareAppsEnabled() {
        return FileUtils.isExistsFile(SHARE_APPS_PATH);
    }

    public static boolean isSharePaidAppsEnabled() {
        return FileUtils.isExistsFile(SHARE_PAID_APPS_PATH);
    }

    public static String[] getAllApks() {
        RomInformation[] roms = RomUtils.getRoms();

        ArrayList<String> apks = new ArrayList<String>();

        for (RomInformation rom : roms) {
            String[] filenames = FileUtils.listdir(rom.data + File.separator + "app");

            if (filenames == null || filenames.length == 0) {
                continue;
            }

            for (String filename : filenames) {
                if (filename.endsWith(".apk")) {
                    apks.add(rom.data + File.separator + "app" + File.separator + filename);
                }
            }
        }

        return apks.toArray(new String[apks.size()]);
    }

    public static int runSyncDaemonDaemon(Context context) {
        return runSyncDaemon(context, "--daemon");
    }

    public static int runSyncDaemonOnce(Context context) {
        return runSyncDaemon(context, " --runonce");
    }

    private static int runSyncDaemon(Context context, String args) {
        int uid = 0;

        try {
            uid = context.getPackageManager().getApplicationInfo(
                    context.getPackageName(), 0).uid;
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }

        if (uid == 0) {
            Log.e(TAG, "Couldn't determine UID");
            return -1;
        }

        // Clean up in case something crashed before
        final String path = "/data/local/tmp/syncdaemon";
        final String syncdaemon = path + "/syncdaemon";

        FileUtils.createTmpFs(path);
        FileUtils.chown(uid, uid, path);

        FileUtils.extractAsset(context, "syncdaemon", new File(syncdaemon));
        int exitCode = CommandUtils.runRootCommand("chmod 755 " + syncdaemon);

        CommandUtils.runRootCommand(syncdaemon + args);

        FileUtils.unmountAndRemove(path);

        return exitCode;
    }

    public static boolean isSyncDaemonRunning() {
        CommandParams params = new CommandParams();
        params.command = new String[] {"pidof", "syncdaemon"};

        CommandRunner cmd = new CommandRunner(params);
        cmd.start();

        try {
            cmd.join();
            CommandResult result = cmd.getResult();
            return result.exitCode == 0;
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        return false;
    }
}
