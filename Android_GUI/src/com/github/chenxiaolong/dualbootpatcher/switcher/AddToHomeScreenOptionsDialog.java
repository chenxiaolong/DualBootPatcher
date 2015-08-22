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
import android.app.DialogFragment;
import android.app.Fragment;
import android.os.Bundle;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

public class AddToHomeScreenOptionsDialog extends DialogFragment {
    public static final String TAG = AddToHomeScreenOptionsDialog.class.getSimpleName();

    private static final String ARG_ROM_INFO = "rom_info";

    public interface AddToHomeScreenOptionsDialogListener {
        void onConfirmAddToHomeScreenOptions(RomInformation info, boolean reboot);
    }

    public static AddToHomeScreenOptionsDialog newInstance(Fragment parent, RomInformation info) {
        if (parent != null) {
            if (!(parent instanceof AddToHomeScreenOptionsDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement AddToHomeScreenOptionsDialogListener");
            }
        }

        AddToHomeScreenOptionsDialog frag = new AddToHomeScreenOptionsDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putParcelable(ARG_ROM_INFO, info);
        frag.setArguments(args);
        return frag;
    }

    AddToHomeScreenOptionsDialogListener getOwner() {
        return (AddToHomeScreenOptionsDialogListener) getTargetFragment();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final RomInformation info = getArguments().getParcelable(ARG_ROM_INFO);

        MaterialDialog dialog = new MaterialDialog.Builder(getActivity())
                .content(R.string.auto_reboot_after_switching)
                .negativeText(R.string.no)
                .positiveText(R.string.yes)
                .callback(new ButtonCallback() {
                    @Override
                    public void onPositive(MaterialDialog dialog) {
                        AddToHomeScreenOptionsDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onConfirmAddToHomeScreenOptions(info, true);
                        }
                    }

                    @Override
                    public void onNegative(MaterialDialog dialog) {
                        AddToHomeScreenOptionsDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onConfirmAddToHomeScreenOptions(info, false);
                        }
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
