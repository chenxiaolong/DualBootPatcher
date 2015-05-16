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
import android.os.Bundle;

import com.afollestad.materialdialogs.MaterialDialog;
import com.github.chenxiaolong.dualbootpatcher.R;

public class GenericConfirmDialog extends DialogFragment {
    public static final String TAG = GenericConfirmDialog.class.getSimpleName();

    private static final String ARG_TITLE = "title";
    private static final String ARG_MESSAGE = "message";
    private static final String ARG_TITLE_RES_ID = "titleResId";
    private static final String ARG_MESSAGE_RES_ID = "messageResId";

    public static GenericConfirmDialog newInstance(String title, String message) {
        GenericConfirmDialog frag = new GenericConfirmDialog();
        Bundle args = new Bundle();
        args.putString(ARG_TITLE, title);
        args.putString(ARG_MESSAGE, message);
        frag.setArguments(args);
        return frag;
    }

    public static GenericConfirmDialog newInstance(int titleResId, int messageResId) {
        GenericConfirmDialog frag = new GenericConfirmDialog();
        Bundle args = new Bundle();
        args.putInt(ARG_TITLE_RES_ID, titleResId);
        args.putInt(ARG_MESSAGE_RES_ID, messageResId);
        frag.setArguments(args);
        return frag;
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        String title = getArguments().getString(ARG_TITLE);
        String message = getArguments().getString(ARG_MESSAGE);
        int titleResId = getArguments().getInt(ARG_TITLE_RES_ID);
        int messageResId = getArguments().getInt(ARG_MESSAGE_RES_ID);

        MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity())
                .positiveText(R.string.ok);

        if (title != null) {
            builder.title(title);
        } else if (titleResId > 0) {
            builder.title(titleResId);
        }

        if (message != null) {
            builder.content(message);
        } else if (messageResId > 0) {
            builder.content(messageResId);
        }

        Dialog dialog = builder.build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
