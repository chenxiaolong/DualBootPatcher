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
import android.text.InputType;
import android.widget.EditText;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.InputCallback;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

public class BackupNameInputDialog extends DialogFragment {
    private static final String ARG_SUGGESTED_NAME = "suggested_name";

    public interface BackupNameInputDialogListener {
        void onSelectedBackupName(String name);
    }

    public static BackupNameInputDialog newInstanceFromFragment(Fragment parent,
                                                                String suggestedName) {
        if (parent != null) {
            if (!(parent instanceof BackupNameInputDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement BackupNameInputDialogListener");
            }
        }

        Bundle args = new Bundle();
        args.putString(ARG_SUGGESTED_NAME, suggestedName);

        BackupNameInputDialog frag = new BackupNameInputDialog();
        frag.setTargetFragment(parent, 0);
        frag.setArguments(args);
        return frag;
    }

    public static BackupNameInputDialog newInstanceFromActivity(String suggestedName) {
        Bundle args = new Bundle();
        args.putString(ARG_SUGGESTED_NAME, suggestedName);

        BackupNameInputDialog frag = new BackupNameInputDialog();
        frag.setArguments(args);
        return frag;
    }

    BackupNameInputDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof BackupNameInputDialogListener)) {
                throw new IllegalStateException(
                        "Parent activity must implement BackupNameInputDialogListener");
            }
            return (BackupNameInputDialogListener) getActivity();
        } else {
            return (BackupNameInputDialogListener) getTargetFragment();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        String suggestedName = getArguments().getString(ARG_SUGGESTED_NAME);

        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .content(R.string.br_name_dialog_desc)
                .negativeText(R.string.cancel)
                .positiveText(R.string.ok)
                .inputType(InputType.TYPE_TEXT_FLAG_AUTO_COMPLETE
                        | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
                .input(getString(R.string.br_name_dialog_hint), suggestedName, false,
                        new InputCallback() {
                    @Override
                    public void onInput(@NonNull MaterialDialog dialog, CharSequence input) {
                        String text = input.toString();
                        if (text.contains("/") || text.equals(".") || text.equals("..")) {
                            dialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);
                        }
                    }
                })
                .alwaysCallInputCallback()
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        BackupNameInputDialogListener owner = getOwner();
                        if (owner != null) {
                            EditText editText = dialog.getInputEditText();
                            if (editText != null) {
                                owner.onSelectedBackupName(editText.getText().toString());
                            }
                        }
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
