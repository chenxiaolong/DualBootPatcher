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

package com.github.chenxiaolong.dualbootpatcher.dialogs;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.widget.CheckBox;
import android.widget.TextView;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

public class FirstUseDialog extends DialogFragment {
    private static final String ARG_TITLE_RES_ID = "titleResid";
    private static final String ARG_MESSAGE_RES_ID = "messageResId";

    public interface FirstUseDialogListener {
        void onConfirmFirstUse(boolean dontShowAgain);
    }

    public static FirstUseDialog newInstance(Fragment parent, int titleResId, int messageResId) {
        if (parent != null) {
            if (!(parent instanceof FirstUseDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement FirstUseDialogListener");
            }
        }

        FirstUseDialog frag = new FirstUseDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putInt(ARG_TITLE_RES_ID, titleResId);
        args.putInt(ARG_MESSAGE_RES_ID, messageResId);
        frag.setArguments(args);
        return frag;
    }

    FirstUseDialogListener getOwner() {
        return (FirstUseDialogListener) getTargetFragment();
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        int titleResId = getArguments().getInt(ARG_TITLE_RES_ID);
        int messageResId = getArguments().getInt(ARG_MESSAGE_RES_ID);

        MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity())
                .customView(R.layout.dialog_first_time, true)
                .positiveText(R.string.ok)
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        CheckBox cb = (CheckBox) dialog.findViewById(R.id.checkbox);

                        FirstUseDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onConfirmFirstUse(cb.isChecked());
                        }
                    }
                });

        if (titleResId != 0) {
            builder.title(titleResId);
        }

        Dialog dialog = builder.build();

        if (messageResId != 0) {
            TextView tv = (TextView)
                    ((MaterialDialog) dialog).getCustomView().findViewById(R.id.message);
            tv.setText(messageResId);
        }

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
