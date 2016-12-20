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

import android.os.Environment;
import android.util.Log;

import java.io.File;
import java.io.IOException;

public final class LogUtils {
    private static final String TAG = LogUtils.class.getSimpleName();

    public static String getPath(String logFile) {
        String fileName = new File(logFile).getName();

        return Environment.getExternalStorageDirectory()
                + File.separator + "MultiBoot"
                + File.separator + "logs"
                + File.separator + fileName;
    }

    public static void dump(String logFile) {
        final File path = new File(getPath(logFile));
        path.getParentFile().mkdirs();
        try {
            Log.v(TAG, "Dumping logcat to " + path);
            Runtime.getRuntime().exec("logcat -d -v threadtime -f " + path + " *").waitFor();
        } catch (InterruptedException e) {
            Log.e(TAG, "Failed to wait for logcat to exit", e);
        } catch (IOException e) {
            Log.e(TAG, "Failed to run logcat", e);
        }
    }
}
