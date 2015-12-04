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

import android.Manifest;
import android.app.Fragment;
import android.content.Context;
import android.content.pm.PackageManager;
import android.support.annotation.NonNull;
import android.support.v13.app.FragmentCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;

import java.util.Collections;
import java.util.HashSet;

public class PermissionUtils {
    private static final boolean DEBUG = true;
    private static final String TAG = PermissionUtils.class.getSimpleName();

    /** Permissions we have already attempted requesting */
    private static final HashSet<String> sRequested = new HashSet<>();

    /**
     * Storage permissions for reading and writing to the internal storage
     */
    public static final String STORAGE_PERMISSIONS[] = new String[]{
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };

    /**
     * Check if the user has allowed a list of permissions (for Android 6.0+)
     *
     * @param context Context
     * @param permissions Permissions to check
     * @return
     */
    public static boolean hasPermissions(@NonNull Context context, @NonNull String[] permissions) {
        for (String permission : permissions) {
            if (ContextCompat.checkSelfPermission(context, permission)
                    != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    /**
     * Request permissions (for Android 6.0+)
     *
     * The permissions dialog will be shown if all the following are true:
     * - Permissions have not been requested before (so the permissions dialog will only be shown
     *   once)
     * - Permissions are not already granted
     *
     * @param context Context (needed to check if permissions have already been granted)
     * @param fragment Parent fragment
     * @param permissions List of permissions to request
     * @param requestCode Request code for use in
     *                    {@link Fragment#onRequestPermissionsResult(int, String[], int[])}
     */
    public static void requestPermissions(@NonNull Context context, @NonNull Fragment fragment,
                                          @NonNull String[] permissions, int requestCode) {
        if (DEBUG) {
            Log.d(TAG, "Requesting permissions:");
            for (String permission : permissions) {
                Log.d(TAG, "- " + permission);
            }
        }

        // Check if we've already requested these permissions before
        if (hasBeenRequestedBefore(permissions)) {
            if (DEBUG) Log.d(TAG, "Permissions have been requested before. Skipping request");
            return;
        }

        if (!hasPermissions(context, permissions)) {
            if (DEBUG) Log.d(TAG, "Permissions have not been granted. Requesting permissions");

            FragmentCompat.requestPermissions(fragment, permissions, requestCode);

            Collections.addAll(sRequested, permissions);
        }
    }

    /**
     * Clear list of permissions that have been requested
     *
     * Allows the permission dialog to be shown again if the user has rejected the permission
     * before.
     */
    public static void clearRequestedCache() {
        sRequested.clear();
    }

    /**
     * Check if a list of permissions have been requested before
     *
     * @param permissions List of permissions
     * @return Whether the permissions have been requested before
     */
    private static boolean hasBeenRequestedBefore(String[] permissions) {
        for (String permission : permissions) {
            if (!sRequested.contains(permission)) {
                return false;
            }
        }
        return true;
    }
}
