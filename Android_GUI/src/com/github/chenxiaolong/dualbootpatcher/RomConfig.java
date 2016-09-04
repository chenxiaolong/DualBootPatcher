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

package com.github.chenxiaolong.dualbootpatcher;

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
import java.util.HashMap;

public class RomConfig {
    private static final String TAG = RomConfig.class.getSimpleName();

    private static HashMap<String, RomConfig> sInstances = new HashMap<>();

    private String mFilename;

    private String mId;
    private String mName;

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

    private RawRoot serialize() {
        RawRoot root = new RawRoot();

        root.id = mId;
        root.name = mName;

        return root;
    }

    private void deserialize(RawRoot root) {
        if (root == null) {
            return;
        }

        mId = root.id;
        mName = root.name;
    }

    private static class RawRoot {
        @SerializedName("id")
        String id;
        @SerializedName("name")
        String name;
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
            writer.setIndent("    ");
            RawRoot root = serialize();
            gson.toJson(root, RawRoot.class, writer);
        } finally {
            IOUtils.closeQuietly(writer);
            IOUtils.closeQuietly(osw);
            IOUtils.closeQuietly(fos);
        }
    }
}
