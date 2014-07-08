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

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.StringReader;
import java.util.Properties;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.FullRootOutputListener;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandRunner;

public class MiscUtils {
    private static String mVersion;

    public static String getVersion(Context context) {
        if (mVersion == null) {
            try {
                mVersion = context.getPackageManager().getPackageInfo(
                        context.getPackageName(), 0).versionName;
                mVersion = mVersion.split("-")[0];
            } catch (NameNotFoundException e) {
                e.printStackTrace();
            }
        }

        return mVersion;
    }

    public static int compareVersions(String version1, String version2) {
        String[] v1s = version1.split("\\.");
        String[] v2s = version2.split("\\.");
        int[] v1 = new int[v1s.length];
        int[] v2 = new int[v2s.length];

        if (v1s.length != v2s.length) {
            return -2;
        }

        try {
            for (int i = 0; i < v1s.length; i++) {
                v1[i] = Integer.parseInt(v1s[i]);
            }
            for (int i = 0; i < v2s.length; i++) {
                v2[i] = Integer.parseInt(v2s[i]);
            }

            for (int i = 0; i < v1.length; i++) {
                if (v1[i] < v2[i]) {
                    return -1;
                } else if (v1[i] > v2[i]) {
                    return 1;
                }
            }

            return 0;
        } catch (NumberFormatException e) {
            e.printStackTrace();
            return -2;
        }
    }

    public static String getPatchedByVersion() {
        Properties prop = getDefaultProp();

        if (prop == null) {
            return "0.0.0";
        }

        if (prop.containsKey("ro.patcher.version")) {
            return prop.getProperty("ro.patcher.version");
        } else {
            return "0.0.0";
        }
    }

    public static String getPartitionConfig() {
        Properties prop = getDefaultProp();

        if (prop == null) {
            return null;
        }

        if (prop.containsKey("ro.patcher.patched")) {
            return prop.getProperty("ro.patcher.patched");
        } else {
            return null;
        }
    }

    public static Properties getProperties(String file) {
        // Make sure build.prop exists
        if (!new RootFile(file).isFile()) {
            return null;
        }

        File propfile = new File(file);
        Properties prop = new Properties();

        String contents = new RootFile(file).getContents();

        if (contents == null) {
            return null;
        }

        try {
            prop.load(new StringReader(contents));
            return prop;
        } catch (IOException e) {
            e.printStackTrace();
        }

        return null;
    }

    public static Properties getDefaultProp() {
        return getProperties("/default.prop");
    }
}
