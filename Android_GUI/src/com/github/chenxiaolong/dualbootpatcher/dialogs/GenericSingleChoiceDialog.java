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
import android.support.annotation.NonNull;
import android.view.View;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackSingleChoice;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;

public class GenericSingleChoiceDialog extends DialogFragment {
    private static final String ARG_ID = "id";
    private static final String ARG_TITLE = "title";
    private static final String ARG_MESSAGE = "message";
    private static final String ARG_POSITIVE = "positive";
    private static final String ARG_NEGATIVE = "negative";
    private static final String ARG_CHOICES = "choices";

    public interface GenericSingleChoiceDialogListener {
        void onConfirmSingleChoice(int id, int index, String text);
    }

    public static GenericSingleChoiceDialog newInstanceFromFragment(Fragment parent, int id,
                                                                    String title, String message,
                                                                    String positive,
                                                                    String negative,
                                                                    String[] choices) {
        if (parent != null) {
            if (!(parent instanceof GenericSingleChoiceDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement GenericSingleChoiceDialogListener");
            }
        }

        GenericSingleChoiceDialog frag = new GenericSingleChoiceDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putInt(ARG_ID, id);
        args.putString(ARG_TITLE, title);
        args.putString(ARG_MESSAGE, message);
        args.putString(ARG_POSITIVE, positive);
        args.putString(ARG_NEGATIVE, negative);
        args.putStringArray(ARG_CHOICES, choices);
        frag.setArguments(args);
        return frag;
    }

    public static GenericSingleChoiceDialog newInstanceFromActivity(int id,
                                                                    String title, String message,
                                                                    String positive,
                                                                    String negative,
                                                                    String[] choices) {
        GenericSingleChoiceDialog frag = new GenericSingleChoiceDialog();
        Bundle args = new Bundle();
        args.putInt(ARG_ID, id);
        args.putString(ARG_TITLE, title);
        args.putString(ARG_MESSAGE, message);
        args.putString(ARG_POSITIVE, positive);
        args.putString(ARG_NEGATIVE, negative);
        args.putStringArray(ARG_CHOICES, choices);
        frag.setArguments(args);
        return frag;
    }

    GenericSingleChoiceDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof GenericSingleChoiceDialogListener)) {
                throw new IllegalStateException(
                        "Activity must implement GenericSingleChoiceDialogListener");
            }
            return (GenericSingleChoiceDialogListener) getActivity();
        } else {
            return (GenericSingleChoiceDialogListener) getTargetFragment();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final int id = getArguments().getInt(ARG_ID);
        String title = getArguments().getString(ARG_TITLE);
        String message = getArguments().getString(ARG_MESSAGE);
        String positive = getArguments().getString(ARG_POSITIVE);
        String negative = getArguments().getString(ARG_NEGATIVE);
        final String[] choices = getArguments().getStringArray(ARG_CHOICES);

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

        builder.items((CharSequence[]) choices);
        builder.itemsCallbackSingleChoice(-1, new ListCallbackSingleChoice() {
            @Override
            public boolean onSelection(MaterialDialog dialog, View itemView, int which,
                                       CharSequence text) {
                dialog.getActionButton(DialogAction.POSITIVE).setEnabled(true);
                return true;
            }
        });
        builder.alwaysCallSingleChoiceCallback();

        builder.onPositive(new SingleButtonCallback() {
            @Override
            public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
                GenericSingleChoiceDialogListener owner = getOwner();
                if (owner != null) {
                    int index = dialog.getSelectedIndex();
                    String text = choices[index];
                    owner.onConfirmSingleChoice(id, index, text);
                }
            }
        });

        MaterialDialog dialog = builder.build();

        // Nothing selected by default, so don't enable OK button
        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
