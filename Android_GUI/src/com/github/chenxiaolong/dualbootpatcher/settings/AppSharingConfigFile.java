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

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Iterator;

public class AppSharingConfigFile extends ConfigFile {
    private static final String CONFIG_FILE = "syncdaemon.json";
    private static final String CONF_PACKAGES = "packages";
    private static final String CONF_SYNC_ACROSS = "syncacross";
    private static final String CONF_SHARE_DATA = "sharedata";

    private static AppSharingConfigFile sInstance;

    private ArrayList<SharedPackage> mPackages;

    private class SharedPackage {
        public String name;
        public boolean dataShared;
        public ArrayList<String> romIds = new ArrayList<String>();
    }

    public static AppSharingConfigFile getInstance() {
        if (sInstance == null) {
            sInstance = new AppSharingConfigFile();
        }

        return sInstance;
    }

    private AppSharingConfigFile() {
        super(CONFIG_FILE);
    }

    @Override
    protected void onLoadedVersion(int version, JSONObject root) {
        if (root == null) {
            return;
        }

        mPackages = new ArrayList<SharedPackage>();

        switch (version) {
        case 1:
            parseVersion1(root);
            break;
        }
    }

    @Override
    protected void onSaveVersion(int version) {
        if (mPackages.size() == 0) {
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
        setChanged(true);

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
            JSONObject root = getEmptyVersionedRoot(1);

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
