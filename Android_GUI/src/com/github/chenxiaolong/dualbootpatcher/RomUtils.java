/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.os.Build;
import android.os.Environment;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.StatBuf;
import com.squareup.picasso.Picasso;

import org.apache.commons.io.Charsets;
import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

import mbtool.daemon.v3.FileOpenFlag;

public class RomUtils {
    private static final String TAG = RomUtils.class.getSimpleName();

    private static RomInformation[] mBuiltinRoms;

    public static final String UNKNOWN_ID = "unknown";
    public static final String PRIMARY_ID = "primary";
    public static final String SECONDARY_ID = "dual";
    public static final String MULTI_ID_PREFIX = "multi-slot-";
    public static final String DATA_ID_PREFIX = "data-slot-";
    public static final String EXTSD_ID_PREFIX = "extsd-slot-";

    @SuppressWarnings("unused")
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
        private String mWallpaperPath;
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
            mWallpaperPath = in.readString();
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
            dest.writeString(mWallpaperPath);
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
                    + ", wallpaperPath=" + mWallpaperPath
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

        public String getWallpaperPath() {
            return mWallpaperPath;
        }

        public void setWallpaperPath(String wallpaperPath) {
            mWallpaperPath = wallpaperPath;
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

    @Nullable
    public static RomInformation getCurrentRom(Context context) {
        try {
            String id = MbtoolSocket.getInstance().getBootedRomId(context);
            Log.d(TAG, "mbtool says current ROM ID is: " + id);

            for (RomInformation rom : getRoms(context)) {
                if (rom.getId().equals(id)) {
                    return rom;
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "mbtool communication error", e);
        }

        return null;
    }

    @NonNull
    public synchronized static RomInformation[] getRoms(Context context) {
        RomInformation[] roms = new RomInformation[0];

        try {
            roms = MbtoolSocket.getInstance().getInstalledRoms(context);

            for (RomInformation rom : roms) {
                rom.setThumbnailPath(Environment.getExternalStorageDirectory()
                        + "/MultiBoot/" + rom.getId() + "/thumbnail.webp");
                rom.setWallpaperPath(Environment.getExternalStorageDirectory()
                        + "/MultiBoot/" + rom.getId() + "/wallpaper.webp");
                rom.setConfigPath(Environment.getExternalStorageDirectory()
                        + "/MultiBoot/" + rom.getId() + "/config.json");
                rom.setImageResId(R.drawable.rom_android);
                rom.setDefaultName(getDefaultName(context, rom));

                loadConfig(rom);
            }
        } catch (IOException e) {
            Log.e(TAG, "mbtool communication error", e);
        }

        return roms;
    }

    @NonNull
    public synchronized static RomInformation[] getBuiltinRoms(Context context) {
        if (mBuiltinRoms == null) {
            String[] ids = new String[]{"primary", "dual", "multi-slot-1", "multi-slot-2",
                    "multi-slot-3"};

            mBuiltinRoms = new RomInformation[ids.length];
            for (int i = 0; i < ids.length; i++) {
                RomInformation rom = new RomInformation();
                rom.setId(ids[i]);
                rom.setDefaultName(getDefaultName(context, rom));
                mBuiltinRoms[i] = rom;
            }
        }

        return mBuiltinRoms;
    }

    public static void loadConfig(RomInformation info) {
        RomConfig config = RomConfig.getConfig(info.getConfigPath());
        info.setName(config.getName());
    }

    public static void saveConfig(RomInformation info) {
        RomConfig config = RomConfig.getConfig(info.getConfigPath());

        config.setId(info.getId());
        config.setName(info.getName());

        try {
            config.commit();
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Failed to save ROM config", e);
        }
    }

    private static String getDefaultName(Context context, RomInformation info) {
        if (info.getId().equals(PRIMARY_ID)) {
            return context.getString(R.string.primary);
        } else if (info.getId().equals(SECONDARY_ID)) {
            return context.getString(R.string.secondary);
        } else if (info.getId().startsWith(MULTI_ID_PREFIX)) {
            String num = info.getId().substring(MULTI_ID_PREFIX.length());
            return context.getString(R.string.multislot, num);
        } else if (info.getId().startsWith(DATA_ID_PREFIX)) {
            String id = info.getId().substring(DATA_ID_PREFIX.length());
            return context.getString(R.string.dataslot, id);
        } else if (info.getId().startsWith(EXTSD_ID_PREFIX)) {
            String id = info.getId().substring(EXTSD_ID_PREFIX.length());
            return context.getString(R.string.extsdslot, id);
        }

        return UNKNOWN_ID;
    }

    public static String getBootImagePath(String romId) {
        return Environment.getExternalStorageDirectory() +
                File.separator + "MultiBoot" +
                File.separator + romId +
                File.separator + "boot.img";
    }

    @NonNull
    public static String getDeviceCodename(Context context) {
        return SystemPropertiesProxy.get(context, "ro.patcher.device", Build.DEVICE);
    }

    private static boolean usesLiveWallpaper(Context context, RomInformation info) {
        String wallpaperInfoPath = info.getDataPath() + "/system/users/0/wallpaper_info.xml";

        MbtoolSocket socket = MbtoolSocket.getInstance();
        int id = -1;

        try {
            id = socket.fileOpen(context, wallpaperInfoPath, new short[]{ FileOpenFlag.RDONLY }, 0);
            if (id < 0) {
                return false;
            }

            StatBuf sb = socket.fileStat(context, id);
            if (sb == null) {
                return false;
            }

            // Check file size
            if (sb.st_size < 0 || sb.st_size > 1024) {
                return false;
            }

            // Read file into memory
            byte[] data = new byte[(int) sb.st_size];
            int nWritten = 0;
            while (nWritten < data.length) {
                ByteBuffer newData = socket.fileRead(context, id, 10240);
                if (newData == null) {
                    return false;
                }

                int nRead = newData.limit() - newData.position();
                newData.get(data, nWritten, nRead);
                nWritten += nRead;
            }

            socket.fileClose(context, id);
            id = -1;

            String xml = new String(data, Charsets.UTF_8);
            return xml.contains("component=");
        } catch (IOException e) {
            return false;
        } finally {
            if (id >= 0) {
                try {
                    socket.fileClose(context, id);
                } catch (IOException e) {
                    // Ignore
                }
            }
        }
    }

    public enum CacheWallpaperResult {
        UP_TO_DATE,
        UPDATED,
        FAILED,
        USES_LIVE_WALLPAPER
    }

    public static CacheWallpaperResult cacheWallpaper(Context context, RomInformation info) {
        if (usesLiveWallpaper(context, info)) {
            // We can't render a snapshot of a live wallpaper
            return CacheWallpaperResult.USES_LIVE_WALLPAPER;
        }

        String wallpaperPath = info.getDataPath() + "/system/users/0/wallpaper";
        File wallpaperCacheFile = new File(info.getWallpaperPath());
        FileOutputStream fos = null;

        MbtoolSocket socket = MbtoolSocket.getInstance();
        int id = -1;

        try {
            id = socket.fileOpen(context, wallpaperPath, new short[]{}, 0);
            if (id < 0) {
                return CacheWallpaperResult.FAILED;
            }

            // Check if we need to re-cache the file
            StatBuf sb = socket.fileStat(context, id);
            if (sb == null) {
                return CacheWallpaperResult.FAILED;
            }

            if (wallpaperCacheFile.exists()
                    && wallpaperCacheFile.lastModified() / 1000 > sb.st_mtime) {
                Log.d(TAG, "Wallpaper for " + info.getId() + " has not been changed");
                return CacheWallpaperResult.UP_TO_DATE;
            }

            // Ignore large wallpapers
            if (sb.st_size < 0 || sb.st_size > 20 * 1024 * 1024) {
                return CacheWallpaperResult.FAILED;
            }

            // Read file into memory
            byte[] data = new byte[(int) sb.st_size];
            int nWritten = 0;
            while (nWritten < data.length) {
                ByteBuffer newData = socket.fileRead(context, id, 10240);
                if (newData == null) {
                    return CacheWallpaperResult.FAILED;
                }

                int nRead = newData.limit() - newData.position();
                newData.get(data, nWritten, nRead);
                nWritten += nRead;
            }

            socket.fileClose(context, id);
            id = -1;

            fos = new FileOutputStream(wallpaperCacheFile);

            // Compression can be very slow (more than 10 seconds) for a large wallpaper, so we'll
            // just cache the actual file instead
            fos.write(data);

            // Load into bitmap
            //Bitmap bitmap = BitmapFactory.decodeByteArray(data, 0, data.length);
            //if (bitmap == null) {
            //    return false;
            //}
            //bitmap.compress(Bitmap.CompressFormat.WEBP, 100, fos);
            //bitmap.recycle();

            // Invalidate picasso cache
            Picasso.with(context).invalidate(wallpaperCacheFile);

            Log.d(TAG, "Wallpaper for " + info.getId() + " has been cached");
            return CacheWallpaperResult.UPDATED;
        } catch (IOException e) {
            Log.e(TAG, "Failed to cache wallpaper for " + info.getId(), e);
            return CacheWallpaperResult.FAILED;
        } finally {
            if (id >= 0) {
                try {
                    socket.fileClose(context, id);
                } catch (IOException e) {
                    // Ignore
                }
            }
            IOUtils.closeQuietly(fos);
        }
    }
}
