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

import android.app.Activity;
import android.app.FragmentManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.EventCollector;

public class RomSettingsEventCollector extends EventCollector {
    public static final String TAG = RomSettingsEventCollector.class.getSimpleName();

    private Context mContext;

    public static RomSettingsEventCollector getInstance(FragmentManager fm) {
        RomSettingsEventCollector rsec = (RomSettingsEventCollector) fm.findFragmentByTag(TAG);
        if (rsec == null) {
            rsec = new RomSettingsEventCollector();
            fm.beginTransaction().add(rsec, RomSettingsEventCollector.TAG).commit();
        }
        return rsec;
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(RomSettingsService.STATE);

                if (RomSettingsService.STATE_UPDATED_RAMDISK.equals(state)) {
                    boolean success = bundle.getBoolean(RomSettingsService.RESULT_SUCCESS);
                    sendEvent(new UpdatedRamdiskEvent(success));
                }
            }
        }
    };

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        if (mContext == null) {
            mContext = getActivity().getApplicationContext();
            mContext.registerReceiver(mReceiver, new IntentFilter(
                    RomSettingsService.BROADCAST_INTENT));
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mContext != null) {
            mContext.unregisterReceiver(mReceiver);
        }
    }

    // Start tasks

    public void updateRamdisk() {
        Intent intent = new Intent(mContext, RomSettingsService.class);
        intent.putExtra(RomSettingsService.ACTION, RomSettingsService.ACTION_UPDATE_RAMDISK);
        mContext.startService(intent);
    }

    // Events

    public class UpdatedRamdiskEvent extends BaseEvent {
        boolean success;

        public UpdatedRamdiskEvent(boolean success) {
            this.success = success;
        }
    }
}
