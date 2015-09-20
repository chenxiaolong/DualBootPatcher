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

package com.github.chenxiaolong.dualbootpatcher.socket;

import android.content.Context;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.BuildConfig;
import com.github.chenxiaolong.dualbootpatcher.SystemPropertiesProxy;
import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.Version.VersionParseException;

import java.io.IOException;
import java.util.HashMap;

public class MbtoolUtils {
    private static final String TAG = MbtoolUtils.class.getSimpleName();

    private static HashMap<Feature, Version> sMinVersionMap = new HashMap<>();

    static {
        if (BuildConfig.BUILD_TYPE.equals("ci")) {
            // Snapshot builds
            sMinVersionMap.put(Feature.DAEMON, Version.from("8.0.0.r1734"));
            sMinVersionMap.put(Feature.APP_SHARING, Version.from("8.0.0.r1353"));
            sMinVersionMap.put(Feature.IN_APP_INSTALLATION, Version.from("8.0.0.r1649"));
        } else {
            // Debug/release builds
            sMinVersionMap.put(Feature.DAEMON, Version.from("8.99.10"));
            sMinVersionMap.put(Feature.APP_SHARING, Version.from("8.99.8"));
            sMinVersionMap.put(Feature.IN_APP_INSTALLATION, Version.from("8.99.10"));
        }
    }

    public static Version getSystemMbtoolVersion(Context context) {
        try {
            return new Version(SystemPropertiesProxy.get(context, "ro.multiboot.version"));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return Version.from("0.0.0");
    }

    public static Version getMbtoolVersion(Context context) {
        String version = null;
        try {
            version = MbtoolSocket.getInstance().version(context);
            return new Version(version);
        } catch (VersionParseException e) {
            Log.e(TAG, "Failed to parse mbtool version: " + version);
        } catch (IOException e) {
            Log.e(TAG, "mbtool communication error", e);
        }
        return Version.from("0.0.0");
    }

    public static Version getMinimumRequiredVersion(Feature feature) {
        return sMinVersionMap.get(feature);
    }

    public enum Feature {
        DAEMON,
        APP_SHARING,
        IN_APP_INSTALLATION
    }
}
