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
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class CustomOptsCW {
    private CardView vCard;
    private TextView vTitle;
    private CheckBox vDeviceCheckBox;
    private CheckBox vHasBootImageBox;
    private TextView vBootImageTitle;
    private EditText vBootImageText;

    private Context mContext;
    private PatcherConfigState mPCS;

    private boolean mUsingPreset;
    private boolean mDisable;

    public CustomOptsCW(Context context, PatcherConfigState pcs, CardView card) {
        mContext = context;
        mPCS = pcs;

        vCard = card;
        vTitle = (TextView) card.findViewById(R.id.card_title);
        vDeviceCheckBox = (CheckBox) card.findViewById(R.id.customopts_devicecheck);
        vHasBootImageBox = (CheckBox) card.findViewById(R.id.customopts_hasbootimage);
        vBootImageTitle = (TextView) card.findViewById(R.id.customopts_bootimage_title);
        vBootImageText = (EditText) card.findViewById(R.id.customopts_bootimage);

        initActions();
        updateViews();
    }

    private void initActions() {
        // Ugly hack to prevent the text box from keeping its focus
        vBootImageText.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_SEARCH
                        || actionId == EditorInfo.IME_ACTION_DONE
                        || event.getAction() == KeyEvent.ACTION_DOWN
                        && event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
                    vBootImageText.clearFocus();
                    InputMethodManager imm = (InputMethodManager) mContext
                            .getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(vBootImageText.getApplicationWindowToken(), 0);
                }
                return false;
            }
        });

        vHasBootImageBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                updateViews();
            }
        });
    }

    private void updateViews() {
        if (mUsingPreset || mDisable) {
            vTitle.setEnabled(false);
            vDeviceCheckBox.setEnabled(false);
            vHasBootImageBox.setEnabled(false);
            vBootImageTitle.setEnabled(false);
            vBootImageText.setEnabled(false);
            return;
        } else {
            vTitle.setEnabled(true);
            vDeviceCheckBox.setEnabled(true);
            vHasBootImageBox.setEnabled(true);
            vBootImageTitle.setEnabled(true);
            vBootImageText.setEnabled(true);
        }

        if (vHasBootImageBox.isChecked()) {
            vBootImageTitle.setEnabled(true);
            vBootImageText.setEnabled(true);
        } else {
            vBootImageTitle.setEnabled(false);
            vBootImageText.setEnabled(false);
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
        return !vDeviceCheckBox.isChecked();
    }

    public boolean isHasBootImageEnabled() {
        return vHasBootImageBox.isChecked();
    }

    public String getBootImage() {
        String bootImage = vBootImageText.getText().toString().trim();
        if (bootImage.isEmpty()) {
            return null;
        } else {
            return bootImage;
        }
    }

    public void reset() {
        vDeviceCheckBox.setChecked(false);
        vHasBootImageBox.setChecked(true);
        vBootImageText.setText("");
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
