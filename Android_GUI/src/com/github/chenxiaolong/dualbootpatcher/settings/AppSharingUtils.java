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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.content.Context;
import android.os.Environment;

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

public class AppSharingUtils {
    public static final String TAG = AppSharingUtils.class.getSimpleName();

    private static final String SHARE_APPS_PATH =
            Environment.getExternalStorageDirectory() + "/MultiBoot/%s/share-app";
    private static final String SHARE_PAID_APPS_PATH =
            Environment.getExternalStorageDirectory() + "/MultiBoot/%s/share-app-asec";

    public static boolean setShareAppsEnabled(boolean enable) {
        String path = String.format(SHARE_APPS_PATH, "ID");
        File f = new File(path);
        if (enable) {
            try {
                f.createNewFile();
            } catch (IOException e) {
                e.printStackTrace();
            }
            return isShareAppsEnabled();
        } else {
            f.delete();
            return !isShareAppsEnabled();
        }
    }

    public static boolean setSharePaidAppsEnabled(boolean enable) {
        String path = String.format(SHARE_PAID_APPS_PATH);
        File f = new File(path);
        if (enable) {
            try {
                f.createNewFile();
            } catch (IOException e) {
                e.printStackTrace();
            }
            return isSharePaidAppsEnabled();
        } else {
            f.delete();
            return !isSharePaidAppsEnabled();
        }
    }

    public static boolean isShareAppsEnabled() {
        return new File(SHARE_APPS_PATH).isFile();
    }

    public static boolean isSharePaidAppsEnabled() {
        return new File(SHARE_PAID_APPS_PATH).isFile();
    }

    public static HashMap<RomInformation, ArrayList<String>> getAllApks(Context context) {
        RomInformation[] roms = RomUtils.getRoms(context);

        HashMap<RomInformation, ArrayList<String>> apksMap = new HashMap<>();

        for (RomInformation rom : roms) {
            ArrayList<String> apks = new ArrayList<>();
            String[] filenames = new File(rom.getDataPath() + File.separator + "app").list();

            if (filenames == null || filenames.length == 0) {
                continue;
            }

            for (String filename : filenames) {
                if (filename.endsWith(".apk")) {
                    apks.add(rom.getDataPath() + File.separator + "app" + File.separator + filename);
                }
            }

            if (apks.size() > 0) {
                apksMap.put(rom, apks);
            }
        }

        return apksMap;
    }
}
