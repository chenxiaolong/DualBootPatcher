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
import android.app.DialogFragment;
import android.app.Fragment;
import android.os.Bundle;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

public class GenericConfirmDialog extends DialogFragment {
    private static final String ARG_ID = "id";
    private static final String ARG_TITLE = "title";
    private static final String ARG_MESSAGE = "message";
    private static final String ARG_BUTTON_TEXT = "buttonText";

    public interface GenericConfirmDialogListener {
        void onConfirmOk(int id);
    }

    public static GenericConfirmDialog newInstanceFromFragment(Fragment parent, int id, String title,
                                                               String message, String buttonText) {
        GenericConfirmDialog frag = new GenericConfirmDialog();
        if (parent instanceof GenericConfirmDialogListener) {
            frag.setTargetFragment(parent, 0);
        }
        Bundle args = new Bundle();
        args.putInt(ARG_ID, id);
        args.putString(ARG_TITLE, title);
        args.putString(ARG_MESSAGE, message);
        args.putString(ARG_BUTTON_TEXT, buttonText);
        frag.setArguments(args);
        return frag;
    }

    public static GenericConfirmDialog newInstanceFromActivity(int id, String title, String message,
                                                               String buttonText) {
        GenericConfirmDialog frag = new GenericConfirmDialog();
        Bundle args = new Bundle();
        args.putInt(ARG_ID, id);
        args.putString(ARG_TITLE, title);
        args.putString(ARG_MESSAGE, message);
        args.putString(ARG_BUTTON_TEXT, buttonText);
        frag.setArguments(args);
        return frag;
    }

    GenericConfirmDialogListener getOwner() {
        if (getTargetFragment() instanceof GenericConfirmDialogListener) {
            return (GenericConfirmDialogListener) getTargetFragment();
        } else if (getActivity() instanceof GenericConfirmDialogListener) {
            return (GenericConfirmDialogListener) getActivity();
        } else {
            return null;
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final int id = getArguments().getInt(ARG_ID);
        String title = getArguments().getString(ARG_TITLE);
        String message = getArguments().getString(ARG_MESSAGE);
        String buttonText = getArguments().getString(ARG_BUTTON_TEXT);

        MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity());

        if (title != null) {
            builder.title(title);
        }
        if (message != null) {
            builder.content(message);
        }
        if (buttonText != null) {
            builder.positiveText(buttonText);
        } else {
            builder.positiveText(R.string.ok);
        }

        builder.callback(new ButtonCallback() {
            @Override
            public void onPositive(MaterialDialog dialog) {
                GenericConfirmDialogListener owner = getOwner();
                if (owner != null) {
                    owner.onConfirmOk(id);
                }
            }
        });

        Dialog dialog = builder.build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
