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

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.EventCollector;

public class AppSharingEventCollector extends EventCollector {
    public static final String TAG = AppSharingEventCollector.class.getSimpleName();

    private Context mContext;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(AppSharingService.STATE);

                if (AppSharingService.STATE_UPDATED_RAMDISK.equals(state)) {
                    boolean failed = bundle.getBoolean(AppSharingService.RESULT_FAILED);

                    sendEvent(new UpdatedRamdiskEvent(failed));
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
                    AppSharingService.BROADCAST_INTENT));
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
        Intent intent = new Intent(mContext, AppSharingService.class);
        intent.putExtra(AppSharingService.ACTION, AppSharingService.ACTION_UPDATE_RAMDISK);
        mContext.startService(intent);
    }

    // Events

    public class UpdatedRamdiskEvent extends BaseEvent {
        boolean failed;

        public UpdatedRamdiskEvent(boolean failed) {
            this.failed = failed;
        }
    }
}
