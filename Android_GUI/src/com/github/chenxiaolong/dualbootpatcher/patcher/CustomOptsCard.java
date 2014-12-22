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
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.view.CardViewNative;

public class CustomOptsCard extends Card {
    private PatcherConfigState mPCS;

    private TextView mTitle;
    private CheckBox mDeviceCheckBox;
    private CheckBox mHasBootImageBox;
    private TextView mBootImageTitle;
    private EditText mBootImageText;

    private boolean mUsingPreset;
    private boolean mDisable;

    public CustomOptsCard(Context context, PatcherConfigState pcs) {
        this(context, R.layout.cardcontent_customopts);
        mPCS = pcs;
    }

    public CustomOptsCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mTitle = (TextView) view.findViewById(R.id.card_title);
            mDeviceCheckBox = (CheckBox) view
                    .findViewById(R.id.customopts_devicecheck);
            mHasBootImageBox = (CheckBox) view
                    .findViewById(R.id.customopts_hasbootimage);
            mBootImageTitle = (TextView) view
                    .findViewById(R.id.customopts_bootimage_title);
            mBootImageText = (EditText) view
                    .findViewById(R.id.customopts_bootimage);

            // Ugly hack to prevent the text box from keeping its focus
            mBootImageText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
                @Override
                public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                    if (actionId == EditorInfo.IME_ACTION_SEARCH || actionId == EditorInfo
                            .IME_ACTION_DONE || event.getAction() == KeyEvent.ACTION_DOWN &&
                            event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
                        mBootImageText.clearFocus();
                        InputMethodManager imm = (InputMethodManager) getContext()
                                .getSystemService(Context.INPUT_METHOD_SERVICE);
                        imm.hideSoftInputFromWindow(mBootImageText.getApplicationWindowToken(), 0);
                    }
                    return false;
                }
            });

            mHasBootImageBox.setOnCheckedChangeListener(
                    new CompoundButton.OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    updateViews();
                }
            });

            updateViews();
        }
    }

    private void updateViews() {
        if (mUsingPreset || mDisable) {
            mTitle.setEnabled(false);
            mDeviceCheckBox.setEnabled(false);
            mHasBootImageBox.setEnabled(false);
            mBootImageTitle.setEnabled(false);
            mBootImageText.setEnabled(false);
            return;
        } else {
            mTitle.setEnabled(true);
            mDeviceCheckBox.setEnabled(true);
            mHasBootImageBox.setEnabled(true);
            mBootImageTitle.setEnabled(true);
            mBootImageText.setEnabled(true);
        }

        if (mHasBootImageBox.isChecked()) {
            mBootImageTitle.setEnabled(true);
            mBootImageText.setEnabled(true);
        } else {
            mBootImageTitle.setEnabled(false);
            mBootImageText.setEnabled(false);
        }
    }

    public void setUsingPreset(boolean enabled) {
        mUsingPreset = enabled;
        updateViews();
    }

    public void setEnabled(boolean enabled) {
        mDisable = !enabled;
        updateViews();
    }

    public boolean isDeviceCheckEnabled() {
        return !mDeviceCheckBox.isChecked();
    }

    public boolean isHasBootImageEnabled() {
        return mHasBootImageBox.isChecked();
    }

    public String getBootImage() {
        String bootImage = mBootImageText.getText().toString().trim();
        if (bootImage.isEmpty()) {
            return null;
        } else {
            return bootImage;
        }
    }

    public void reset() {
        mDeviceCheckBox.setChecked(false);
        mHasBootImageBox.setChecked(true);
        mBootImageText.setText("");
    }

    public void refreshState() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_PATCHING:
            setEnabled(false);

            if (getCardView() != null) {
                if ((mPCS.mSupported & PatcherConfigState.SUPPORTED_FILE) != 0
                        && (mPCS.mSupported & PatcherConfigState.SUPPORTED_PARTCONFIG) != 0) {
                    ((CardViewNative) getCardView()).setVisibility(View.GONE);
                } else {
                    ((CardViewNative) getCardView()).setVisibility(View.VISIBLE);
                }
            }

            break;

        case PatcherConfigState.STATE_CHOSE_FILE:
            setEnabled(true);

            if (getCardView() != null) {
                if ((mPCS.mSupported & PatcherConfigState.SUPPORTED_FILE) != 0
                        && (mPCS.mSupported & PatcherConfigState.SUPPORTED_PARTCONFIG) != 0) {
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
