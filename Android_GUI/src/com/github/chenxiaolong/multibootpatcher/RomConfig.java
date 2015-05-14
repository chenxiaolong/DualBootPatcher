/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.multibootpatcher;

import android.support.annotation.NonNull;
import android.util.Log;

import com.google.gson.Gson;
import com.google.gson.JsonParseException;
import com.google.gson.annotations.SerializedName;
import com.google.gson.stream.JsonReader;
import com.google.gson.stream.JsonWriter;

import org.apache.commons.io.Charsets;
import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

public class RomConfig {
    private static final String TAG = RomConfig.class.getSimpleName();

    private static HashMap<String, RomConfig> sInstances = new HashMap<>();

    private String mFilename;

    private String mId;
    private String mName;
    private boolean mGlobalAppSharing;
    private boolean mGlobalPaidAppSharing;
    private boolean mIndivAppSharing;
    private HashMap<String, SharedItems> mSharedPkgs = new HashMap<>();

    public static class SharedItems {
        public boolean sharedApk;
        public boolean sharedData;

        public SharedItems() {
        }

        public SharedItems(boolean sharedApk, boolean sharedData) {
            this.sharedApk = sharedApk;
            this.sharedData = sharedData;
        }

        public SharedItems(SharedItems si) {
            this.sharedApk = si.sharedApk;
            this.sharedData = si.sharedData;
        }
    }

    private RomConfig(String filename) {
        mFilename = filename;
    }

    public static RomConfig getConfig(String filename) {
        File file = new File(filename);

        // Return existing instance if it exists
        if (sInstances.containsKey(file.getAbsolutePath())) {
            return sInstances.get(file.getAbsolutePath());
        }

        RomConfig config = new RomConfig(file.getAbsolutePath());
        try {
            config.loadFile();
        } catch (FileNotFoundException e) {
            Log.e(TAG, "Failed to load " + file, e);
        }

        sInstances.put(file.getAbsolutePath(), config);

        return config;
    }

    public synchronized void commit() throws FileNotFoundException {
        saveFile();
    }

    public void apply() {
        new Thread() {
            @Override
            public void run() {
                try {
                    commit();
                } catch (FileNotFoundException e) {
                    Log.e(TAG, "Asynchronous commit() failed", e);
                }
            }
        }.start();
    }

    public String getId() {
        return mId;
    }

    public void setId(String id) {
        mId = id;
    }

    public String getName() {
        return mName;
    }

    public void setName(String name) {
        mName = name;
    }

    public boolean isGlobalAppSharingEnabled() {
        return mGlobalAppSharing;
    }

    public void setGlobalAppSharingEnabled(boolean enabled) {
        mGlobalAppSharing = enabled;
    }

    public boolean isGlobalPaidAppSharingEnabled() {
        return mGlobalPaidAppSharing;
    }

    public void setGlobalPaidAppSharingEnabled(boolean enabled) {
        mGlobalPaidAppSharing = enabled;
    }

    public boolean isIndivAppSharingEnabled() {
        return mIndivAppSharing;
    }

    public void setIndivAppSharingEnabled(boolean enabled) {
        mIndivAppSharing = enabled;
    }

    @NonNull
    public HashMap<String, SharedItems> getIndivAppSharingPackages() {
        HashMap<String, SharedItems> result = new HashMap<>();
        for (Map.Entry<String, SharedItems> item : mSharedPkgs.entrySet()) {
            result.put(item.getKey(), new SharedItems(item.getValue()));
        }
        return result;
    }

    public void setIndivAppSharingPackages(HashMap<String, SharedItems> pkgs) {
        mSharedPkgs = new HashMap<>();
        for (Map.Entry<String, SharedItems> item : pkgs.entrySet()) {
            mSharedPkgs.put(item.getKey(), new SharedItems(item.getValue()));
        }
    }

    private RawRoot serialize() {
        RawRoot root = new RawRoot();

        root.id = mId;
        root.name = mName;

        root.appSharing = new RawAppSharing();
        root.appSharing.global = mGlobalAppSharing;
        root.appSharing.globalPaid = mGlobalPaidAppSharing;
        root.appSharing.individual = mIndivAppSharing;

        if (!mSharedPkgs.isEmpty()) {
            ArrayList<RawPackage> packages = new ArrayList<>();

            for (Map.Entry<String, SharedItems> item : mSharedPkgs.entrySet()) {
                RawPackage rp = new RawPackage();
                rp.pkgId = item.getKey();
                rp.shareApk = item.getValue().sharedApk;
                rp.shareData = item.getValue().sharedData;
                packages.add(rp);
            }

            root.appSharing.packages = packages.toArray(new RawPackage[mSharedPkgs.size()]);
        }

        return root;
    }

    private void deserialize(RawRoot root) {
        if (root == null) {
            return;
        }

        mId = root.id;
        mName = root.name;

        if (root.appSharing != null) {
            mGlobalAppSharing = root.appSharing.global;
            mGlobalPaidAppSharing = root.appSharing.globalPaid;
            mIndivAppSharing = root.appSharing.individual;

            if (root.appSharing.packages != null) {
                for (RawPackage rp : root.appSharing.packages) {
                    if (rp.pkgId == null) {
                        continue;
                    }

                    mSharedPkgs.put(rp.pkgId, new SharedItems(rp.shareApk, rp.shareData));
                }
            }
        }
    }

    private static class RawRoot {
        @SerializedName("id")
        String id;
        @SerializedName("name")
        String name;
        @SerializedName("app_sharing")
        RawAppSharing appSharing;
    }

    private static class RawAppSharing {
        @SerializedName("global")
        boolean global;
        @SerializedName("global_paid")
        boolean globalPaid;
        @SerializedName("individual")
        boolean individual;
        @SerializedName("packages")
        RawPackage[] packages;
    }

    private static class RawPackage {
        @SerializedName("pkg_id")
        String pkgId;
        @SerializedName("share_apk")
        boolean shareApk;
        @SerializedName("share_data")
        boolean shareData;
    }

    private void loadFile() throws FileNotFoundException {
        FileReader fr = null;
        JsonReader jr = null;
        try {
            fr = new FileReader(mFilename);
            jr = new JsonReader(fr);
            RawRoot root = new Gson().fromJson(jr, RawRoot.class);
            deserialize(root);
        } catch (JsonParseException e) {
            e.printStackTrace();
        } finally {
            IOUtils.closeQuietly(jr);
            IOUtils.closeQuietly(fr);
        }
    }

    private void saveFile() throws FileNotFoundException {
        File configFile = new File(mFilename);
        configFile.getParentFile().mkdirs();

        Gson gson = new Gson();

        JsonWriter writer = null;
        OutputStreamWriter osw = null;
        FileOutputStream fos = null;
        try {
            fos = new FileOutputStream(configFile);
            osw = new OutputStreamWriter(fos, Charsets.UTF_8);
            writer = new JsonWriter(osw);
            RawRoot root = serialize();
            gson.toJson(root, RawRoot.class, writer);
        } finally {
            IOUtils.closeQuietly(writer);
            IOUtils.closeQuietly(osw);
            IOUtils.closeQuietly(fos);
        }
    }
}
