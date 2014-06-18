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

import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandRunner;

public class SettingsUtils {
    public static final String TAG = "SettingsUtils";

    private static final String SHARE_APPS_PATH = "/data/patcher.share-app";
    private static final String SHARE_PAID_APPS_PATH = "/data/patcher.share-app-asec";

    private static int runRootCommand(String command) {
        RootCommandParams params = new RootCommandParams();
        params.command = command;

        RootCommandRunner cmd = new RootCommandRunner(params);
        cmd.start();

        try {
            cmd.join();
            CommandResult result = cmd.getResult();

            return result.exitCode;
        } catch (Exception e) {
            e.printStackTrace();
        }

        return -1;
    }

    public static boolean setShareAppsEnabled(boolean enable) {
        if (enable) {
            runRootCommand("touch " + SHARE_APPS_PATH);
            return isShareAppsEnabled();
        } else {
            runRootCommand("rm -f " + SHARE_APPS_PATH);
            return !isShareAppsEnabled();
        }
    }

    public static boolean setSharePaidAppsEnabled(boolean enable) {
        if (enable) {
            runRootCommand("touch " + SHARE_PAID_APPS_PATH);
            return isSharePaidAppsEnabled();
        } else {
            runRootCommand("rm -f " + SHARE_PAID_APPS_PATH);
            return !isSharePaidAppsEnabled();
        }
    }

    public static boolean isShareAppsEnabled() {
        return runRootCommand("ls " + SHARE_APPS_PATH) == 0;
    }

    public static boolean isSharePaidAppsEnabled() {
        return runRootCommand("ls " + SHARE_PAID_APPS_PATH) == 0;
    }
}
