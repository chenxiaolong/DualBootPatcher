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

import it.gmariotti.cardslib.library.internal.Card;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.PatcherInformation;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation.Device;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation.PartitionConfig;
import com.github.chenxiaolong.dualbootpatcher.R;

public class DevicePartConfigCard extends Card {
    private TextView mTitle;
    Spinner mDeviceSpinner;
    private Spinner mPartConfigSpinner;
    private TextView mPartConfigDesc;

    private PatcherInformation mInfo;

    public DevicePartConfigCard(Context context) {
        this(context, R.layout.cardcontent_device_partconfig);
    }

    public DevicePartConfigCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mTitle = (TextView) view.findViewById(R.id.card_title);
            mDeviceSpinner = (Spinner) view.findViewById(R.id.spinner_device);
            mPartConfigSpinner = (Spinner) view
                    .findViewById(R.id.spinner_partconfig);
            mPartConfigDesc = (TextView) view
                    .findViewById(R.id.partconfig_desc);
        }
    }

    private void populateDevices() {
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mDeviceSpinner.setAdapter(adapter);

        for (int i = 0; i < mInfo.mDevices.length; i++) {
            PatcherInformation.Device device = mInfo.mDevices[i];
            String text = String.format("%s (%s)", device.mCodeName,
                    device.mName);
            adapter.add(text);
        }
        adapter.notifyDataSetChanged();
    }

    private void populatePartConfigs() {
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPartConfigSpinner.setAdapter(adapter);

        for (int i = 0; i < mInfo.mPartitionConfigs.length; i++) {
            adapter.add(mInfo.mPartitionConfigs[i].mName);
        }
        adapter.notifyDataSetChanged();

        mPartConfigSpinner
                .setOnItemSelectedListener(new OnItemSelectedListener() {
                    @Override
                    public void onItemSelected(AdapterView<?> parent,
                            View view, int position, long id) {
                        PartitionConfig config = mInfo.mPartitionConfigs[position];
                        mPartConfigDesc.setText(config.mDesc);
                    }

                    @Override
                    public void onNothingSelected(AdapterView<?> parent) {
                    }
                });
    }

    public void setPatcherInfo(PatcherInformation info) {
        mInfo = info;

        populateDevices();
        populatePartConfigs();
    }

    public Device getDevice() {
        int position = mDeviceSpinner.getSelectedItemPosition();
        if (position < 0) {
            position = 0;
        }
        return mInfo.mDevices[position];
    }

    public PartitionConfig getPartConfig() {
        int position = mPartConfigSpinner.getSelectedItemPosition();
        if (position < 0) {
            position = 0;
        }
        return mInfo.mPartitionConfigs[position];
    }

    public void setEnabled(boolean enabled) {
        mTitle.setEnabled(enabled);
        mDeviceSpinner.setEnabled(enabled);
        mPartConfigSpinner.setEnabled(enabled);
        mPartConfigDesc.setEnabled(enabled);
    }
}
