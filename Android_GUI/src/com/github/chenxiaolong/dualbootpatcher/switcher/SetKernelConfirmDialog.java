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
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

public class SetKernelConfirmDialog extends DialogFragment {
    public static final String TAG = SetKernelConfirmDialog.class.getSimpleName();

    private static final String ARG_ROM = "rom";

    public interface SetKernelConfirmDialogListener {
        void onConfirmSetKernel();
    }

    public static SetKernelConfirmDialog newInstance(Fragment parent, RomInformation info) {
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

    SetKernelConfirmDialogListener getOwner() {
        return (SetKernelConfirmDialogListener) getTargetFragment();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        RomInformation info = getArguments().getParcelable(ARG_ROM);

        String message = String.format(getString(R.string.switcher_ask_set_kernel_desc),
                info.getName());

        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .title(R.string.switcher_ask_set_kernel_title)
                .content(message)
                .positiveText(R.string.proceed)
                .negativeText(R.string.cancel)
                .callback(new ButtonCallback() {
                    @Override
                    public void onPositive(MaterialDialog dialog) {
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
