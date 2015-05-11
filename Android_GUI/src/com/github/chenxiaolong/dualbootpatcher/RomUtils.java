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
import android.os.Environment;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.github.chenxiaolong.multibootpatcher.socket.MbtoolSocket;
import com.google.gson.Gson;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.OutputStreamWriter;
import java.io.UnsupportedEncodingException;

public class RomUtils {
    private static final String TAG = RomUtils.class.getSimpleName();

    private static RomInformation[] mBuiltinRoms;

    public static final String UNKNOWN_ID = "unknown";
    public static final String PRIMARY_ID = "primary";
    public static final String SECONDARY_ID = "dual";
    public static final String MULTI_ID_PREFIX = "multi-slot-";
    public static final String DATA_ID_PREFIX = "data-slot-";

    public static class RomInformation implements Parcelable {
        // Mount points
        private String mSystem;
        private String mCache;
        private String mData;

        // Identifiers
        private String mId;

        private String mVersion;
        private String mBuild;

        private String mThumbnailPath;
        private String mConfigPath;

        private String mDefaultName;
        private String mName;
        private int mImageResId;

        public RomInformation() {
        }

        protected RomInformation(Parcel in) {
            mSystem = in.readString();
            mCache = in.readString();
            mData = in.readString();
            mId = in.readString();
            mVersion = in.readString();
            mBuild = in.readString();
            mThumbnailPath = in.readString();
            mConfigPath = in.readString();
            mDefaultName = in.readString();
            mName = in.readString();
            mImageResId = in.readInt();
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(mSystem);
            dest.writeString(mCache);
            dest.writeString(mData);
            dest.writeString(mId);
            dest.writeString(mVersion);
            dest.writeString(mBuild);
            dest.writeString(mThumbnailPath);
            dest.writeString(mConfigPath);
            dest.writeString(mDefaultName);
            dest.writeString(mName);
            dest.writeInt(mImageResId);
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
            return "id=" + mId
                    + ", system=" + mSystem
                    + ", cache=" + mCache
                    + ", data=" + mData
                    + ", version=" + mVersion
                    + ", build=" + mBuild
                    + ", thumbnailPath=" + mThumbnailPath
                    + ", configPath=" + mConfigPath
                    + ", defaultName=" + mDefaultName
                    + ", name=" + mName
                    + ", imageResId=" + mImageResId;
        }

        public String getSystemPath() {
            return mSystem;
        }

        public void setSystemPath(String system) {
            mSystem = system;
        }

        public String getCachePath() {
            return mCache;
        }

        public void setCachePath(String cache) {
            mCache = cache;
        }

        public String getDataPath() {
            return mData;
        }

        public void setDataPath(String data) {
            mData = data;
        }

        public String getId() {
            return mId;
        }

        public void setId(String id) {
            mId = id;
        }

        public String getVersion() {
            return mVersion;
        }

        public void setVersion(String version) {
            mVersion = version;
        }

        public String getBuild() {
            return mBuild;
        }

        public void setBuild(String build) {
            mBuild = build;
        }

        public String getThumbnailPath() {
            return mThumbnailPath;
        }

        public void setThumbnailPath(String thumbnailPath) {
            mThumbnailPath = thumbnailPath;
        }

        public String getConfigPath() {
            return mConfigPath;
        }

        public void setConfigPath(String configPath) {
            mConfigPath = configPath;
        }

        public String getDefaultName() {
            return mDefaultName;
        }

        public void setDefaultName(String defaultName) {
            mDefaultName = defaultName;
        }

        public String getName() {
            return mName != null ? mName : mDefaultName;
        }

        public void setName(String name) {
            mName = name;
        }

        public int getImageResId() {
            return mImageResId;
        }

        private void setImageResId(int imageResId) {
            mImageResId = imageResId;
        }
    }

    private static class RomConfig {
        public String id;
        public String name;
    }

    public static RomInformation getCurrentRom(Context context) {
        String id = MbtoolSocket.getInstance().getCurrentRom(context);
        Log.d(TAG, "mbtool says current ROM ID is: " + id);

        if (id != null) {
            for (RomInformation rom : getRoms(context)) {
                if (rom.getId().equals(id)) {
                    return rom;
                }
            }
        }

        return null;
    }

    public synchronized static RomInformation[] getRoms(Context context) {
        RomInformation[] roms = MbtoolSocket.getInstance().getInstalledRoms(context);

        if (roms != null) {
            for (RomInformation rom : roms) {
                rom.setThumbnailPath(Environment.getExternalStorageDirectory()
                        + "/MultiBoot/" + rom.getId() + "/thumbnail.webp");
                rom.setConfigPath(Environment.getExternalStorageDirectory()
                        + "/MultiBoot/" + rom.getId() + "/config.json");
                rom.setImageResId(R.drawable.rom_android);
                rom.setDefaultName(getDefaultName(context, rom));

                loadConfig(rom);
            }
        }

        return roms;
    }

    public synchronized static RomInformation[] getBuiltinRoms(Context context) {
        if (mBuiltinRoms == null) {
            String[] ids = MbtoolSocket.getInstance().getBuiltinRomIds(context);

            if (ids != null) {
                mBuiltinRoms = new RomInformation[ids.length];
                for (int i = 0; i < ids.length; i++) {
                    RomInformation rom = new RomInformation();
                    rom.setId(ids[i]);
                    rom.setDefaultName(getDefaultName(context, rom));
                    mBuiltinRoms[i] = rom;
                }
            }
        }

        return mBuiltinRoms;
    }

    public static void loadConfig(RomInformation info) {
        RomConfig config = null;
        Gson gson = new Gson();
        try {
            config = gson.fromJson(new JsonReader(new FileReader(info.getConfigPath())),
                    RomConfig.class);
        } catch (FileNotFoundException e) {
            // Ignore
        }

        // Pretty minimal config file right now
        if (config != null) {
            info.setName(config.name);
        }
    }

    public static void saveConfig(RomInformation info) {
        File configFile = new File(info.getConfigPath());
        configFile.getParentFile().mkdirs();

        RomConfig config = new RomConfig();
        config.id = info.getId();
        config.name = info.getName();

        Gson gson = new Gson();

        JsonWriter writer = null;
        try {
            writer = new JsonWriter(new OutputStreamWriter(
                    new FileOutputStream(configFile), "UTF-8"));
            gson.toJson(config, RomConfig.class, writer);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } finally {
            IOUtils.closeQuietly(writer);
        }
    }

    private static String getDefaultName(Context context, RomInformation info) {
        if (info.getId().equals(PRIMARY_ID)) {
            return context.getString(R.string.primary);
        } else if (info.getId().equals(SECONDARY_ID)) {
            return context.getString(R.string.secondary);
        } else if (info.getId().startsWith(MULTI_ID_PREFIX)) {
            String num = info.getId().substring(MULTI_ID_PREFIX.length());
            return String.format(context.getString(R.string.multislot), num);
        } else if (info.getId().startsWith(DATA_ID_PREFIX)) {
            String id = info.getId().substring(DATA_ID_PREFIX.length());
            return String.format(context.getString(R.string.dataslot), id);
        }

        return UNKNOWN_ID;
    }

    public static String getBootImagePath(String romId) {
        return Environment.getExternalStorageDirectory() +
                File.separator + "MultiBoot" +
                File.separator + romId +
                File.separator + "boot.img";
    }
}
