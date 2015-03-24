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

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

public class ExperimentalInAppWipeDialog extends DialogFragment {
    public static final String TAG = ExperimentalInAppWipeDialog.class.getSimpleName();

    public interface ExperimentalInAppWipeDialogListener {
        void onConfirmInAppRomWipeWarning();
    }

    public static ExperimentalInAppWipeDialog newInstance(Fragment parent) {
        if (parent != null) {
            if (!(parent instanceof ExperimentalInAppWipeDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement ExperimentalInAppWipeDialogListener");
            }
        }

        ExperimentalInAppWipeDialog frag = new ExperimentalInAppWipeDialog();
        frag.setTargetFragment(parent, 0);
        return frag;
    }

    ExperimentalInAppWipeDialogListener getOwner() {
        return (ExperimentalInAppWipeDialogListener) getTargetFragment();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .content(R.string.wipe_rom_dialog_warning_message)
                .positiveText(R.string.proceed)
                .callback(new ButtonCallback() {
                    @Override
                    public void onPositive(MaterialDialog dialog) {
                        ExperimentalInAppWipeDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onConfirmInAppRomWipeWarning();
                        }
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
