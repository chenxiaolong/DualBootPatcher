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

import java.util.ArrayList;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.PatcherInformation;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation.Device;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation.FileInfo;
import com.github.chenxiaolong.dualbootpatcher.R;

public class PresetCard extends Card {
    private TextView mTitle;
    Spinner mPresetSpinner;
    ArrayAdapter<String> mPresetAdapter;
    private TextView mPresetName;

    private PatcherInformation mInfo;
    private final ArrayList<FileInfo> mPresets = new ArrayList<FileInfo>();

    public PresetCard(Context context) {
        this(context, R.layout.cardcontent_preset);
    }

    public PresetCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mTitle = (TextView) view.findViewById(R.id.card_title);
            mPresetSpinner = (Spinner) view.findViewById(R.id.spinner_preset);
            mPresetName = (TextView) view.findViewById(R.id.preset_name);
        }
    }

    private void populatePresets(Device device) {
        mPresetAdapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        mPresetAdapter
                .setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPresetSpinner.setAdapter(mPresetAdapter);

        reloadPresets(device);
    }

    public void reloadPresets(Device device) {
        mPresetAdapter.clear();
        mPresets.clear();

        // Default to using custom options
        mPresetAdapter.add(getContext().getString(R.string.preset_custom));

        // Only add valid presets for the device
        String[] dirs = getContext().getResources().getStringArray(
                R.array.preset_dirs);

        for (int i = 0; i < mInfo.mFileInfos.length; i++) {
            PatcherInformation.FileInfo info = mInfo.mFileInfos[i];

            if (info.mPath.startsWith(device.mCodeName + "/")) {
                mPresetAdapter.add(info.mPath);
                mPresets.add(info);
                continue;
            }

            for (String dir : dirs) {
                if (info.mPath.startsWith(dir + "/")) {
                    mPresetAdapter.add(info.mPath);
                    mPresets.add(info);
                    continue;
                }
            }
        }
        mPresetAdapter.notifyDataSetChanged();
    }

    void updatePresetDescription(int position) {
        if (position == 0) {
            mPresetName.setText(getContext().getString(
                    R.string.preset_custom_desc));
        } else {
            // mPresetName.setText(mInfo.mFileInfos[position - 1].mName);
            mPresetName.setText(mPresets.get(position - 1).mName);
        }
    }

    public void setPatcherInfo(PatcherInformation info, Device device) {
        mInfo = info;

        populatePresets(device);
    }

    public FileInfo getPreset() {
        int pos = mPresetSpinner.getSelectedItemPosition();

        if (pos == 0) {
            // Custom options
            return null;
        } else {
            // return mInfo.mFileInfos[pos - 1];
            return mPresets.get(pos - 1);
        }
    }

    public void setEnabled(boolean enabled) {
        mTitle.setEnabled(enabled);
        mPresetSpinner.setEnabled(enabled);
        mPresetName.setEnabled(enabled);
    }

    public void reset() {
        mPresetSpinner.setSelection(0);
    }
}
