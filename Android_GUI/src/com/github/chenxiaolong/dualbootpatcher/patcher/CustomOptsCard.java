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
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.view.CardView;

public class CustomOptsCard extends Card {
    public static interface CustomOptsSelectedListener {
        public void onAutoPatcherSelected(String autoPatcherId);

        public void onRamdiskSelected(String ramdisk);

        public void onPatchedInitSelected(String init);
    }

    private PatcherConfigState mPCS;
    private CustomOptsSelectedListener mListener;

    private TextView mTitle;
    private ArrayAdapter<String> mAutoPatcherAdapter;
    private Spinner mAutoPatcherSpinner;
    private CheckBox mDeviceCheckBox;
    private CheckBox mHasBootImageBox;
    private TextView mRamdiskTitle;
    private ArrayAdapter<String> mRamdiskAdapter;
    private Spinner mRamdiskSpinner;
    private TextView mInitTitle;
    private ArrayAdapter<String> mInitAdapter;
    private Spinner mInitSpinner;
    private TextView mBootImageTitle;
    private EditText mBootImageText;

    private boolean mUsingPreset;
    private boolean mDisable;

    public CustomOptsCard(Context context, PatcherConfigState pcs,
                          CustomOptsSelectedListener listener) {
        this(context, R.layout.cardcontent_customopts);
        mPCS = pcs;
        mListener = listener;
    }

    public CustomOptsCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mTitle = (TextView) view.findViewById(R.id.card_title);
            mAutoPatcherSpinner = (Spinner) view
                    .findViewById(R.id.spinner_autopatch);
            mDeviceCheckBox = (CheckBox) view
                    .findViewById(R.id.customopts_devicecheck);
            mHasBootImageBox = (CheckBox) view
                    .findViewById(R.id.customopts_hasbootimage);
            mRamdiskTitle = (TextView) view
                    .findViewById(R.id.customopts_ramdisk);
            mRamdiskSpinner = (Spinner) view.findViewById(R.id.spinner_ramdisk);
            mInitTitle = (TextView) view
                    .findViewById(R.id.customopts_patchedinit);
            mInitSpinner = (Spinner) view
                    .findViewById(R.id.spinner_patchedinit);
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

        initControls();
    }

    private void initControls() {
        mAutoPatcherAdapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        mAutoPatcherAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mAutoPatcherSpinner.setAdapter(mAutoPatcherAdapter);

        mAutoPatcherSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (mListener != null){
                    mListener.onAutoPatcherSelected(mAutoPatcherSpinner.getSelectedItem().toString());
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        mRamdiskAdapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        mRamdiskAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mRamdiskSpinner.setAdapter(mRamdiskAdapter);

        mRamdiskSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (mListener != null) {
                    mListener.onRamdiskSelected(mRamdiskSpinner.getSelectedItem().toString());
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        mInitAdapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        mInitAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mInitSpinner.setAdapter(mInitAdapter);

        mInitSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int positions, long id) {
                if (mListener != null) {
                    int position = mInitSpinner.getSelectedItemPosition();
                    if (position == 0) {
                        mListener.onPatchedInitSelected(null);
                    } else {
                        mListener.onPatchedInitSelected(mInitSpinner.getSelectedItem().toString());
                    }
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    public void refreshAutoPatchers() {
        for (String ap : PatcherUtils.sPC.getAutoPatchers()) {
            if (ap.equals("PatchFile")) {
                continue;
            }
            mAutoPatcherAdapter.add(ap);
        }
        mAutoPatcherAdapter.notifyDataSetChanged();
    }

    public void refreshRamdisks() {
        mRamdiskAdapter.clear();

        for (String rp : PatcherUtils.sPC.getRamdiskPatchers()) {
            if (rp.startsWith(mPCS.mDevice.getCodename() + "/")) {
                mRamdiskAdapter.add(rp);
            }
        }

        mRamdiskAdapter.notifyDataSetChanged();
    }

    public void refreshInits() {
        // Default to no patched init
        mInitAdapter.add(getContext().getString(R.string.none));
        mInitAdapter.addAll(PatcherUtils.sPC.getInitBinaries());
        mInitAdapter.notifyDataSetChanged();
    }

    private void updateViews() {
        if (mUsingPreset || mDisable) {
            mTitle.setEnabled(false);
            mAutoPatcherSpinner.setEnabled(false);
            mDeviceCheckBox.setEnabled(false);
            mHasBootImageBox.setEnabled(false);
            mRamdiskTitle.setEnabled(false);
            mRamdiskSpinner.setEnabled(false);
            mInitTitle.setEnabled(false);
            mInitSpinner.setEnabled(false);
            mBootImageTitle.setEnabled(false);
            mBootImageText.setEnabled(false);
            return;
        } else {
            mTitle.setEnabled(true);
            mAutoPatcherSpinner.setEnabled(true);
            mDeviceCheckBox.setEnabled(true);
            mHasBootImageBox.setEnabled(true);
            mRamdiskTitle.setEnabled(true);
            mRamdiskSpinner.setEnabled(true);
            mInitTitle.setEnabled(true);
            mInitSpinner.setEnabled(true);
            mBootImageTitle.setEnabled(true);
            mBootImageText.setEnabled(true);
        }

        if (mHasBootImageBox.isChecked()) {
            mRamdiskTitle.setEnabled(true);
            mRamdiskSpinner.setEnabled(true);
            mInitTitle.setEnabled(true);
            mInitSpinner.setEnabled(true);
            mBootImageTitle.setEnabled(true);
            mBootImageText.setEnabled(true);
        } else {
            mRamdiskTitle.setEnabled(false);
            mRamdiskSpinner.setEnabled(false);
            mInitTitle.setEnabled(false);
            mInitSpinner.setEnabled(false);
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
        mAutoPatcherSpinner.setSelection(0);
        mDeviceCheckBox.setChecked(false);
        mHasBootImageBox.setChecked(true);
        mRamdiskSpinner.setSelection(0);
        mInitSpinner.setSelection(0);
        mBootImageText.setText("");
    }

    public void refreshState() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_PATCHING:
            setEnabled(false);

            if (getCardView() != null) {
                if ((mPCS.mSupported & PatcherConfigState.SUPPORTED_FILE) != 0
                        && (mPCS.mSupported & PatcherConfigState.SUPPORTED_PARTCONFIG) != 0) {
                    ((CardView) getCardView()).setVisibility(View.GONE);
                } else {
                    ((CardView) getCardView()).setVisibility(View.VISIBLE);
                }
            }

            break;

        case PatcherConfigState.STATE_CHOSE_FILE:
            setEnabled(true);

            if (getCardView() != null) {
                if ((mPCS.mSupported & PatcherConfigState.SUPPORTED_FILE) != 0
                        && (mPCS.mSupported & PatcherConfigState.SUPPORTED_PARTCONFIG) != 0) {
                    ((CardView) getCardView()).setVisibility(View.GONE);
                } else {
                    ((CardView) getCardView()).setVisibility(View.VISIBLE);
                }
            }

            break;

        case PatcherConfigState.STATE_INITIAL:
        case PatcherConfigState.STATE_FINISHED:
            setEnabled(true);

            if (getCardView() != null) {
                ((CardView) getCardView()).setVisibility(View.GONE);
            }

            break;
        }
    }
}
