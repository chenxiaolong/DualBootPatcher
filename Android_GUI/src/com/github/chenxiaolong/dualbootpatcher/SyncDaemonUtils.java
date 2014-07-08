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

package com.github.chenxiaolong.dualbootpatcher;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandRunner;

import java.io.File;

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
public class SyncDaemonUtils {
    public static final String TAG = "SyncDaemonUtils";
    public static final String MOUNT_POINT = "/data/local/tmp/syncdaemon";

    public static boolean isRunning(Context context) {
        CommandParams params = new CommandParams();
        params.command = CommandUtils.getBusyboxCommand(context, "pidof",
                new String[] { "syncdaemon" });

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

    public static int runDaemon(Context context) {
        return run(context, "--daemon");
    }

    private static int run(Context context, String args) {
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

        File syncdaemon = new File(context.getCacheDir() + File.separator + "syncdaemon");
        if (!syncdaemon.exists()) {
            FileUtils.extractAsset(context, "syncdaemon", syncdaemon);
        }

        // Clean up in case something crashed before
        final String daemonBinary = MOUNT_POINT + File.separator + "syncdaemon";

        RootFile mountPoint = new RootFile(MOUNT_POINT);
        mountPoint.mountTmpFs();
        mountPoint.chown(uid, uid);

        CommandUtils.runRootCommand("cp " + syncdaemon.getAbsolutePath() + " " + daemonBinary);
        new RootFile(daemonBinary).chmod(0755);
        CommandUtils.runRootCommand("chcon u:object_r:system_file:s0 " + daemonBinary);

        int exitCode = CommandUtils.runRootCommand(daemonBinary + args);

        mountPoint.unmountTmpFs();
        mountPoint.recursiveDelete();

        return exitCode;
    }
}
