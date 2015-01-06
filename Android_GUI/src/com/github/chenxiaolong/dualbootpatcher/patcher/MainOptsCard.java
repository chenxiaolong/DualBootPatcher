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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Device;

import java.util.ArrayList;

import it.gmariotti.cardslib.library.internal.Card;

public class MainOptsCard extends Card {
    public static interface MainOptsSelectedListener {
        public void onPatcherSelected(String patcherName);

        public void onDeviceSelected(Device device);
    }

    private PatcherConfigState mPCS;
    private MainOptsSelectedListener mListener;

    private TextView mTitle;
    private ArrayAdapter<String> mPatcherAdapter;
    private Spinner mPatcherSpinner;
    private ArrayAdapter<String> mDeviceAdapter;
    private Spinner mDeviceSpinner;

    public MainOptsCard(Context context, PatcherConfigState pcs,
                        MainOptsSelectedListener listener) {
        this(context, R.layout.cardcontent_mainopts);
        mPCS = pcs;
        mListener = listener;
    }

    public MainOptsCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mTitle = (TextView) view.findViewById(R.id.card_title);
            mPatcherSpinner = (Spinner) view.findViewById(R.id.spinner_patcher);
            mDeviceSpinner = (Spinner) view.findViewById(R.id.spinner_device);
        }

        initControls();
    }

    private void initControls() {
        // Patchers

        mPatcherAdapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        mPatcherAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPatcherSpinner.setAdapter(mPatcherAdapter);

        mPatcherSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (mListener != null) {
                    mListener.onPatcherSelected(mPatcherSpinner.getSelectedItem().toString());
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        // Devices

        mDeviceAdapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        mDeviceAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mDeviceSpinner.setAdapter(mDeviceAdapter);

        mDeviceSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
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

    public void refreshPatchers() {
        ArrayList<String> patchers = new ArrayList<String>();
        for (String patcherId : PatcherUtils.sPC.getPatchers()) {
            patchers.add(PatcherUtils.sPC.getPatcherName(patcherId));
        }

        mPatcherAdapter.addAll(patchers.toArray(new String[0]));
        mPatcherAdapter.notifyDataSetChanged();
    }

    public void refreshDevices() {
        for (Device device : PatcherUtils.sPC.getDevices()) {
            String text = String.format("%s (%s)", device.getCodename(), device.getName());
            mDeviceAdapter.add(text);
        }
        mDeviceAdapter.notifyDataSetChanged();
    }

    public Device getDevice() {
        int position = mDeviceSpinner.getSelectedItemPosition();
        if (position < 0) {
            position = 0;
        }
        return PatcherUtils.sPC.getDevices()[position];
    }

    public void setEnabled(boolean enabled) {
        mTitle.setEnabled(enabled);
        mPatcherSpinner.setEnabled(enabled);
        mDeviceSpinner.setEnabled(enabled);
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
