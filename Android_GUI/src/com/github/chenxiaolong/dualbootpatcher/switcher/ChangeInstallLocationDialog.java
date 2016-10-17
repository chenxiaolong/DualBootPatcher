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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.os.Bundle;
import android.support.annotation.NonNull;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

public class ChangeInstallLocationDialog extends DialogFragment {
    private static final String ARG_ZIP_ROM_ID = "zip_rom_id";

    public interface ChangeInstallLocationDialogListener {
        void onChangeInstallLocationClicked(boolean changeInstallLocation);
    }

    public static ChangeInstallLocationDialog newInstance(Fragment parent, String zipRomId) {
        if (parent != null) {
            if (!(parent instanceof ChangeInstallLocationDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement ChangeInstallLocationDialogListener");
            }
        }

        ChangeInstallLocationDialog frag = new ChangeInstallLocationDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putString(ARG_ZIP_ROM_ID, zipRomId);
        frag.setArguments(args);
        return frag;
    }

    ChangeInstallLocationDialogListener getOwner() {
        return (ChangeInstallLocationDialogListener) getTargetFragment();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        String zipRomId = getArguments().getString(ARG_ZIP_ROM_ID);
        String message = String.format(
                getString(R.string.in_app_flashing_change_install_location_desc), zipRomId);

        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .content(message)
                .positiveText(R.string.in_app_flashing_change_install_location)
                .negativeText(R.string.in_app_flashing_keep_current_location)
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        ChangeInstallLocationDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onChangeInstallLocationClicked(true);
                        }
                    }
                })
                .onNegative(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        ChangeInstallLocationDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onChangeInstallLocationClicked(false);
                        }
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
