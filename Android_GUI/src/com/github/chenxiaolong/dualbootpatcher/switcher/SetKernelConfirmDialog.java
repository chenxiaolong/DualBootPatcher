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
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

public class SetKernelConfirmDialog extends DialogFragment {
    private static final String ARG_ROM = "rom";

    public interface SetKernelConfirmDialogListener {
        void onConfirmSetKernel();
    }

    public static SetKernelConfirmDialog newInstanceFromFragment(Fragment parent, RomInformation info) {
        if (parent != null) {
            if (!(parent instanceof SetKernelConfirmDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement SetKernelConfirmDialogListener");
            }
        }

        SetKernelConfirmDialog frag = new SetKernelConfirmDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putParcelable(ARG_ROM, info);
        frag.setArguments(args);
        return frag;
    }

    public static SetKernelConfirmDialog newInstanceFromActivity(RomInformation info) {
        SetKernelConfirmDialog frag = new SetKernelConfirmDialog();
        Bundle args = new Bundle();
        args.putParcelable(ARG_ROM, info);
        frag.setArguments(args);
        return frag;
    }

    SetKernelConfirmDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof SetKernelConfirmDialogListener)) {
                throw new IllegalStateException(
                        "Parent activity must implement SetKernelConfirmDialogListener");
            }
            return (SetKernelConfirmDialogListener) getActivity();
        } else {
            return (SetKernelConfirmDialogListener) getTargetFragment();
        }
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        RomInformation info = getArguments().getParcelable(ARG_ROM);

        String message = String.format(getString(R.string.switcher_ask_set_kernel_desc),
                info.getName());

        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .title(R.string.switcher_ask_set_kernel_title)
                .content(message)
                .positiveText(R.string.proceed)
                .negativeText(R.string.cancel)
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        SetKernelConfirmDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onConfirmSetKernel();
                        }
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
