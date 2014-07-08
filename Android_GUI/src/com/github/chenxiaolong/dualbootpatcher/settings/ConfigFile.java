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

package com.github.chenxiaolong.dualbootpatcher.settings;

import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.RootFile;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;

public class ConfigFile {
    private static final String CONFIG_FILE = "syncdaemon.json";
    private static final String CONF_VERSION = "jsonversion";
    private static final String CONF_PACKAGES = "packages";
    private static final String CONF_SYNC_ACROSS = "syncacross";
    private static final String CONF_SHARE_DATA = "sharedata";

    private int mVersion;
    private boolean mChanged;

    private ArrayList<SharedPackage> mPackages = new ArrayList<SharedPackage>();

    private class SharedPackage {
        public String name;
        public boolean dataShared;
        public ArrayList<String> romIds = new ArrayList<String>();
    }

    public ConfigFile() {
        loadConfig();
    }

    // Ouch, that grammar ...
    public static boolean isExistsConfigFile() {
        File f = new File(getConfigFile());
        return f.exists();
    }

    private static String getConfigFile() {
        // Avoid root for this (not using FileUtils.isExistsDirectory())
        File d = new File(RomUtils.RAW_DATA);

        if (d.exists() && d.isDirectory()) {
            return RomUtils.RAW_DATA + File.separator + CONFIG_FILE;
        } else {
            return RomUtils.DATA + File.separator + CONFIG_FILE;
        }
    }

    private void loadConfig() {
        // Only load once
        if (mPackages.size() == 0) {
            mPackages.clear();
            mVersion = 0;

            String contents = new RootFile(getConfigFile()).getContents();
            if (contents != null) {
                try {
                    JSONObject root = new JSONObject(contents);
                    mVersion = root.getInt(CONF_VERSION);

                    switch (mVersion) {
                        case 1:
                            parseVersion1(root);
                            break;
                    }
                } catch (JSONException e) {
                    e.printStackTrace();
                }
            }
        }
    }

    public void save() {
        if (mChanged) {
            if (mPackages.size() > 0) {
                switch (mVersion) {
                    case 0:
                    case 1:
                        new RootFile(getConfigFile()).writeContents(createVersion1());
                        break;
                }
            } else {
                new RootFile(getConfigFile()).delete();
            }

            mChanged = false;
        }
    }

    public boolean isRomSynced(String pkg, RomInformation info) {
        for (SharedPackage pkginfo : mPackages) {
            if (pkginfo.name.equals(pkg)) {
                for (int j = 0; j < pkginfo.romIds.size(); j++) {
                    if (pkginfo.romIds.get(j).equals(info.id)) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    public void setRomSynced(String pkg, RomInformation info, boolean sync) {
        mChanged = true;

        SharedPackage pkginfo = null;

        for (SharedPackage curpkginfo : mPackages) {
            if (curpkginfo.name.equals(pkg)) {
                pkginfo = curpkginfo;
                break;
            }
        }

        if (pkginfo == null) {
            pkginfo = new SharedPackage();
            pkginfo.name = pkg;
            if (sync) {
                pkginfo.romIds.add(info.id);
            }
            mPackages.add(pkginfo);
        } else {
            int found = -1;

            for (int i = 0; i < pkginfo.romIds.size(); i++) {
                if (pkginfo.romIds.get(i).equals(info.id)) {
                    found = i;
                    break;
                }
            }

            if (sync && found == -1) {
                pkginfo.romIds.add(info.id);
            } else if (!sync && found != -1) {
                pkginfo.romIds.remove(found);
            }
        }
    }

    public boolean isDataShared(String pkg) {
        SharedPackage pkginfo = null;

        for (SharedPackage curpkginfo : mPackages) {
            if (curpkginfo.name.equals(pkg)) {
                pkginfo = curpkginfo;
                break;
            }
        }

        return pkginfo != null && pkginfo.dataShared;

    }

    private void parseVersion1(JSONObject root) {
        if (root == null) {
            return;
        }

        try {
            if (!root.has(CONF_PACKAGES)) {
                return;
            }

            JSONObject pkgs = root.getJSONObject(CONF_PACKAGES);

            Iterator<String> iter = (Iterator<String>) pkgs.keys();
            while (iter.hasNext()) {
                SharedPackage pkginfo = new SharedPackage();

                String key = iter.next();
                JSONObject pkg = pkgs.getJSONObject(key);

                pkginfo.name = key;

                if (!pkg.has(CONF_SYNC_ACROSS)) {
                    continue;
                }

                JSONArray syncacross = pkg.getJSONArray(CONF_SYNC_ACROSS);

                for (int i = 0; i < syncacross.length(); i++) {
                    pkginfo.romIds.add(syncacross.getString(i));
                }

                if (pkg.has(CONF_SHARE_DATA)) {
                    pkginfo.dataShared = pkg.getBoolean(CONF_SHARE_DATA);
                }

                mPackages.add(pkginfo);
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    private String createVersion1() {
        try {
            JSONObject root = new JSONObject();
            root.put(CONF_VERSION, 1);

            JSONObject pkgs = new JSONObject();
            root.put(CONF_PACKAGES, pkgs);

            for (SharedPackage pkginfo : mPackages) {
                // Not much point in syncing across one rom...
                if (pkginfo.romIds.size() < 2) {
                    continue;
                }

                JSONObject pkg = new JSONObject();
                pkg.put(CONF_SHARE_DATA, pkginfo.dataShared);

                JSONArray syncacross = new JSONArray();
                for (String romId : pkginfo.romIds) {
                    syncacross.put(romId);
                }
                pkg.put(CONF_SYNC_ACROSS, syncacross);

                pkgs.put(pkginfo.name, pkg);
            }

            return root.toString(4);
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return null;
    }
}
