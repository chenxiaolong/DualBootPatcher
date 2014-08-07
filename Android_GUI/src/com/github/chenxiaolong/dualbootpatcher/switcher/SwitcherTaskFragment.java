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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.app.Activity;
import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import com.squareup.otto.Bus;

public class SwitcherTaskFragment extends Fragment {
    public static final String TAG = SwitcherTaskFragment.class.getSimpleName();

    private Context mContext;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(SwitcherService.STATE);

                if (SwitcherService.STATE_CHOSE_ROM.equals(state)) {
                    boolean failed = bundle.getBoolean(SwitcherService.RESULT_FAILED);
                    String message = bundle.getString(SwitcherService.RESULT_MESSAGE);
                    String kernelId = bundle.getString(SwitcherService.RESULT_KERNEL_ID);

                    OnChoseRomEvent event = new OnChoseRomEvent();
                    event.failed = failed;
                    event.message = message;
                    event.kernelId = kernelId;
                    getBusInstance().post(event);
                } else if (SwitcherService.STATE_SET_KERNEL.equals(state)) {
                    boolean failed = bundle.getBoolean(SwitcherService.RESULT_FAILED);
                    String message = bundle.getString(SwitcherService.RESULT_MESSAGE);
                    String kernelId = bundle.getString(SwitcherService.RESULT_KERNEL_ID);

                    OnSetKernelEvent event = new OnSetKernelEvent();
                    event.failed = failed;
                    event.message = message;
                    event.kernelId = kernelId;
                    getBusInstance().post(event);
                }
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setRetainInstance(true);
    }

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

    // Otto bus

    private static final Bus BUS = new Bus();

    public static Bus getBusInstance() {
        return BUS;
    }

    // Events

    public static class SwitcherTaskEvent {
    }

    public static class OnChoseRomEvent extends SwitcherTaskEvent {
        boolean failed;
        String message;
        String kernelId;
    }

    public static class OnSetKernelEvent extends SwitcherTaskEvent {
        boolean failed;
        String message;
        String kernelId;
    }

    // Task starters

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
}
