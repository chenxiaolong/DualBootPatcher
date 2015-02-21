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

package com.github.chenxiaolong.multibootpatcher.settings;

import android.app.IntentService;
import android.content.Intent;

import com.github.chenxiaolong.dualbootpatcher.settings.AppSharingUtils;

public class RomSettingsService extends IntentService {
    private static final String TAG = RomSettingsService.class.getSimpleName();
    public static final String BROADCAST_INTENT =
            "com.chenxiaolong.github.multibootpatcher.BROADCAST_SETTINGS_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_UPDATE_RAMDISK = "update_ramdisk";

    public static final String STATE = "state";
    public static final String STATE_UPDATED_RAMDISK = "updated_ramdisk";

    public static final String RESULT_SUCCESS = "success";

    public RomSettingsService() {
        super(TAG);
    }

    private void updateRamdisk() {
        boolean success = AppSharingUtils.updateRamdisk(this);

        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATED_RAMDISK);
        i.putExtra(RESULT_SUCCESS, success);
        sendBroadcast(i);
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_UPDATE_RAMDISK.equals(action)) {
            updateRamdisk();
        }
    }
}
