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

package com.github.chenxiaolong.dualbootpatcher.dialogs;

import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.os.Bundle;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ButtonCallback;

public class GenericYesNoDialog extends DialogFragment {
    private static final String ARG_ID = "id";
    private static final String ARG_TITLE = "title";
    private static final String ARG_MESSAGE = "message";
    private static final String ARG_POSITIVE = "positive";
    private static final String ARG_NEGATIVE = "negative";

    public interface GenericYesNoDialogListener {
        void onConfirmYesNo(int id, boolean choice);
    }

    public static GenericYesNoDialog newInstanceFromFragment(Fragment parent, int id, String title,
                                                             String message, String positive,
                                                             String negative) {
        if (parent != null) {
            if (!(parent instanceof GenericYesNoDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement GenericYesNoDialogListener");
            }
        }

        GenericYesNoDialog frag = new GenericYesNoDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putInt(ARG_ID, id);
        args.putString(ARG_TITLE, title);
        args.putString(ARG_MESSAGE, message);
        args.putString(ARG_POSITIVE, positive);
        args.putString(ARG_NEGATIVE, negative);
        frag.setArguments(args);
        return frag;
    }

    public static GenericYesNoDialog newInstanceFromActivity(int id, String title, String message,
                                                             String positive, String negative) {
        GenericYesNoDialog frag = new GenericYesNoDialog();
        Bundle args = new Bundle();
        args.putInt(ARG_ID, id);
        args.putString(ARG_TITLE, title);
        args.putString(ARG_MESSAGE, message);
        args.putString(ARG_POSITIVE, positive);
        args.putString(ARG_NEGATIVE, negative);
        frag.setArguments(args);
        return frag;
    }

    GenericYesNoDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof GenericYesNoDialogListener)) {
                throw new IllegalStateException(
                        "Activity must implement GenericYesNoDialogListener");
            }
            return (GenericYesNoDialogListener) getActivity();
        } else {
            return (GenericYesNoDialogListener) getTargetFragment();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final int id = getArguments().getInt(ARG_ID);
        String title = getArguments().getString(ARG_TITLE);
        String message = getArguments().getString(ARG_MESSAGE);
        String positive = getArguments().getString(ARG_POSITIVE);
        String negative = getArguments().getString(ARG_NEGATIVE);

        MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity());

        if (title != null) {
            builder.title(title);
        }
        if (message != null) {
            builder.content(message);
        }
        if (positive != null) {
            builder.positiveText(positive);
        }
        if (negative != null) {
            builder.negativeText(negative);
        }

        builder.callback(new ButtonCallback() {
            @Override
            public void onPositive(MaterialDialog dialog) {
                GenericYesNoDialogListener owner = getOwner();
                if (owner != null) {
                    owner.onConfirmYesNo(id, true);
                }
            }

            @Override
            public void onNegative(MaterialDialog dialog) {
                GenericYesNoDialogListener owner = getOwner();
                if (owner != null) {
                    owner.onConfirmYesNo(id, false);
                }
            }
        });

        Dialog dialog = builder.build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
