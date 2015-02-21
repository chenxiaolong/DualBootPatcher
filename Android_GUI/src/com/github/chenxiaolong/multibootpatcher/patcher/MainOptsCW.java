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

package com.github.chenxiaolong.multibootpatcher.patcher;

import android.content.Context;
import android.support.v7.widget.CardView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Device;

import java.util.ArrayList;

public class MainOptsCW {
    protected static interface MainOptsListener {
        public void onDeviceSelected(Device device);
    }

    private TextView vTitle;
    private Spinner vDeviceSpinner;

    private Context mContext;
    private PatcherConfigState mPCS;
    private MainOptsListener mListener;

    private ArrayAdapter<String> mDeviceAdapter;
    private ArrayList<String> mDevices = new ArrayList<>();

    public MainOptsCW(Context context, PatcherConfigState pcs, CardView card,
                      MainOptsListener listener) {
        mContext = context;
        mPCS = pcs;
        mListener = listener;
        vTitle = (TextView) card.findViewById(R.id.card_title);
        vDeviceSpinner = (Spinner) card.findViewById(R.id.spinner_device);

        initDevices();
    }

    private void initDevices() {
        mDeviceAdapter = new ArrayAdapter<>(mContext,
                android.R.layout.simple_spinner_item, android.R.id.text1, mDevices);
        mDeviceAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        vDeviceSpinner.setAdapter(mDeviceAdapter);

        vDeviceSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (mListener != null) {
                    mListener.onDeviceSelected(PatcherUtils.sPC.getDevices()[position]);
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    public void refreshDevices() {
        mDevices.clear();
        for (Device device : PatcherUtils.sPC.getDevices()) {
            mDevices.add(String.format("%s (%s)", device.getId(), device.getName()));
        }
        mDeviceAdapter.notifyDataSetChanged();
    }

    public Device getDevice() {
        int position = vDeviceSpinner.getSelectedItemPosition();
        if (position < 0) {
            position = 0;
        }
        return PatcherUtils.sPC.getDevices()[position];
    }

    public void setEnabled(boolean enabled) {
        vTitle.setEnabled(enabled);
        vDeviceSpinner.setEnabled(enabled);
    }

    public void refreshState() {
        // Disable when patching
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_PATCHING:
            setEnabled(false);
            break;

        case PatcherConfigState.STATE_INITIAL:
        case PatcherConfigState.STATE_CHOSE_FILE:
        case PatcherConfigState.STATE_FINISHED:
            setEnabled(true);
            break;
        }
    }
}