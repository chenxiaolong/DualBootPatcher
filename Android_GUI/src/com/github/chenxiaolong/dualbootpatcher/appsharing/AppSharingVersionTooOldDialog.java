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

package com.github.chenxiaolong.dualbootpatcher.appsharing;

import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.os.Bundle;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

public class AppSharingVersionTooOldDialog extends DialogFragment {
    public static final String TAG = AppSharingVersionTooOldDialog.class.getSimpleName();

    public interface AppSharingVersionTooOldDialogListener {
        void onConfirmVersionTooOld();
    }

    public static AppSharingVersionTooOldDialog newInstance(Fragment parent) {
        if (parent != null) {
            if (!(parent instanceof AppSharingVersionTooOldDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement AppSharingVersionTooOldDialogListener");
            }
        }

        AppSharingVersionTooOldDialog frag = new AppSharingVersionTooOldDialog();
        frag.setTargetFragment(parent, 0);
        return frag;
    }

    AppSharingVersionTooOldDialogListener getOwner() {
        return (AppSharingVersionTooOldDialogListener) getTargetFragment();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .content(R.string.a_s_settings_version_too_old)
                .positiveText(R.string.ok)
                .callback(new ButtonCallback() {
                    @Override
                    public void onPositive(MaterialDialog dialog) {
                        AppSharingVersionTooOldDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onConfirmVersionTooOld();
                        }
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
