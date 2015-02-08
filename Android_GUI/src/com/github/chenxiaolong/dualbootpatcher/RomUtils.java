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

package com.github.chenxiaolong.dualbootpatcher;

import android.content.Context;
import android.os.Parcel;
import android.os.Parcelable;

import com.github.chenxiaolong.dualbootpatcher.settings.RomInfoConfigFile;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolSocket;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

// TODO: This should completely go away as soon as the mbtool daemon is ready. Too much
//       duplicated code here

public class RomUtils {
    private static final String TAG = RomUtils.class.getSimpleName();

    private static RomInformation[] mRoms;

    public static final String UNKNOWN_ID = "unknown";
    public static final String PRIMARY_ID = "primary";
    public static final String SECONDARY_ID = "dual";
    public static final String MULTI_ID_PREFIX = "multi-slot-";

    public static class RomInformation implements Parcelable {
        // Mount points
        public String system;
        public String cache;
        public String data;

        public boolean useRawPaths;

        // Identifiers
        public String id;

        public String thumbnailPath;

        public RomInformation() {
        }

        protected RomInformation(Parcel in) {
            system = in.readString();
            cache = in.readString();
            data = in.readString();
            useRawPaths = in.readInt() != 0;
            id = in.readString();
            thumbnailPath = in.readString();
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
            dest.writeInt(useRawPaths ? 1 : 0);
            dest.writeString(id);
            dest.writeString(thumbnailPath);
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

        @Override
        public String toString() {
            return "id=" + id
                    + ", system=" + system
                    + ", cache=" + cache
                    + ", data=" + data
                    + ", useRawPaths=" + useRawPaths
                    + ", thumbnailPath=" + thumbnailPath;
        }
    }

    public static RomInformation getCurrentRom(Context context) {
        String id = null;
        try {
            id = SystemPropertiesProxy.get(context, "ro.multiboot.romid");
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        }

        if (id != null) {
            for (RomInformation rom : getRoms()) {
                if (rom.id.equals(id)) {
                    return rom;
                }
            }
        }

        return null;
    }

    public static RomInformation[] getRoms() {
        if (mRoms == null) {
            mRoms = MbtoolSocket.getInstance().getInstalledRoms();
        }

        return mRoms;
    }

    public static RomInformation getRomFromId(String id) {
        RomInformation[] roms = getRoms();
        for (RomInformation rom : roms) {
            if (rom.id.equals(id)) {
                return rom;
            }
        }

        return null;
    }

    public static String getDefaultName(Context context, RomInformation info) {
        if (info.id.equals(PRIMARY_ID)) {
            return context.getString(R.string.primary);
        } else if (info.id.equals(SECONDARY_ID)) {
            return context.getString(R.string.secondary);
        } else if (info.id.startsWith(MULTI_ID_PREFIX)) {
            Pattern p = Pattern.compile("^" + MULTI_ID_PREFIX + "(.+)");
            Matcher m = p.matcher(info.id);
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

    public static int getIconResource(RomInformation info) {
        return R.drawable.rom_android;
    }
}
