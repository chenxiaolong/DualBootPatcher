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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.content.Context;
import android.support.v7.widget.CardView;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatchInfo;

import java.util.ArrayList;

import it.gmariotti.cardslib.library.view.CardViewNative;

public class PresetCW {
    protected static interface PresetListener {
        public void onPresetSelected(PatchInfo info);
    }

    private CardView vCard;
    private TextView vTitle;
    private Spinner vPresetSpinner;
    private TextView vPresetName;

    private ArrayAdapter<String> mPresetAdapter;
    private ArrayList<String> mPresets = new ArrayList<>();

    private Context mContext;
    private PatcherConfigState mPCS;
    private PresetListener mListener;

    public PresetCW(Context context, PatcherConfigState pcs, CardView card,
                    PresetListener listener) {
        mContext = context;
        mPCS = pcs;
        mListener = listener;

        vCard = card;
        vTitle = (TextView) card.findViewById(R.id.card_title);
        vPresetSpinner = (Spinner) card.findViewById(R.id.spinner_preset);
        vPresetName = (TextView) card.findViewById(R.id.preset_name);

        initPresets();
    }

    private void initPresets() {
        mPresetAdapter = new ArrayAdapter<String>(mContext, android.R.layout.simple_spinner_item,
                android.R.id.text1, mPresets);
        mPresetAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        vPresetSpinner.setAdapter(mPresetAdapter);

        vPresetSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                updatePresetDescription(position);

                if (mListener != null) {
                    mListener.onPresetSelected(getPreset());
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    public void refreshPresets() {
        mPresets.clear();

        // Default to using custom options
        mPresets.add(mContext.getString(R.string.preset_custom));
        for (PatchInfo info : mPCS.mPatchInfos) {
            mPresets.add(info.getId());
        }

        mPresetAdapter.notifyDataSetChanged();
    }

    private void updatePresetDescription(int position) {
        if (position == 0) {
            vPresetName.setText(mContext.getString(R.string.preset_custom_desc));
        } else {
            vPresetName.setText(mPCS.mPatchInfos[position - 1].getName());
        }
    }

    public PatchInfo getPreset() {
        int pos = vPresetSpinner.getSelectedItemPosition();

        if (pos == 0) {
            // Custom options
            return null;
        } else {
            return mPCS.mPatchInfos[pos - 1];
        }
    }

    public void setEnabled(boolean enabled) {
        vTitle.setEnabled(enabled);
        vPresetSpinner.setEnabled(enabled);
        vPresetName.setEnabled(enabled);
    }

    public void reset() {
        vPresetSpinner.setSelection(0);
    }

    public void refreshState() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_PATCHING:
            setEnabled(false);

            if ((mPCS.mSupported & PatcherConfigState.SUPPORTED_FILE) != 0) {
                vCard.setVisibility(View.GONE);
            } else {
                vCard.setVisibility(View.VISIBLE);
            }

            break;

        case PatcherConfigState.STATE_CHOSE_FILE:
            setEnabled(true);

            if ((mPCS.mSupported & PatcherConfigState.SUPPORTED_FILE) != 0) {
                vCard.setVisibility(View.GONE);
            } else {
                vCard.setVisibility(View.VISIBLE);
            }

            break;

        case PatcherConfigState.STATE_INITIAL:
        case PatcherConfigState.STATE_FINISHED:
            setEnabled(true);

            vCard.setVisibility(View.GONE);

            break;
        }
    }
}
