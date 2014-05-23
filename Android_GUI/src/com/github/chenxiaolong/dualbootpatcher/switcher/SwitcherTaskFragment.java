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

public class SwitcherTaskFragment extends Fragment {
    public static final String TAG = "switcher_task";

    private ChoseRomListener mChoseRomListener;
    private SetKernelListener mSetKernelListener;
    private Context mContext;

    private boolean mSaveChoseRomFailed;
    private String mSaveChoseRomMessage;
    private String mSaveChoseRomRom;
    private boolean mChoseRom;

    private boolean mSaveSetKernelFailed;
    private String mSaveSetKernelMessage;
    private String mSaveSetKernelRom;
    private boolean mSetKernel;

    public static interface ChoseRomListener {
        public void onChoseRom(boolean failed, String message, String rom);
    }

    public static interface SetKernelListener {
        public void onSetKernel(boolean failed, String message, String rom);
    }

    public final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(SwitcherService.STATE);

                if (SwitcherService.STATE_CHOSE_ROM.equals(state)) {
                    boolean failed = bundle.getBoolean("failed");
                    String message = bundle.getString("message");
                    String rom = bundle.getString("rom");

                    onChoseRom(failed, message, rom);
                } else if (SwitcherService.STATE_SET_KERNEL.equals(state)) {
                    boolean failed = bundle.getBoolean("failed");
                    String message = bundle.getString("message");
                    String rom = bundle.getString("rom");

                    onSetKernel(failed, message, rom);
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

    public synchronized void attachChoseRomListener(ChoseRomListener listener) {
        mChoseRomListener = listener;

        // Process queued events
        if (mChoseRom) {
            mChoseRomListener.onChoseRom(mSaveChoseRomFailed,
                    mSaveChoseRomMessage, mSaveChoseRomRom);
            mChoseRom = false;
        }
    }

    public synchronized void attachSetKernelListener(SetKernelListener listener) {
        mSetKernelListener = listener;

        // Process queued events
        if (mSetKernel) {
            mSetKernelListener.onSetKernel(mSaveSetKernelFailed,
                    mSaveSetKernelMessage, mSaveSetKernelRom);
            mSetKernel = false;
        }
    }

    public synchronized void detachChoseRomListener() {
        mChoseRomListener = null;
    }

    public synchronized void detachSetKernelListener() {
        mSetKernelListener = null;
    }

    private synchronized void onChoseRom(boolean failed, String message,
            String rom) {
        if (mChoseRomListener != null) {
            mChoseRomListener.onChoseRom(failed, message, rom);
        } else {
            mChoseRom = true;
            mSaveChoseRomFailed = failed;
            mSaveChoseRomMessage = message;
            mSaveChoseRomRom = rom;
        }
    }

    private synchronized void onSetKernel(boolean failed, String message,
            String rom) {
        if (mSetKernelListener != null) {
            mSetKernelListener.onSetKernel(failed, message, rom);
        } else {
            mSetKernel = true;
            mSaveSetKernelFailed = failed;
            mSaveSetKernelMessage = message;
            mSaveSetKernelRom = rom;
        }
    }
}
