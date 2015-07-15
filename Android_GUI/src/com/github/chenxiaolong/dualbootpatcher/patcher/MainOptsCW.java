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

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ValueAnimator;
import android.animation.ValueAnimator.AnimatorUpdateListener;
import android.content.Context;
import android.os.Bundle;
import android.support.v7.widget.CardView;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.LayoutParams;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.PatchInfo;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils.InstallLocation;

import java.util.ArrayList;

public class MainOptsCW implements PatcherUIListener {
    protected interface MainOptsListener {
        void onDeviceSelected(Device device);

        void onRomIdSelected(String id);

        void onPresetSelected(PatchInfo info);
    }

    private static final String EXTRA_IS_DATA_SLOT = "is_data_slot";

    private CardView vCard;
    private View vDummy;
    private Spinner vDeviceSpinner;
    private Spinner vRomIdSpinner;
    private EditText vRomIdDataSlotId;
    private TextView vRomIdDesc;
    private LinearLayout vUnsupportedContainer;
    private Spinner vPresetSpinner;
    private TextView vPresetName;
    private LinearLayout vCustomOptsContainer;
    private CheckBox vDeviceCheckBox;
    private CheckBox vHasBootImageBox;
    private LinearLayout vBootImageContainer;
    private EditText vBootImageText;

    private Context mContext;
    private PatcherConfigState mPCS;
    private MainOptsListener mListener;

    private ArrayAdapter<String> mDeviceAdapter;
    private ArrayList<String> mDevices = new ArrayList<>();
    private ArrayAdapter<String> mRomIdAdapter;
    private ArrayList<String> mRomIds = new ArrayList<>();
    private ArrayAdapter<String> mPresetAdapter;
    private ArrayList<String> mPresets = new ArrayList<>();

    private boolean mIsDataSlot;

    public MainOptsCW(Context context, PatcherConfigState pcs, CardView card,
                      MainOptsListener listener) {
        mContext = context;
        mPCS = pcs;
        mListener = listener;

        vCard = card;
        vDummy = card.findViewById(R.id.customopts_dummylayout);
        vDeviceSpinner = (Spinner) card.findViewById(R.id.spinner_device);
        vRomIdSpinner = (Spinner) card.findViewById(R.id.spinner_rom_id);
        vRomIdDataSlotId = (EditText) card.findViewById(R.id.rom_id_data_slot_id);
        vRomIdDesc = (TextView) card.findViewById(R.id.rom_id_desc);
        vUnsupportedContainer = (LinearLayout) card.findViewById(R.id.unsupported_container);
        vPresetSpinner = (Spinner) card.findViewById(R.id.spinner_preset);
        vPresetName = (TextView) card.findViewById(R.id.preset_name);
        vCustomOptsContainer = (LinearLayout) card.findViewById(R.id.customopts_container);
        vDeviceCheckBox = (CheckBox) card.findViewById(R.id.customopts_devicecheck);
        vHasBootImageBox = (CheckBox) card.findViewById(R.id.customopts_hasbootimage);
        vBootImageContainer = (LinearLayout) card.findViewById(R.id.customopts_bootimage_container);
        vBootImageText = (EditText) card.findViewById(R.id.customopts_bootimage);

        initDevices();
        initRomIds();
        initPresets();
        initActions();
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

    private void initRomIds() {
        mRomIdAdapter = new ArrayAdapter<>(mContext,
                android.R.layout.simple_spinner_item, android.R.id.text1, mRomIds);
        mRomIdAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        vRomIdSpinner.setAdapter(mRomIdAdapter);

        vRomIdSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mIsDataSlot = (position >= 0 && position == mRomIds.size() - 1);

                onRomIdSelected(position);

                if (mListener != null) {
                    mListener.onRomIdSelected(getRomId());
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        vRomIdDataSlotId.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (!mIsDataSlot) {
                    return;
                }

                String text = s.toString();

                if (text.isEmpty()) {
                    vRomIdDataSlotId.setError(mContext.getString(
                            R.string.install_location_data_slot_id_error_is_empty));
                } else if (!text.matches("[a-z0-9]+")) {
                    vRomIdDataSlotId.setError(mContext.getString(
                            R.string.install_location_data_slot_id_error_invalid));
                }

                onDataSlotIdChanged(s.toString());

                if (mListener != null) {
                    mListener.onRomIdSelected(getRomId());
                }
            }
        });
    }

    private void initPresets() {
        mPresetAdapter = new ArrayAdapter<>(mContext, android.R.layout.simple_spinner_item,
                android.R.id.text1, mPresets);
        mPresetAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        vPresetSpinner.setAdapter(mPresetAdapter);

        vPresetSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                onPresetSelected(position);

                if (mListener != null) {
                    mListener.onPresetSelected(getPreset());
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });
    }

    private void initActions() {
        preventTextViewKeepFocus(vRomIdDataSlotId);
        preventTextViewKeepFocus(vBootImageText);

        vHasBootImageBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                buttonView.setClickable(false);
                onHasBootImageChecked(isChecked);
            }
        });
    }

    /**
     * Ugly hack to prevent the text box from keeping its focus.
     */
    private void preventTextViewKeepFocus(final TextView tv) {
        tv.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_SEARCH
                        || actionId == EditorInfo.IME_ACTION_DONE
                        || (event != null
                        && event.getAction() == KeyEvent.ACTION_DOWN
                        && event.getKeyCode() == KeyEvent.KEYCODE_ENTER)) {
                    tv.clearFocus();
                    InputMethodManager imm = (InputMethodManager)
                            mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(vBootImageText.getApplicationWindowToken(), 0);
                    vDummy.requestFocus();
                }
                return false;
            }
        });
    }

    @Override
    public void onCardCreate() {
        // Must rebuild from the saved state
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_INITIAL:
        case PatcherConfigState.STATE_FINISHED:
            // Don't show unsupported file options initially or when patching is finished
            collapseUnsupportedOpts();

            // Don't show preset name by default
            vPresetName.setVisibility(View.GONE);

            // But when we're finished, make sure views are re-enabled
            if (mPCS.mState == PatcherConfigState.STATE_FINISHED) {
                setEnabled(true);
            }

            break;

        case PatcherConfigState.STATE_CHOSE_FILE:
        case PatcherConfigState.STATE_PATCHING:
            // Show unsupported file options if file isn't supported
            if (mPCS.mSupported) {
                collapseUnsupportedOpts();
            } else {
                expandUnsupportedOpts();
            }

            // Show custom options if we're not using a preset
            if (mPCS.mPatchInfo == null) {
                expandCustomOpts();
            } else {
                collapseCustomOpts();
            }

            // But if we're patching, then every option must be disabled
            if (mPCS.mState == PatcherConfigState.STATE_PATCHING) {
                setEnabled(false);
            }

            break;
        }
    }

    @Override
    public void onRestoreCardState(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            mIsDataSlot = savedInstanceState.getBoolean(EXTRA_IS_DATA_SLOT, false);
        }

        // Show data slot ID text box if data-slot is chosen
        if (mIsDataSlot) {
            expandDataSlotId();
        } else {
            collapseDataSlotId();
        }
    }

    @Override
    public void onSaveCardState(Bundle outState) {
        outState.putBoolean(EXTRA_IS_DATA_SLOT, mIsDataSlot);
    }

    @Override
    public void onChoseFile() {
        // Show custom options if the file is unsupported
        if (mPCS.mSupported) {
            animateCollapseUnsupportedOpts();
        } else {
            animateExpandUnsupportedOpts();
        }
    }

    @Override
    public void onStartedPatching() {
        // Disable all views while patching
        setEnabled(false);
    }

    @Override
    public void onFinishedPatching() {
        // Enable all views after patching and hide custom options
        setEnabled(true);
        animateCollapseUnsupportedOpts();

        reset();
    }

    /**
     * Refresh the list of supported devices from libmbp.
     */
    public void refreshDevices() {
        mDevices.clear();
        for (Device device : PatcherUtils.sPC.getDevices()) {
            mDevices.add(String.format("%s - %s", device.getId(), device.getName()));
        }
        mDeviceAdapter.notifyDataSetChanged();
    }

    /**
     * Refresh the list of available ROM IDs
     */
    public void refreshRomIds() {
        mRomIds.clear();
        for (InstallLocation location : PatcherUtils.getInstallLocations(mContext)) {
            mRomIds.add(location.name);
        }
        mRomIds.add(mContext.getString(R.string.install_location_data_slot));
        mRomIdAdapter.notifyDataSetChanged();
    }

    /**
     * Refresh the list of available presets from libmbp.
     */
    public void refreshPresets() {
        mPresets.clear();

        // Default to using custom options
        mPresets.add(mContext.getString(R.string.preset_custom));
        for (PatchInfo info : mPCS.mPatchInfos) {
            mPresets.add(info.getId());
        }

        mPresetAdapter.notifyDataSetChanged();
    }

    private void onRomIdSelected(int position) {
        if (mIsDataSlot) {
            onDataSlotIdChanged(vRomIdDataSlotId.getText().toString());
            animateExpandDataSlotId();
        } else {
            InstallLocation[] locations = PatcherUtils.getInstallLocations(mContext);
            vRomIdDesc.setText(locations[position].description);
            animateCollapseDataSlotId();
        }
    }

    private void onDataSlotIdChanged(String text) {
        if (text.isEmpty()) {
            vRomIdDesc.setText(R.string.install_location_data_slot_id_empty);
        } else {
            InstallLocation location = PatcherUtils.getDataSlotInstallLocation(mContext, text);
            vRomIdDesc.setText(location.description);
        }
    }

    /**
     * Callback for the preset spinner. Animates the showing and hiding of the custom opts views.
     *
     * @param position Selected position of preset spinner
     */
    private void onPresetSelected(int position) {
        if (position == 0) {
            //vPresetName.setText(mContext.getString(R.string.preset_custom_desc));
            animateExpandCustomOpts();
        } else {
            vPresetName.setText(mPCS.mPatchInfos[position - 1].getName());
            animateCollapseCustomOpts();
        }
    }

    /**
     * Callback for the "has boot image" checkbox. Animates the showing and hiding of the boot image
     * option views.
     *
     * @param isChecked Whether the checkbox is checked
     */
    private void onHasBootImageChecked(boolean isChecked) {
        if (isChecked) {
            animateExpandBootImageOpts();
        } else {
            animateCollapseBootImageOpts();
        }
    }

    /**
     * Create an Animator that animates the changing of a view's height.
     *
     * @param view View to animate
     * @param start Starting height
     * @param end Ending height
     * @return Animator object
     */
    public static ValueAnimator createHeightAnimator(final View view, final int start,
                                                     final int end) {
        ValueAnimator animator = ValueAnimator.ofInt(start, end);
        animator.addUpdateListener(new AnimatorUpdateListener() {
            @Override
            public void onAnimationUpdate(ValueAnimator valueAnimator) {
                ViewGroup.LayoutParams params = view.getLayoutParams();
                params.height = (Integer) valueAnimator.getAnimatedValue();
                view.setLayoutParams(params);
            }
        });
        return animator;
    }

    /**
     * Create an Animator that expands a ViewGroup. When the Animator starts, the visibility of the
     * ViewGroup will be set to View.VISIBLE and the layout height will grow from 0px to the
     * measured height, as determined by measureView() and view.getMeasuredHeight(). When the
     * Animator completes, the layout height mode will be set to wrap_content.
     *
     * @param view View to expand
     * @return Animator object
     */
    private static Animator createExpandLayoutAnimator(final View view) {
        measureView(view);

        Animator animator = createHeightAnimator(view, 0, view.getMeasuredHeight());
        animator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationStart(Animator animation) {
                view.setVisibility(View.VISIBLE);
            }

            @Override
            public void onAnimationEnd(Animator animation) {
                setHeightWrapContent(view);
            }
        });

        return animator;
    }

    /**
     * Create an Animator that collapses a ViewGroup. When the Animator completes, the visibility
     * of the ViewGroup will be set to View.GONE and the layout height mode will be set to
     * wrap_content.
     *
     * @param view View to collapse
     * @return Animator object
     */
    private static Animator createCollapseLayoutAnimator(final View view) {
        ValueAnimator animator = createHeightAnimator(view, view.getHeight(), 0);
        animator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                view.setVisibility(View.GONE);

                setHeightWrapContent(view);
            }
        });
        return animator;
    }

    private void expandDataSlotId() {
        vRomIdDataSlotId.setVisibility(View.VISIBLE);
    }

    private void collapseDataSlotId() {
        vRomIdDataSlotId.setVisibility(View.GONE);
    }

    private void animateExpandDataSlotId() {
        if (vRomIdDataSlotId.getVisibility() == View.VISIBLE) {
            return;
        }

        Animator animator = createExpandLayoutAnimator(vRomIdDataSlotId);
        animator.start();
    }

    private void animateCollapseDataSlotId() {
        if (vRomIdDataSlotId.getVisibility() == View.GONE) {
            return;
        }

        Animator animator = createCollapseLayoutAnimator(vRomIdDataSlotId);
        animator.start();
    }

    /**
     * Expand the unsupported file options without using an animation. This should only be used
     * when setting the initial UI state (eg. hiding a view after an orientation change).
     */
    private void expandUnsupportedOpts() {
        vUnsupportedContainer.setVisibility(View.VISIBLE);
    }

    /**
     * Collapse the unsupported file options without using an animation. This should only be used
     * when setting the initial UI state (eg. hiding a view after an orientation change).
     */
    private void collapseUnsupportedOpts() {
        vUnsupportedContainer.setVisibility(View.GONE);
    }

    /**
     * Animate the expanding of the unsupported file options. This should only be called from
     * onChoseFile() when an unsupported file is chosen.
     *
     * No animation will occur if the layout is already expanded.
     */
    private void animateExpandUnsupportedOpts() {
        if (vUnsupportedContainer.getVisibility() == View.VISIBLE) {
            return;
        }

        Animator animator = createExpandLayoutAnimator(vUnsupportedContainer);
        animator.start();
    }

    /**
     * Animate the collapsing of the unsupported file options. This should only be called from
     * onChoseFile() when a supported file is chosen.
     *
     * No animation will occur if the layout is already collapsed.
     */
    private void animateCollapseUnsupportedOpts() {
        if (vUnsupportedContainer.getVisibility() == View.GONE) {
            return;
        }

        Animator animator = createCollapseLayoutAnimator(vUnsupportedContainer);
        animator.start();
    }

    /**
     * Expand the custom options without using an animation. This should only be used when
     * setting the initial UI state (eg. hiding a view after an orientation change).
     */
    private void expandCustomOpts() {
        vCustomOptsContainer.setVisibility(View.VISIBLE);
        vPresetName.setVisibility(View.GONE);
    }

    /**
     * Collapse the custom options without using an animation. This should only be used when
     * setting the initial UI state (eg. hiding a view after an orientation change).
     */
    private void collapseCustomOpts() {
        vCustomOptsContainer.setVisibility(View.GONE);
        vPresetName.setVisibility(View.VISIBLE);
    }

    /**
     * Animate the expanding of the custom options. This should only be called if the user chooses
     * to use custom options (as opposed to a preset).
     *
     * No animation will occur if the layout is already expanded.
     */
    private void animateExpandCustomOpts() {
        if (vCustomOptsContainer.getVisibility() == View.VISIBLE) {
            return;
        }

        Animator animator = createCollapseLayoutAnimator(vPresetName);
        Animator animator2 = createExpandLayoutAnimator(vCustomOptsContainer);
        AnimatorSet set = new AnimatorSet();
        set.play(animator2).after(animator);
        set.start();
    }

    /**
     * Animate the collapsing of the custom options. This should only be called if the user chooses
     * a valid preset.
     *
     * No animation will occur if the layout is already collapsed.
     */
    private void animateCollapseCustomOpts() {
        if (vCustomOptsContainer.getVisibility() == View.GONE) {
            return;
        }

        Animator animator = createCollapseLayoutAnimator(vCustomOptsContainer);
        Animator animator2 = createExpandLayoutAnimator(vPresetName);
        AnimatorSet set = new AnimatorSet();
        set.play(animator2).after(animator);
        set.start();
    }

    /**
     * Animate the expanding of the boot image options. This should only be called if the user
     * clicks on the "has boot image" checkbox.
     *
     * No animation will occur if the layout is already expanded.
     */
    private void animateExpandBootImageOpts() {
        if (vBootImageContainer.getVisibility() == View.VISIBLE) {
            return;
        }

        Animator animator = createExpandLayoutAnimator(vBootImageContainer);
        animator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                vHasBootImageBox.setClickable(true);
            }
        });
        animator.start();
    }

    /**
     * Animate the collapsing of the boot image options. This should only be called if the user
     * clicks on the "has boot image" checkbox.
     *
     * No animation will occur if the layout is already collapsed.
     */
    private void animateCollapseBootImageOpts() {
        if (vBootImageContainer.getVisibility() == View.GONE) {
            return;
        }

        Animator animator = createCollapseLayoutAnimator(vBootImageContainer);
        animator.addListener(new AnimatorListenerAdapter() {
            @Override
            public void onAnimationEnd(Animator animation) {
                vHasBootImageBox.setClickable(true);
            }
        });
        animator.start();
    }

    /**
     * Measure the dimensions of a view before it is drawn (eg. if visibility is View.GONE).
     *
     * @param view View
     */
    private static void measureView(View view) {
        View parent = (View) view.getParent();
        final int widthSpec = View.MeasureSpec.makeMeasureSpec(parent.getMeasuredWidth() -
                        parent.getPaddingLeft() - parent.getPaddingRight(),
                View.MeasureSpec.AT_MOST);
        final int heightSpec = View.MeasureSpec.makeMeasureSpec(0,
                View.MeasureSpec.UNSPECIFIED);
        view.measure(widthSpec, heightSpec);
    }

    /**
     * Set a layout's height param to wrap_content.
     *
     * @param view ViewGroup/layout
     */
    private static void setHeightWrapContent(View view) {
        LayoutParams params = view.getLayoutParams();
        params.height = LayoutParams.WRAP_CONTENT;
        view.setLayoutParams(params);
    }

    private String getRomId() {
        if (mIsDataSlot) {
            return PatcherUtils.getDataSlotRomId(vRomIdDataSlotId.getText().toString());
        } else {
            int pos = vRomIdSpinner.getSelectedItemPosition();
            if (pos >= 0) {
                InstallLocation[] locations = PatcherUtils.getInstallLocations(mContext);
                return locations[pos].id;
            } else {
                return null;
            }
        }
    }

    /**
     * Get the PatchInfo object corresponding to the selected preset
     *
     * @return PatchInfo object if preset is used. Otherwise, null
     */
    private PatchInfo getPreset() {
        int pos = vPresetSpinner.getSelectedItemPosition();

        if (pos == 0) {
            // Custom options
            return null;
        } else {
            return mPCS.mPatchInfos[pos - 1];
        }
    }

    /**
     * Check whether device checks should be removed
     *
     * @return False (!!) if device checks should be removed
     */
    public boolean isDeviceCheckEnabled() {
        return !vDeviceCheckBox.isChecked();
    }

    /**
     * Check whether "has boot image" is checked
     *
     * @return Whether the "has boot image" checkbox is checked
     */
    public boolean isHasBootImageEnabled() {
        return vHasBootImageBox.isChecked();
    }

    /**
     * Get the user-entered boot image string
     *
     * @return Boot image filename
     */
    public String getBootImage() {
        String bootImage = vBootImageText.getText().toString().trim();
        if (bootImage.isEmpty()) {
            return null;
        } else {
            return bootImage;
        }
    }

    /**
     * Enable or disable this card
     *
     * @param enabled Whether the card should be enabled or disabled
     */
    public void setEnabled(boolean enabled) {
        setEnabledRecursive(vCard, enabled);
    }

    /**
     * Enable or disable a view and all of its subviews.
     *
     * @param view View or layout
     * @param enabled Whether the views should be enabled or disabled
     */
    private static void setEnabledRecursive(View view, boolean enabled) {
        view.setEnabled(enabled);
        if (view instanceof ViewGroup) {
            ViewGroup vg = (ViewGroup) view;
            for (int i = 0; i < vg.getChildCount(); i++) {
                View child = vg.getChildAt(i);
                setEnabledRecursive(child, enabled);
            }
        }
    }

    /**
     * Reset options to sane defaults after a file has been patched
     */
    private void reset() {
        vPresetSpinner.setSelection(0);
        vDeviceCheckBox.setChecked(false);
        vHasBootImageBox.setChecked(true);
        vBootImageText.setText("");
    }
}
