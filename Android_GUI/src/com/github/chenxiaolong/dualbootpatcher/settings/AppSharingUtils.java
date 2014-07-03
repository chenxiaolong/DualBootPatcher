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

import java.io.File;
import java.util.ArrayList;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

public class AppSharingUtils {
    public static final String TAG = "SettingsUtils";

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
}
