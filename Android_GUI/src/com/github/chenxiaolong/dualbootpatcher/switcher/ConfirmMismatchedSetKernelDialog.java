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
import android.support.v4.app.Fragment;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

public class ConfirmMismatchedSetKernelDialog extends DialogFragment {
    private static final String ARG_CURRENT_ROM = "current_rom";
    private static final String ARG_TARGET_ROM = "target_rom";

    public interface ConfirmMismatchedSetKernelDialogListener {
        void onConfirmMismatchedSetKernel();
    }

    public static ConfirmMismatchedSetKernelDialog newInstanceFromFragment(Fragment parent,
                                                                           String currentRom,
                                                                           String targetRom) {
        if (parent != null) {
            if (!(parent instanceof ConfirmMismatchedSetKernelDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement ConfirmMismatchedSetKernelDialogListener");
            }
        }

        ConfirmMismatchedSetKernelDialog frag = new ConfirmMismatchedSetKernelDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putString(ARG_CURRENT_ROM, currentRom);
        args.putString(ARG_TARGET_ROM, targetRom);
        frag.setArguments(args);
        return frag;
    }

    public static ConfirmMismatchedSetKernelDialog newInstanceFromActivity(String currentRom,
                                                                           String targetRom) {
        ConfirmMismatchedSetKernelDialog frag = new ConfirmMismatchedSetKernelDialog();
        Bundle args = new Bundle();
        args.putString(ARG_CURRENT_ROM, currentRom);
        args.putString(ARG_TARGET_ROM, targetRom);
        frag.setArguments(args);
        return frag;
    }

    ConfirmMismatchedSetKernelDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof ConfirmMismatchedSetKernelDialogListener)) {
                throw new IllegalStateException(
                        "Parent activity must implement ConfirmMismatchedSetKernelDialogListener");
            }
            return (ConfirmMismatchedSetKernelDialogListener) getActivity();
        } else {
            return (ConfirmMismatchedSetKernelDialogListener) getTargetFragment();
        }
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        String currentRom = getArguments().getString(ARG_CURRENT_ROM);
        String targetRom = getArguments().getString(ARG_TARGET_ROM);
        String message = String.format(
                getString(R.string.set_kernel_mismatched_rom), currentRom, targetRom);

        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .content(message)
                .positiveText(R.string.proceed)
                .negativeText(R.string.cancel)
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        ConfirmMismatchedSetKernelDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onConfirmMismatchedSetKernel();
                        }
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
