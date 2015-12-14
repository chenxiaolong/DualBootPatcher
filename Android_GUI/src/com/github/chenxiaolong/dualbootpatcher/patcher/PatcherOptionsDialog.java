/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.widget.AppCompatEditText;
import android.support.v7.widget.AppCompatSpinner;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.TextView;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils.InstallLocation;

import java.util.ArrayList;
import java.util.Collections;

public class PatcherOptionsDialog extends DialogFragment {
    private static final String ARG_ID = "id";
    private static final String ARG_PRESELECTED_DEVICE_ID = "preselected_device_id";
    private static final String ARG_PRESELECTED_ROM_ID = "rom_id";

    private MaterialDialog mDialog;
    private AppCompatSpinner mDeviceSpinner;
    private AppCompatSpinner mRomIdSpinner;
    private AppCompatEditText mRomIdNamedSlotId;
    private TextView mRomIdDesc;
    private View mDummy;

    private ArrayAdapter<String> mDeviceAdapter;
    private ArrayList<String> mDevices = new ArrayList<>();
    private ArrayAdapter<String> mRomIdAdapter;
    private ArrayList<String> mRomIds = new ArrayList<>();
    private ArrayList<InstallLocation> mInstallLocations = new ArrayList<>();

    private boolean mIsNamedSlot;

    public interface PatcherOptionsDialogListener {
        void onConfirmedOptions(int id, Device device, String romId);
    }

    public static PatcherOptionsDialog newInstanceFromFragment(Fragment parent, int id,
                                                               String preselectedDeviceId,
                                                               String preselectedRomId) {
        if (parent != null) {
            if (!(parent instanceof PatcherOptionsDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement PatcherOptionsDialogListener");
            }
        }

        PatcherOptionsDialog frag = new PatcherOptionsDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putInt(ARG_ID, id);
        args.putString(ARG_PRESELECTED_DEVICE_ID, preselectedDeviceId);
        args.putString(ARG_PRESELECTED_ROM_ID, preselectedRomId);
        frag.setArguments(args);
        return frag;
    }

    public static PatcherOptionsDialog newInstanceFromActivity(int id,
                                                               String preselectedDeviceId,
                                                               String preselectedRomId) {
        PatcherOptionsDialog frag = new PatcherOptionsDialog();
        Bundle args = new Bundle();
        args.putInt(ARG_ID, id);
        args.putString(ARG_PRESELECTED_DEVICE_ID, preselectedDeviceId);
        args.putString(ARG_PRESELECTED_ROM_ID, preselectedRomId);
        frag.setArguments(args);
        return frag;
    }

    PatcherOptionsDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof PatcherOptionsDialogListener)) {
                throw new IllegalStateException(
                        "Parent activity must implement PatcherOptionsDialogListener");
            }
            return (PatcherOptionsDialogListener) getActivity();
        } else {
            return (PatcherOptionsDialogListener) getTargetFragment();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final int id = getArguments().getInt(ARG_ID);
        String preselectedDeviceId = getArguments().getString(ARG_PRESELECTED_DEVICE_ID);
        String preselectedRomId = getArguments().getString(ARG_PRESELECTED_ROM_ID);

        mDialog = new MaterialDialog.Builder(getActivity())
                .title(R.string.patcher_options_dialog_title)
                .customView(R.layout.dialog_patcher_opts, true)
                .positiveText(R.string.proceed)
                .negativeText(R.string.cancel)
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        PatcherOptionsDialogListener owner = getOwner();
                        if (owner != null) {
                            int position = mDeviceSpinner.getSelectedItemPosition();
                            Device device = PatcherUtils.sPC.getDevices()[position];
                            owner.onConfirmedOptions(id, device, getRomId());
                        }
                    }
                })
                .build();

        View view = mDialog.getCustomView();
        mDeviceSpinner = (AppCompatSpinner) view.findViewById(R.id.spinner_device);
        mRomIdSpinner = (AppCompatSpinner) view.findViewById(R.id.spinner_rom_id);
        mRomIdNamedSlotId = (AppCompatEditText) view.findViewById(R.id.rom_id_named_slot_id);
        mRomIdDesc = (TextView) view.findViewById(R.id.rom_id_desc);
        mDummy = view.findViewById(R.id.customopts_dummylayout);

        // Initialize devices
        initDevices();
        // Initialize ROM IDs
        initRomIds();
        // Initialize actions
        initActions();

        // Load devices
        refreshDevices();
        // Load ROM IDs
        refreshRomIds();

        // Select our device on initial startup
        if (savedInstanceState == null) {
            if (preselectedDeviceId == null) {
                Device device = PatcherUtils.getCurrentDevice(getActivity(), PatcherUtils.sPC);
                if (device != null) {
                    preselectedDeviceId = device.getId();
                }
            }
            if (preselectedDeviceId != null) {
                selectDeviceId(preselectedDeviceId);
            }

            // Select initial ROM ID
            if (preselectedRomId != null) {
                selectRomId(preselectedRomId);
            }
        }

        setCancelable(false);
        mDialog.setCanceledOnTouchOutside(false);

        return mDialog;
    }

    private void initDevices() {
        mDeviceAdapter = new ArrayAdapter<>(getActivity(),
                android.R.layout.simple_spinner_item, android.R.id.text1, mDevices);
        mDeviceAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mDeviceSpinner.setAdapter(mDeviceAdapter);
    }

    private void initRomIds() {
        mRomIdAdapter = new ArrayAdapter<>(getActivity(),
                android.R.layout.simple_spinner_item, android.R.id.text1, mRomIds);
        mRomIdAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        mRomIdSpinner.setAdapter(mRomIdAdapter);

        mRomIdSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mDialog.getActionButton(DialogAction.POSITIVE).setEnabled(true);

                mIsNamedSlot = (mRomIds.size() >= 2
                        && (position == mRomIds.size() - 1
                        || position == mRomIds.size() - 2));

                onRomIdSelected(position);
                if (mIsNamedSlot) {
                    onNamedSlotIdTextChanged(mRomIdNamedSlotId.getText().toString());
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
            }
        });

        mRomIdNamedSlotId.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
            }

            @Override
            public void afterTextChanged(Editable s) {
                onNamedSlotIdTextChanged(s.toString());
            }
        });
    }

    private void initActions() {
        preventTextViewKeepFocus(mRomIdNamedSlotId);
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
                            getActivity().getSystemService(Context.INPUT_METHOD_SERVICE);
                    imm.hideSoftInputFromWindow(tv.getApplicationWindowToken(), 0);
                    mDummy.requestFocus();
                }
                return false;
            }
        });
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
        mInstallLocations.clear();
        Collections.addAll(mInstallLocations, PatcherUtils.getInstallLocations(getActivity()));
        Collections.addAll(mInstallLocations, PatcherUtils.getNamedInstallLocations(getActivity()));

        mRomIds.clear();
        for (InstallLocation location : mInstallLocations) {
            mRomIds.add(location.name);
        }
        mRomIds.add(getString(R.string.install_location_data_slot));
        mRomIds.add(getString(R.string.install_location_extsd_slot));
        mRomIdAdapter.notifyDataSetChanged();
    }

    private void selectDeviceId(String deviceId) {
        Device[] devices = PatcherUtils.sPC.getDevices();
        for (int i = 0; i < devices.length; i++) {
            Device device = devices[i];
            if (device.getId().equals(deviceId)) {
                mDeviceSpinner.setSelection(i);
                return;
            }
        }
    }

    private void selectRomId(String romId) {
        for (int i = 0; i < mInstallLocations.size(); i++) {
            InstallLocation location = mInstallLocations.get(i);
            if (location.id.equals(romId)) {
                mRomIdSpinner.setSelection(i);
                return;
            }
        }
        if (PatcherUtils.isDataSlotRomId(romId)) {
            mRomIdSpinner.setSelection(mRomIds.size() - 2);
            String namedId = PatcherUtils.getDataSlotIdFromRomId(romId);
            mRomIdNamedSlotId.setText(namedId);
            onNamedSlotIdChanged(namedId);
        } else if (PatcherUtils.isExtsdSlotRomId(romId)) {
            mRomIdSpinner.setSelection(mRomIds.size() - 1);
            String namedId = PatcherUtils.getExtsdSlotIdFromRomId(romId);
            mRomIdNamedSlotId.setText(namedId);
            onNamedSlotIdChanged(namedId);
        }
    }

    private void onRomIdSelected(int position) {
        if (mIsNamedSlot) {
            onNamedSlotIdChanged(mRomIdNamedSlotId.getText().toString());
            mRomIdNamedSlotId.setVisibility(View.VISIBLE);
        } else {
            mRomIdDesc.setText(mInstallLocations.get(position).description);
            mRomIdNamedSlotId.setVisibility(View.GONE);
        }
    }

    private void onNamedSlotIdTextChanged(String text) {
        if (!mIsNamedSlot) {
            return;
        }

        if (text.isEmpty()) {
            mRomIdNamedSlotId.setError(getString(
                    R.string.install_location_named_slot_id_error_is_empty));
            mDialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);
        } else if (!text.matches("[a-z0-9]+")) {
            mRomIdNamedSlotId.setError(getString(
                    R.string.install_location_named_slot_id_error_invalid));
            mDialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);
        } else {
            mDialog.getActionButton(DialogAction.POSITIVE).setEnabled(true);
        }

        onNamedSlotIdChanged(text);
    }

    private void onNamedSlotIdChanged(String text) {
        if (text.isEmpty()) {
            mRomIdDesc.setText(R.string.install_location_named_slot_id_empty);
        } else if (mRomIdSpinner.getSelectedItemPosition() == mRomIds.size() - 2) {
            InstallLocation location = PatcherUtils.getDataSlotInstallLocation(getActivity(), text);
            mRomIdDesc.setText(location.description);
        } else if (mRomIdSpinner.getSelectedItemPosition() == mRomIds.size() - 1) {
            InstallLocation location = PatcherUtils.getExtsdSlotInstallLocation(getActivity(), text);
            mRomIdDesc.setText(location.description);
        }
    }

    private String getRomId() {
        if (mIsNamedSlot) {
            if (mRomIdSpinner.getSelectedItemPosition() == mRomIds.size() - 2) {
                return PatcherUtils.getDataSlotRomId(mRomIdNamedSlotId.getText().toString());
            } else if (mRomIdSpinner.getSelectedItemPosition() == mRomIds.size() - 1) {
                return PatcherUtils.getExtsdSlotRomId(mRomIdNamedSlotId.getText().toString());
            }
        } else {
            int pos = mRomIdSpinner.getSelectedItemPosition();
            if (pos >= 0) {
                return mInstallLocations.get(pos).id;
            }
        }
        return null;
    }
}
