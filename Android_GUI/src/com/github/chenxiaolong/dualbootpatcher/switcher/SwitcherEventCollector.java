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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import com.github.chenxiaolong.multibootpatcher.EventCollector;

public class SwitcherEventCollector extends EventCollector {
    public static final String TAG = SwitcherEventCollector.class.getSimpleName();

    private Context mContext;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(SwitcherService.STATE);

                if (SwitcherService.STATE_CHOSE_ROM.equals(state)) {
                    boolean failed = bundle.getBoolean(SwitcherService.RESULT_FAILED);
                    String kernelId = bundle.getString(SwitcherService.RESULT_KERNEL_ID);

                    sendEvent(new SwitchedRomEvent(failed, kernelId));
                } else if (SwitcherService.STATE_SET_KERNEL.equals(state)) {
                    boolean failed = bundle.getBoolean(SwitcherService.RESULT_FAILED);
                    String kernelId = bundle.getString(SwitcherService.RESULT_KERNEL_ID);

                    sendEvent(new SetKernelEvent(failed, kernelId));
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
                    SwitcherService.BROADCAST_INTENT));
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

    public void chooseRom(String kernelId) {
        Intent intent = new Intent(mContext, SwitcherService.class);
        intent.putExtra(SwitcherService.ACTION, SwitcherService.ACTION_CHOOSE_ROM);
        intent.putExtra(SwitcherService.PARAM_KERNEL_ID, kernelId);
        mContext.startService(intent);
    }

    public void setKernel(String kernelId) {
        Intent intent = new Intent(mContext, SwitcherService.class);
        intent.putExtra(SwitcherService.ACTION, SwitcherService.ACTION_SET_KERNEL);
        intent.putExtra(SwitcherService.PARAM_KERNEL_ID, kernelId);
        mContext.startService(intent);
    }

    // Events

    public class SwitchedRomEvent extends BaseEvent {
        boolean failed;
        String kernelId;

        public SwitchedRomEvent(boolean failed, String kernelId) {
            this.failed = failed;
            this.kernelId = kernelId;
        }
    }

    public class SetKernelEvent extends BaseEvent {
        boolean failed;
        String kernelId;

        public SetKernelEvent(boolean failed, String kernelId) {
            this.failed = failed;
            this.kernelId = kernelId;
        }
    }
}
