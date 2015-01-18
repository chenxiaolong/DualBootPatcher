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

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Iterator;

public class RomInfoConfigFile extends ConfigFile {
    private static final String CONFIG_FILE = "rominfo.json";
    private static final String CONF_ROMS = "roms";
    private static final String CONF_NAME = "name";

    private static RomInfoConfigFile sInstance;

    private ArrayList<RomConfig> mRoms;

    private class RomConfig {
        public String id;
        public String name;
    }

    public static RomInfoConfigFile getInstance() {
        if (sInstance == null) {
            sInstance = new RomInfoConfigFile();
        }

        return sInstance;
    }

    private RomInfoConfigFile() {
        super(CONFIG_FILE);
    }

    @Override
    protected void onLoadedVersion(int version, JSONObject root) {
        mRoms = new ArrayList<RomConfig>();

        if (root == null) {
            return;
        }

        switch (version) {
        case 1:
            parseVersion1(root);
            break;
        }
    }

    @Override
    protected void onSaveVersion(int version) {
        if (mRoms.size() == 0) {
            delete();
            return;
        }

        switch (version) {
        case 0:
        case 1:
            write(1, createVersion1());
            break;
        }
    }

    public synchronized String getRomName(RomInformation info) {
        for (RomConfig rom : mRoms) {
            if (rom.id.equals(info.id)) {
                return rom.name;
            }
        }

        return null;
    }

    public synchronized void setRomName(RomInformation info, String name) {
        setChanged(true);

        RomConfig rom = null;

        for (RomConfig curRom : mRoms) {
            if (curRom.id.equals(info.id)) {
                rom = curRom;
                break;
            }
        }

        // Remove if name is null
        if (name == null && rom != null) {
            mRoms.remove(rom);
            return;
        }

        if (rom == null) {
            rom = new RomConfig();
            rom.id = info.id;
            rom.name = name;
            mRoms.add(rom);
        } else {
            rom.name = name;
        }
    }

    private void parseVersion1(JSONObject root) {
        if (root == null) {
            return;
        }

        try {
            if (!root.has(CONF_ROMS)) {
                return;
            }

            JSONObject roms = root.getJSONObject(CONF_ROMS);

            Iterator<String> iter = (Iterator<String>) roms.keys();
            while (iter.hasNext()) {
                RomConfig rom = new RomConfig();

                String key = iter.next();
                JSONObject pkg = roms.getJSONObject(key);

                rom.id = key;

                if (!pkg.has(CONF_NAME)) {
                    continue;
                }

                rom.name = pkg.getString(CONF_NAME);

                mRoms.add(rom);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    private String createVersion1() {
        try {
            JSONObject root = getEmptyVersionedRoot(1);

            JSONObject roms = new JSONObject();
            root.put(CONF_ROMS, roms);

            for (RomConfig rom : mRoms) {
                JSONObject romObj = new JSONObject();
                romObj.put(CONF_NAME, rom.name);

                roms.put(rom.id, romObj);
            }

            return root.toString(4);
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return null;
    }
}
