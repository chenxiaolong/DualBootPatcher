/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils.InstallLocation;

import java.util.ArrayList;

public class MainOptsCW implements PatcherUIListener {
    protected interface MainOptsListener {
        void onDeviceSelected(Device device);

        void onRomIdSelected(String id);
    }

    private static final String EXTRA_IS_NAMED_SLOT = "is_named_slot";

    private CardView vCard;
    private View vDummy;
    private Spinner vDeviceSpinner;
    private Spinner vRomIdSpinner;
    private EditText vRomIdNamedSlotId;
    private TextView vRomIdDesc;

    private Context mContext;
    private PatcherConfigState mPCS;
    private MainOptsListener mListener;

    private ArrayAdapter<String> mDeviceAdapter;
    private ArrayList<String> mDevices = new ArrayList<>();
    private ArrayAdapter<String> mRomIdAdapter;
    private ArrayList<String> mRomIds = new ArrayList<>();

    private boolean mIsNamedSlot;

    public MainOptsCW(Context context, PatcherConfigState pcs, CardView card,
                      MainOptsListener listener) {
        mContext = context;
        mPCS = pcs;
        mListener = listener;

        vCard = card;
        vDummy = card.findViewById(R.id.customopts_dummylayout);
        vDeviceSpinner = (Spinner) card.findViewById(R.id.spinner_device);
        vRomIdSpinner = (Spinner) card.findViewById(R.id.spinner_rom_id);
        vRomIdNamedSlotId = (EditText) card.findViewById(R.id.rom_id_named_slot_id);
        vRomIdDesc = (TextView) card.findViewById(R.id.rom_id_desc);

        initDevices();
        initRomIds();
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
                mIsNamedSlot = (mRomIds.size() >= 2
                        && (position == mRomIds.size() - 1
                        || position == mRomIds.size() - 2));

                onRomIdSelected(position);

                if (mListener != null) {
                    mListener.onRomIdSelected(getRomId());
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        vRomIdNamedSlotId.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                if (!mIsNamedSlot) {
                    return;
                }

                String text = s.toString();

                if (text.isEmpty()) {
                    vRomIdNamedSlotId.setError(mContext.getString(
                            R.string.install_location_named_slot_id_error_is_empty));
                } else if (!text.matches("[a-z0-9]+")) {
                    vRomIdNamedSlotId.setError(mContext.getString(
                            R.string.install_location_named_slot_id_error_invalid));
                }

                onNamedSlotIdChanged(s.toString());

                if (mListener != null) {
                    mListener.onRomIdSelected(getRomId());
                }
            }
        });
    }

    private void initActions() {
        preventTextViewKeepFocus(vRomIdNamedSlotId);
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
                    imm.hideSoftInputFromWindow(vRomIdNamedSlotId.getApplicationWindowToken(), 0);
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
            // But when we're finished, make sure views are re-enabled
            if (mPCS.mState == PatcherConfigState.STATE_FINISHED) {
                setEnabled(true);
            }

            break;

        case PatcherConfigState.STATE_CHOSE_FILE:
        case PatcherConfigState.STATE_PATCHING:
            // If we're patching, then every option must be disabled
            if (mPCS.mState == PatcherConfigState.STATE_PATCHING) {
                setEnabled(false);
            }

            break;
        }
    }

    @Override
    public void onRestoreCardState(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            mIsNamedSlot = savedInstanceState.getBoolean(EXTRA_IS_NAMED_SLOT, false);
        }

        // Show named slot ID text box if data-slot or extsd-slot is chosen
        if (mIsNamedSlot) {
            expandNamedSlotId();
        } else {
            collapseNamedSlotId();
        }

        // Select our device on initial startup
        if (savedInstanceState == null) {
            String realCodename = RomUtils.getDeviceCodename(mContext);
            Device[] devices = PatcherUtils.sPC.getDevices();
            outer:
            for (int i = 0; i < devices.length; i++) {
                for (String codename : devices[i].getCodenames()) {
                    if (realCodename.equals(codename)) {
                        vDeviceSpinner.setSelection(i);
                        break outer;
                    }
                }
            }
        }
    }

    @Override
    public void onSaveCardState(Bundle outState) {
        outState.putBoolean(EXTRA_IS_NAMED_SLOT, mIsNamedSlot);
    }

    @Override
    public void onChoseFile() {
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
        mRomIds.add(mContext.getString(R.string.install_location_extsd_slot));
        mRomIdAdapter.notifyDataSetChanged();
    }

    private void onRomIdSelected(int position) {
        if (mIsNamedSlot) {
            onNamedSlotIdChanged(vRomIdNamedSlotId.getText().toString());
            animateExpandNamedSlotId();
        } else {
            InstallLocation[] locations = PatcherUtils.getInstallLocations(mContext);
            vRomIdDesc.setText(locations[position].description);
            animateCollapseNamedSlotId();
        }
    }

    private void onNamedSlotIdChanged(String text) {
        if (text.isEmpty()) {
            vRomIdDesc.setText(R.string.install_location_named_slot_id_empty);
        } else if (vRomIdSpinner.getSelectedItemPosition() == mRomIds.size() - 2) {
            InstallLocation location = PatcherUtils.getDataSlotInstallLocation(mContext, text);
            vRomIdDesc.setText(location.description);
        } else if (vRomIdSpinner.getSelectedItemPosition() == mRomIds.size() - 1) {
            InstallLocation location = PatcherUtils.getExtsdSlotInstallLocation(mContext, text);
            vRomIdDesc.setText(location.description);
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

    private void expandNamedSlotId() {
        vRomIdNamedSlotId.setVisibility(View.VISIBLE);
    }

    private void collapseNamedSlotId() {
        vRomIdNamedSlotId.setVisibility(View.GONE);
    }

    private void animateExpandNamedSlotId() {
        if (vRomIdNamedSlotId.getVisibility() == View.VISIBLE) {
            return;
        }

        Animator animator = createExpandLayoutAnimator(vRomIdNamedSlotId);
        animator.start();
    }

    private void animateCollapseNamedSlotId() {
        if (vRomIdNamedSlotId.getVisibility() == View.GONE) {
            return;
        }

        Animator animator = createCollapseLayoutAnimator(vRomIdNamedSlotId);
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
        if (mIsNamedSlot) {
            if (vRomIdSpinner.getSelectedItemPosition() == mRomIds.size() - 2) {
                return PatcherUtils.getDataSlotRomId(vRomIdNamedSlotId.getText().toString());
            } else if (vRomIdSpinner.getSelectedItemPosition() == mRomIds.size() - 1) {
                return PatcherUtils.getExtsdSlotRomId(vRomIdNamedSlotId.getText().toString());
            }
        } else {
            int pos = vRomIdSpinner.getSelectedItemPosition();
            if (pos >= 0) {
                InstallLocation[] locations = PatcherUtils.getInstallLocations(mContext);
                return locations[pos].id;
            }
        }
        return null;
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
}
