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

import java.io.File;

import android.content.Context;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.PatcherInformation;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation.Device;
import com.github.chenxiaolong.dualbootpatcher.R;

public class CustomOptsCard extends Card {
    private TextView mTitle;
    private RadioGroup mRadioGroup;
    private RadioButton mAutopatchButton;
    private RadioButton mPatchButton;
    private Spinner mAutopatchSpinner;
    Button mChoosePatchButton;
    private CheckBox mDeviceCheckBox;
    private CheckBox mHasBootImageBox;
    private CheckBox mLokiBox;
    private TextView mRamdiskTitle;
    private Spinner mRamdiskSpinner;
    private ArrayAdapter<String> mRamdiskAdapter;
    private TextView mInitTitle;
    private Spinner mInitSpinner;
    private TextView mBootImageTitle;
    private EditText mBootImageText;

    private PatcherInformation mInfo;

    private boolean mUsingPreset;
    private boolean mDisable;

    private String mDiffFile;

    public CustomOptsCard(Context context) {
        this(context, R.layout.cardcontent_customopts);
    }

    public CustomOptsCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mTitle = (TextView) view.findViewById(R.id.card_title);
            mRadioGroup = (RadioGroup) view
                    .findViewById(R.id.customopts_radiogroup);
            mAutopatchButton = (RadioButton) view
                    .findViewById(R.id.customopts_autopatch_button);
            mPatchButton = (RadioButton) view
                    .findViewById(R.id.customopts_patch_button);
            mAutopatchSpinner = (Spinner) view
                    .findViewById(R.id.spinner_autopatch);
            mChoosePatchButton = (Button) view
                    .findViewById(R.id.customopts_choosepatch);
            mDeviceCheckBox = (CheckBox) view
                    .findViewById(R.id.customopts_devicecheck);
            mHasBootImageBox = (CheckBox) view
                    .findViewById(R.id.customopts_hasbootimage);
            mLokiBox = (CheckBox) view.findViewById(R.id.customopts_loki);
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
            mBootImageText
                    .setOnEditorActionListener(new TextView.OnEditorActionListener() {
                        @Override
                        public boolean onEditorAction(TextView v, int actionId,
                                KeyEvent event) {
                            if (actionId == EditorInfo.IME_ACTION_SEARCH
                                    || actionId == EditorInfo.IME_ACTION_DONE
                                    || event.getAction() == KeyEvent.ACTION_DOWN
                                    && event.getKeyCode() == KeyEvent.KEYCODE_ENTER) {
                                mBootImageText.clearFocus();
                                InputMethodManager imm = (InputMethodManager) getContext()
                                        .getSystemService(
                                                Context.INPUT_METHOD_SERVICE);
                                imm.hideSoftInputFromWindow(mBootImageText
                                        .getApplicationWindowToken(), 0);
                            }
                            return false;
                        }
                    });

            mRadioGroup
                    .setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(RadioGroup group,
                                int checkedId) {
                            switch (checkedId) {
                            case R.id.customopts_autopatch_button:
                                mAutopatchSpinner.setVisibility(View.VISIBLE);
                                mChoosePatchButton.setVisibility(View.GONE);
                                break;

                            case R.id.customopts_patch_button:
                                mAutopatchSpinner.setVisibility(View.GONE);
                                mChoosePatchButton.setVisibility(View.VISIBLE);
                                break;
                            }

                        }
                    });

            if (mRadioGroup.getCheckedRadioButtonId() == -1) {
                mRadioGroup.check(R.id.customopts_autopatch_button);
            }

            mHasBootImageBox
                    .setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
                        @Override
                        public void onCheckedChanged(CompoundButton buttonView,
                                boolean isChecked) {
                            updateViews();
                        }
                    });

            updateViews();
        }
    }

    private void populateAutopatchers() {
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mAutopatchSpinner.setAdapter(adapter);

        for (int i = 0; i < mInfo.mAutopatchers.length; i++) {
            String autopatcher = mInfo.mAutopatchers[i];
            adapter.add(autopatcher);
        }
        adapter.notifyDataSetChanged();
    }

    private void populateRamdisks(Device device) {
        mRamdiskAdapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        mRamdiskAdapter
                .setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mRamdiskSpinner.setAdapter(mRamdiskAdapter);

        reloadRamdisks(device);
    }

    public void reloadRamdisks(Device device) {
        mRamdiskAdapter.clear();

        for (int i = 0; i < mInfo.mRamdisks.length; i++) {
            String ramdisk = mInfo.mRamdisks[i];
            if (ramdisk.startsWith(device.mCodeName + "/")) {
                mRamdiskAdapter.add(ramdisk);
            }
        }
        mRamdiskAdapter.notifyDataSetChanged();
    }

    private void populateInits() {
        ArrayAdapter<String> adapter = new ArrayAdapter<String>(getContext(),
                android.R.layout.simple_spinner_item, android.R.id.text1);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mInitSpinner.setAdapter(adapter);

        // Default to no patched init
        adapter.add(getContext().getString(R.string.none));

        for (int i = 0; i < mInfo.mInits.length; i++) {
            String init = mInfo.mInits[i];
            adapter.add(init);
        }

        adapter.notifyDataSetChanged();
    }

    private void updateViews() {
        if (mDiffFile != null) {
            String filename = new File(mDiffFile).getName();
            mChoosePatchButton.setText(filename);
        }

        if (mUsingPreset || mDisable) {
            mTitle.setEnabled(false);
            mAutopatchButton.setEnabled(false);
            mPatchButton.setEnabled(false);
            mAutopatchSpinner.setEnabled(false);
            mChoosePatchButton.setEnabled(false);
            mDeviceCheckBox.setEnabled(false);
            mHasBootImageBox.setEnabled(false);
            mLokiBox.setEnabled(false);
            mRamdiskTitle.setEnabled(false);
            mRamdiskSpinner.setEnabled(false);
            mInitTitle.setEnabled(false);
            mInitSpinner.setEnabled(false);
            mBootImageTitle.setEnabled(false);
            mBootImageText.setEnabled(false);
            return;
        } else {
            mTitle.setEnabled(true);
            mAutopatchButton.setEnabled(true);
            mPatchButton.setEnabled(true);
            mAutopatchSpinner.setEnabled(true);
            mChoosePatchButton.setEnabled(true);
            mDeviceCheckBox.setEnabled(true);
            mHasBootImageBox.setEnabled(true);
            mLokiBox.setEnabled(true);
            mRamdiskTitle.setEnabled(true);
            mRamdiskSpinner.setEnabled(true);
            mInitTitle.setEnabled(true);
            mInitSpinner.setEnabled(true);
            mBootImageTitle.setEnabled(true);
            mBootImageText.setEnabled(true);
        }

        if (mHasBootImageBox.isChecked()) {
            mLokiBox.setEnabled(true);
            mRamdiskTitle.setEnabled(true);
            mRamdiskSpinner.setEnabled(true);
            mInitTitle.setEnabled(true);
            mInitSpinner.setEnabled(true);
            mBootImageTitle.setEnabled(true);
            mBootImageText.setEnabled(true);
        } else {
            mLokiBox.setEnabled(false);
            mRamdiskTitle.setEnabled(false);
            mRamdiskSpinner.setEnabled(false);
            mInitTitle.setEnabled(false);
            mInitSpinner.setEnabled(false);
            mBootImageTitle.setEnabled(false);
            mBootImageText.setEnabled(false);
        }
    }

    public void setPatcherInfo(PatcherInformation info, Device device) {
        mInfo = info;

        populateAutopatchers();
        populateRamdisks(device);
        populateInits();
    }

    public void setUsingPreset(boolean enabled) {
        mUsingPreset = enabled;
        updateViews();
    }

    public void setEnabled(boolean enabled) {
        mDisable = !enabled;
        updateViews();
    }

    public void onDiffFileSelected(String file) {
        mDiffFile = file;
        updateViews();
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putString("diffFile", mDiffFile);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        mDiffFile = savedInstanceState.getString("diffFile");
    }

    public boolean isUsingAutopatcher() {
        return mRadioGroup.getCheckedRadioButtonId() == R.id.customopts_autopatch_button;
    }

    public boolean isUsingPatch() {
        return mRadioGroup.getCheckedRadioButtonId() == R.id.customopts_patch_button;
    }

    public String getAutopatcher() {
        return mInfo.mAutopatchers[mAutopatchSpinner.getSelectedItemPosition()];
    }

    public String getPatch() {
        return mDiffFile;
    }

    public boolean isDeviceCheckEnabled() {
        return !mDeviceCheckBox.isChecked();
    }

    public boolean isHasBootImageEnabled() {
        return mHasBootImageBox.isChecked();
    }

    public boolean isLokiEnabled() {
        return mLokiBox.isChecked();
    }

    public String getRamdisk() {
        // return mInfo.mRamdisks[mRamdiskSpinner.getSelectedItemPosition()];
        return mRamdiskAdapter.getItem(mRamdiskSpinner
                .getSelectedItemPosition());
    }

    public String getPatchedInit() {
        int position = mInitSpinner.getSelectedItemPosition();

        if (position == 0) {
            return null;
        } else {
            return mInfo.mInits[position - 1];
        }
    }

    public String getBootImage() {
        return mBootImageText.getText().toString();
    }

    public void reset() {
        mRadioGroup.check(R.id.customopts_autopatch_button);
        mAutopatchSpinner.setSelection(0);
        mChoosePatchButton.setText(R.string.customopts_choosepatch);
        mDeviceCheckBox.setChecked(false);
        mHasBootImageBox.setChecked(true);
        mLokiBox.setChecked(false);
        mRamdiskSpinner.setSelection(0);
        mInitSpinner.setSelection(0);
        mBootImageText.setText(R.string.customopts_bootimage_default);

        mDiffFile = null;
    }
}
