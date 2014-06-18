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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import java.io.File;
import java.util.ArrayList;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.content.Context;

import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.MiscUtils;
import com.github.chenxiaolong.dualbootpatcher.R;

public class RomDetector {
    private static ArrayList<String> mRoms;

    public static String[] getRoms() {
        if (mRoms == null) {
            mRoms = new ArrayList<String>();

            // Primary - either:
            // - No /raw-system and have /system/build.prop
            // - Have /raw-system/build.prop
            if (!new File("/raw-system").exists()
                    && new File("/system/build.prop").exists()) {
                mRoms.add("/system");
            } else if (new File("/raw-system/build.prop").exists()) {
                mRoms.add("/raw-system");
            }

            if (FileUtils.isExistsDirectory("/raw-system/dual")) {
                mRoms.add("/raw-system/dual");
            } else if (FileUtils.isExistsDirectory("/system/dual")) {
                mRoms.add("/system/dual");
            }

            int max = 10;
            for (int i = 0; i < max; i++) {
                if (FileUtils.isExistsDirectory("/raw-cache/multi-slot-" + i + "/system")) {
                    mRoms.add("/raw-cache/multi-slot-" + i + "/system");
                } else if (FileUtils.isExistsDirectory("/cache/multi-slot-" + i + "/system")) {
                    mRoms.add("/cache/multi-slot-" + i + "/system");
                }
            }
        }

        return mRoms.toArray(new String[mRoms.size()]);
    }

    private static String getDefaultName(Context context, String rom) {
        if (rom.equals("/system") || rom.equals("/raw-system")) {
            return context.getString(R.string.primary);
        } else if (rom.contains("system/dual")) {
            return context.getString(R.string.secondary);
        } else if (rom.contains("cache/multi-slot-")) {
            Pattern p = Pattern
                    .compile("^/(?:raw-)?cache/multi-slot-([^/]+)/system$");
            Matcher m = p.matcher(rom);
            String num = "UNKNOWN";
            if (m.find()) {
                num = m.group(1);
            }
            return String.format(context.getString(R.string.multislot), num);
        } else {
            return "UNKNOWN";
        }
    }

    public static String getName(Context context, String rom) {
        // TODO: Allow renaming
        return getDefaultName(context, rom);
    }

    public static String getId(String rom) {
        if (rom.equals("/system") || rom.equals("/raw-system")) {
            return "primary";
        } else if (rom.contains("system/dual")) {
            return "secondary";
        } else if (rom.contains("cache/multi-slot-")) {
            Pattern p = Pattern
                    .compile("^/(?:raw-)?cache/(multi-slot-[^/]+)/system$");
            Matcher m = p.matcher(rom);
            if (m.find()) {
                return m.group(1);
            } else {
                return null;
            }
        } else {
            return null;
        }
    }

    private static Properties getBuildProp(String rom) {
        return MiscUtils.getProperties(rom + "/build.prop");
    }

    public static String getVersion(String rom) {
        Properties prop = getBuildProp(rom);

        if (prop == null) {
            return null;
        }

        if (prop.containsKey("ro.modversion")) {
            return prop.getProperty("ro.modversion");
        } else if (prop.containsKey("ro.slim.version")) {
            return prop.getProperty("ro.slim.version");
        } else if (prop.containsKey("ro.cm.version")) {
            return prop.getProperty("ro.cm.version");
        } else if (prop.containsKey("ro.omni.version")) {
            return prop.getProperty("ro.omni.version");
        } else if (prop.containsKey("ro.build.display.id")) {
            return prop.getProperty("ro.build.display.id");
        } else {
            return null;
        }
    }

    public static int getIconResource(String rom) {
        Properties prop = getBuildProp(rom);

        if (prop == null) {
            return R.drawable.rom_android;
        }

        if (prop.containsKey("ro.slim.version")) {
            return R.drawable.rom_slimroms;
        } else if (prop.containsKey("ro.cm.version")) {
            return R.drawable.rom_cyanogenmod;
        } else if (prop.containsKey("ro.omni.version")) {
            return R.drawable.rom_omnirom;
        } else {
            return R.drawable.rom_android;
        }
    }
}
