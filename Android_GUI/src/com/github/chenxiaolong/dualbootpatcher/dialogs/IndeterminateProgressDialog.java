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
import android.os.Bundle;

import com.afollestad.materialdialogs.MaterialDialog;
import com.github.chenxiaolong.dualbootpatcher.R;

public class IndeterminateProgressDialog extends DialogFragment {
    public static final String TAG = IndeterminateProgressDialog.class.getSimpleName();

    public static IndeterminateProgressDialog newInstance() {
        return new IndeterminateProgressDialog();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .content(R.string.please_wait)
                .progress(true, 0)
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}