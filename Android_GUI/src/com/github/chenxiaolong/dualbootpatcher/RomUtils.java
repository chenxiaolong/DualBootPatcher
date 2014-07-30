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
import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.settings.RomInfoConfigFile;

import java.io.File;
import java.util.ArrayList;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class RomUtils {
    private static final String TAG = RomUtils.class.getSimpleName();

    private static ArrayList<RomInformation> mRoms;

    public static final String BUILD_PROP = "build.prop";

    public static final String RAW_SYSTEM = "/raw-system";
    public static final String RAW_CACHE = "/raw-cache";
    public static final String RAW_DATA = "/raw-data";
    public static final String SYSTEM = "/system";
    public static final String CACHE = "/cache";
    public static final String DATA = "/data";

    public static final String UNKNOWN_ID = "unknown";
    public static final String PRIMARY_ID = "primary";
    public static final String SECONDARY_ID = "dual";
    public static final String MULTI_ID_PREFIX = "multi-slot-";
    public static final String PRIMARY_KERNEL_ID = PRIMARY_ID;
    public static final String SECONDARY_KERNEL_ID = "secondary";
    public static final String MULTI_KERNEL_ID_PREFIX = MULTI_ID_PREFIX;

    private static final String KEY_MOD_VERSION = "ro.modversion";
    private static final String KEY_SLIM_VERSION = "ro.slim.version";
    private static final String KEY_CM_VERSION = "ro.cm.version";
    private static final String KEY_OMNI_VERSION = "ro.omni.version";
    private static final String KEY_DISPLAY_ID = "ro.build.display.id";

    public static class RomInformation implements Parcelable {
        // Mount points
        public String system;
        public String cache;
        public String data;

        // Identifiers
        public String id;
        public String kernelId;

        public RomInformation() {
        }

        protected RomInformation(Parcel in) {
            system = in.readString();
            cache = in.readString();
            data = in.readString();
            id = in.readString();
            kernelId = in.readString();
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(system);
            dest.writeString(cache);
            dest.writeString(data);
            dest.writeString(id);
            dest.writeString(kernelId);
        }

        @SuppressWarnings("unused")
        public static final Parcelable.Creator<RomInformation> CREATOR =
                new Parcelable.Creator<RomInformation>() {
            @Override
            public RomInformation createFromParcel(Parcel in) {
                return new RomInformation(in);
            }

            @Override
            public RomInformation[] newArray(int size) {
                return new RomInformation[size];
            }
        };
    }

    private static boolean isBootedInPrimary(Context context) {
        return !new RootFile(RAW_SYSTEM).isDirectory() || FileUtils.isSameInode(context,
                RAW_SYSTEM + File.separator + BUILD_PROP, SYSTEM + File.separator + BUILD_PROP);
    }

    public static RomInformation getCurrentRom(Context context) {
        RomInformation romProp = getCurrentRomViaProp(context);
        RomInformation romInode = getCurrentRomViaInode(context);

        if (romProp != romInode) {
            Log.e(TAG, "The default.prop and inode methods returned different ROMs!");
        }

        if (romInode != null) {
            return romInode;
        } else {
            return romProp;
        }
    }

    private static RomInformation getCurrentRomViaProp(Context context) {
        String curPartConfig = MiscUtils.getPartitionConfig();
        RomInformation[] roms = getRoms(context);

        for (RomInformation rom : roms) {
            if (rom.id.equals(PRIMARY_KERNEL_ID) && curPartConfig == null) {
                return rom;
            } else if (rom.id.equals(curPartConfig)) {
                return rom;
            }
        }

        return null;
    }

    @SuppressWarnings("unused")
    private static RomInformation getCurrentRomViaInode(Context context) {
        RomInformation[] roms = getRoms(context);

        for (RomInformation rom : roms) {
            if (FileUtils.isSameInode(context, rom.system + File.separator + BUILD_PROP,
                    SYSTEM + File.separator + BUILD_PROP)) {
                return rom;
            }
        }

        return null;
    }

    public static RomInformation[] getRoms(Context context) {
        if (mRoms == null) {
            mRoms = new ArrayList<RomInformation>();

            RomInformation info;

            // Check if primary ROM exists
            if (isBootedInPrimary(context)) {
                info = new RomInformation();

                info.system = SYSTEM;
                info.cache = CACHE;
                info.data = DATA;

                info.id = PRIMARY_ID;
                info.kernelId = PRIMARY_KERNEL_ID;

                mRoms.add(info);
            } else if (new RootFile(RAW_SYSTEM + File.separator + BUILD_PROP).isFile()) {
                info = new RomInformation();

                info.system = RAW_SYSTEM;
                info.cache = RAW_CACHE;
                info.data = RAW_DATA;

                info.id = PRIMARY_ID;
                info.kernelId = PRIMARY_KERNEL_ID;

                mRoms.add(info);
            }

            if (new RootFile(RAW_SYSTEM + File.separator + SECONDARY_ID + File
                    .separator + BUILD_PROP).isFile()) {
                info = new RomInformation();

                info.system = RAW_SYSTEM + File.separator + SECONDARY_ID;
                info.cache = RAW_CACHE + File.separator + SECONDARY_ID;
                info.data = RAW_DATA + File.separator + SECONDARY_ID;

                info.id = SECONDARY_ID;
                info.kernelId = SECONDARY_KERNEL_ID;

                mRoms.add(info);
            } else if (new RootFile(SYSTEM + File.separator + SECONDARY_ID + File
                    .separator + BUILD_PROP).isFile()) {
                info = new RomInformation();

                info.system = SYSTEM + File.separator + SECONDARY_ID;
                info.cache = CACHE + File.separator + SECONDARY_ID;
                info.data = DATA + File.separator + SECONDARY_ID;

                info.id = SECONDARY_ID;
                info.kernelId = SECONDARY_KERNEL_ID;

                mRoms.add(info);
            }

            int max = 10;
            for (int i = 0; i < max; i++) {
                String id = MULTI_ID_PREFIX + i;
                String rawSystemPath = RAW_CACHE + File.separator + id + SYSTEM;
                String rawCachePath = RAW_SYSTEM + File.separator + id + CACHE;
                String rawDataPath = RAW_DATA + File.separator + id;
                String systemPath = CACHE + File.separator + id + SYSTEM;
                String cachePath = SYSTEM + File.separator + id + CACHE;
                String dataPath = DATA + File.separator + id;

                if (new RootFile(rawSystemPath).isDirectory()) {
                    info = new RomInformation();

                    info.system = rawSystemPath;
                    info.cache = rawCachePath;
                    info.data = rawDataPath;

                    info.id = id;
                    info.kernelId = id;

                    mRoms.add(info);
                } else if (new RootFile(systemPath).isDirectory()) {
                    info = new RomInformation();

                    info.system = systemPath;
                    info.cache = cachePath;
                    info.data = dataPath;

                    info.id = id;
                    info.kernelId = id;

                    mRoms.add(info);
                }
            }
        }

        return mRoms.toArray(new RomInformation[mRoms.size()]);
    }

    public static RomInformation getRomFromId(Context context, String id) {
        RomInformation[] roms = getRoms(context);
        for (RomInformation rom : roms) {
            if (rom.id.equals(id)) {
                return rom;
            }
        }

        return null;
    }

    public static String getDefaultName(Context context, RomInformation info) {
        if (info.kernelId.equals(PRIMARY_KERNEL_ID)) {
            return context.getString(R.string.primary);
        } else if (info.kernelId.equals(SECONDARY_KERNEL_ID)) {
            return context.getString(R.string.secondary);
        } else if (info.kernelId.startsWith(MULTI_KERNEL_ID_PREFIX)) {
            Pattern p = Pattern.compile("^" + MULTI_KERNEL_ID_PREFIX + "(.+)");
            Matcher m = p.matcher(info.kernelId);
            String num;
            if (m.find()) {
                num = m.group(1);
                return String.format(context.getString(R.string.multislot), num);
            }
        }

        return UNKNOWN_ID;
    }

    public static String getName(Context context, RomInformation info) {
        String customName = RomInfoConfigFile.getInstance().getRomName(info);
        if (customName == null || customName.trim().isEmpty()) {
            return getDefaultName(context, info);
        } else {
            return customName;
        }
    }

    public static void setName(RomInformation info, String name) {
        final RomInfoConfigFile config = RomInfoConfigFile.getInstance();

        if (name != null && !name.trim().isEmpty()) {
            config.setRomName(info, name.trim());
        } else {
            config.setRomName(info, null);
        }

        new Thread() {
            @Override
            public void run() {
                config.save();
            }
        }.start();
    }

    private static Properties getBuildProp(RomInformation info) {
        return MiscUtils.getProperties(info.system + File.separator + BUILD_PROP);
    }

    public static String getVersion(RomInformation info) {
        Properties prop = getBuildProp(info);

        if (prop == null) {
            return null;
        }

        if (prop.containsKey(KEY_MOD_VERSION)) {
            return prop.getProperty(KEY_MOD_VERSION);
        } else if (prop.containsKey(KEY_SLIM_VERSION)) {
            return prop.getProperty(KEY_SLIM_VERSION);
        } else if (prop.containsKey(KEY_CM_VERSION)) {
            return prop.getProperty(KEY_CM_VERSION);
        } else if (prop.containsKey(KEY_OMNI_VERSION)) {
            return prop.getProperty(KEY_OMNI_VERSION);
        } else if (prop.containsKey(KEY_DISPLAY_ID)) {
            return prop.getProperty(KEY_DISPLAY_ID);
        } else {
            return null;
        }
    }

    public static int getIconResource(RomInformation info) {
        Properties prop = getBuildProp(info);

        if (prop == null) {
            return R.drawable.rom_android;
        }

        if (prop.containsKey(KEY_SLIM_VERSION)) {
            return R.drawable.rom_slimroms;
        } else if (prop.containsKey(KEY_CM_VERSION)) {
            return R.drawable.rom_cyanogenmod;
        } else if (prop.containsKey(KEY_OMNI_VERSION)) {
            return R.drawable.rom_omnirom;
        } else {
            return R.drawable.rom_android;
        }
    }
}
