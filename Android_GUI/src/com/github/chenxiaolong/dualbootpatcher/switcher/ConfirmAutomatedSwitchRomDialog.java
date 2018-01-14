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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.widget.CheckBox;
import android.widget.TextView;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

public class ConfirmAutomatedSwitchRomDialog extends DialogFragment {
    private static final String ARG_ROM_ID = "rom_id";

    private ConfirmAutomatedSwitchRomDialogListener mListener;

    public interface ConfirmAutomatedSwitchRomDialogListener {
        void onConfirmSwitchRom(boolean dontShowAgain);

        void onCancelSwitchRom();
    }

    public static ConfirmAutomatedSwitchRomDialog newInstance(String romId) {
        ConfirmAutomatedSwitchRomDialog frag = new ConfirmAutomatedSwitchRomDialog();
        Bundle args = new Bundle();
        args.putString(ARG_ROM_ID, romId);
        frag.setArguments(args);
        return frag;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mListener = (ConfirmAutomatedSwitchRomDialogListener) getActivity();
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        String romId = getArguments().getString(ARG_ROM_ID);

        MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity())
                .customView(R.layout.dialog_first_time, true)
                .title(R.string.switching_rom)
                .positiveText(R.string.proceed)
                .negativeText(R.string.cancel)
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        CheckBox cb = (CheckBox) dialog.findViewById(R.id.checkbox);
                        if (mListener != null) {
                            mListener.onConfirmSwitchRom(cb.isChecked());
                        }
                    }
                })
                .onNegative(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        if (mListener != null) {
                            mListener.onCancelSwitchRom();
                        }
                    }
                });

        MaterialDialog dialog = builder.build();

        if (dialog.getCustomView() != null) {
            TextView tv = (TextView) dialog.getCustomView().findViewById(R.id.message);
            tv.setText(getString(R.string.confirm_automated_switch_rom, romId));
        }

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}