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

public class SetKernelNeededDialog extends DialogFragment {
    private static final String ARG_MESSAGE_RES_ID = "message_res_id";

    public interface SetKernelNeededDialogListener {
        void onConfirmSetKernelNeeded();
    }

    public static SetKernelNeededDialog newInstance(Fragment parent, int messageResId) {
        if (parent != null) {
            if (!(parent instanceof SetKernelNeededDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement SetKernelNeededDialogListener");
            }
        }

        SetKernelNeededDialog frag = new SetKernelNeededDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putInt(ARG_MESSAGE_RES_ID, messageResId);
        frag.setArguments(args);
        return frag;
    }

    SetKernelNeededDialogListener getOwner() {
        return (SetKernelNeededDialogListener) getTargetFragment();
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        int messageResId = getArguments().getInt(ARG_MESSAGE_RES_ID);

        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .content(messageResId)
                .positiveText(R.string.set_kernel_now)
                .negativeText(R.string.set_kernel_later)
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        SetKernelNeededDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onConfirmSetKernelNeeded();
                        }
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
