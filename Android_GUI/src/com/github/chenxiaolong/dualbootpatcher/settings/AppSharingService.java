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

import android.app.IntentService;
import android.content.Intent;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

public class AppSharingService extends IntentService {
    private static final String TAG = AppSharingService.class.getSimpleName();
    public static final String BROADCAST_INTENT =
            "com.chenxiaolong.github.multibootpatcher.BROADCAST_APP_SHARING_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_PACKAGE_REMOVED = "package_removed";

    public static final String STATE = "state";

    public static final String RESULT_ROMINFO = "rominfo";
    public static final String RESULT_VERSION = "version";

    public static final String EXTRA_PACKAGE = "package";

    public AppSharingService() {
        super(TAG);
    }

    private void onPackageRemoved(String pkg) {
        RomInformation info = RomUtils.getCurrentRom(AppSharingService.this);

        if (info == null) {
            Log.e(TAG, "Failed to determine current ROM. App sharing status was NOT updated");
            return;
        }

        // Unsync package if explicitly removed
        //if (config.isRomSynced(pkg, info)) {
            //config.setRomSynced(pkg, info, false);
        //}

        //config.save();
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_PACKAGE_REMOVED.equals(action)) {
            onPackageRemoved(intent.getStringExtra(EXTRA_PACKAGE));
        }
    }
}
