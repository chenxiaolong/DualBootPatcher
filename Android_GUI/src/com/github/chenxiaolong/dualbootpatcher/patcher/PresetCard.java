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
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatchInfo;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.view.CardViewNative;

public class PresetCard extends Card {
    public static interface PresetSelectedListener {
        public void onPresetSelected(PatchInfo info);
    }

    private PatcherConfigState mPCS;
    private PresetSelectedListener mListener;

    private TextView mTitle;
    private Spinner mPresetSpinner;
    private ArrayAdapter<String> mPresetAdapter;
    private TextView mPresetName;

    public PresetCard(Context context, PatcherConfigState pcs, PresetSelectedListener listener) {
        this(context, R.layout.card_inner_layout_preset);
        mPCS = pcs;
        mListener = listener;
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

        initControls();
    }

    private void initControls() {
        mPresetAdapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        mPresetAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mPresetSpinner.setAdapter(mPresetAdapter);

        mPresetSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
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
        mPresetAdapter.clear();

        // Default to using custom options
        mPresetAdapter.add(getContext().getString(R.string.preset_custom));

        for (PatchInfo info : mPCS.mPatchInfos) {
            mPresetAdapter.add(info.getId());
        }

        mPresetAdapter.notifyDataSetChanged();
    }

    private void updatePresetDescription(int position) {
        if (position == 0) {
            mPresetName.setText(getContext().getString(R.string.preset_custom_desc));
        } else {
            mPresetName.setText(mPCS.mPatchInfos[position - 1].getName());
        }
    }

    public PatchInfo getPreset() {
        int pos = mPresetSpinner.getSelectedItemPosition();

        if (pos == 0) {
            // Custom options
            return null;
        } else {
            return mPCS.mPatchInfos[pos - 1];
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

    public void refreshState() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_PATCHING:
            setEnabled(false);

            if (getCardView() != null) {
                if ((mPCS.mSupported & PatcherConfigState.SUPPORTED_FILE) != 0) {
                    ((CardViewNative) getCardView()).setVisibility(View.GONE);
                } else {
                    ((CardViewNative) getCardView()).setVisibility(View.VISIBLE);
                }
            }

            break;

        case PatcherConfigState.STATE_CHOSE_FILE:
            setEnabled(true);

            if (getCardView() != null) {
                if ((mPCS.mSupported & PatcherConfigState.SUPPORTED_FILE) != 0) {
                    ((CardViewNative) getCardView()).setVisibility(View.GONE);
                } else {
                    ((CardViewNative) getCardView()).setVisibility(View.VISIBLE);
                }
            }

            break;

        case PatcherConfigState.STATE_INITIAL:
        case PatcherConfigState.STATE_FINISHED:
            setEnabled(true);

            if (getCardView() != null) {
                ((CardViewNative) getCardView()).setVisibility(View.GONE);
            }

            break;
        }
    }
}
