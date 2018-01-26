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
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

public class UpdateRamdiskResultDialog extends DialogFragment {
    private static final String ARG_SUCCEEDED = "succeeded";
    private static final String ARG_ALLOW_REBOOT = "allow_reboot";

    public interface UpdateRamdiskResultDialogListener {
        void onConfirmUpdatedRamdisk(boolean reboot);
    }

    public static UpdateRamdiskResultDialog newInstanceFromFragment(Fragment parent,
                                                                    boolean succeeded,
                                                                    boolean allowReboot) {
        if (parent != null) {
            if (!(parent instanceof UpdateRamdiskResultDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement UpdateRamdiskResultDialogListener");
            }
        }

        UpdateRamdiskResultDialog frag = new UpdateRamdiskResultDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putBoolean(ARG_SUCCEEDED, succeeded);
        args.putBoolean(ARG_ALLOW_REBOOT, allowReboot);
        frag.setArguments(args);
        return frag;
    }

    public static UpdateRamdiskResultDialog newInstanceFromActivity(boolean succeeded,
                                                                    boolean allowReboot) {
        UpdateRamdiskResultDialog frag = new UpdateRamdiskResultDialog();
        Bundle args = new Bundle();
        args.putBoolean(ARG_SUCCEEDED, succeeded);
        args.putBoolean(ARG_ALLOW_REBOOT, allowReboot);
        frag.setArguments(args);
        return frag;
    }

    UpdateRamdiskResultDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof UpdateRamdiskResultDialogListener)) {
                throw new IllegalStateException(
                        "Parent activity must implement UpdateRamdiskResultDialogListener");
            }
            return (UpdateRamdiskResultDialogListener) getActivity();
        } else {
            return (UpdateRamdiskResultDialogListener) getTargetFragment();
        }
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        boolean succeeded = getArguments().getBoolean(ARG_SUCCEEDED);
        boolean allowReboot = getArguments().getBoolean(ARG_ALLOW_REBOOT);

        MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity());

        if (!succeeded) {
            builder.title(R.string.update_ramdisk_failure_title);
            builder.content(R.string.update_ramdisk_failure_desc);

            builder.negativeText(R.string.ok);
        } else {
            builder.title(R.string.update_ramdisk_success_title);

            if (allowReboot) {
                builder.content(R.string.update_ramdisk_reboot_desc);
                builder.negativeText(R.string.reboot_later);
                builder.positiveText(R.string.reboot_now);

                builder.onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        confirm(true);
                    }
                });
                builder.onNegative(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        confirm(false);
                    }
                });
            } else {
                builder.content(R.string.update_ramdisk_no_reboot_desc);
                builder.positiveText(R.string.ok);

                builder.onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        confirm(false);
                    }
                });
            }
        }

        Dialog dialog = builder.build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }

    private void confirm(boolean reboot) {
        UpdateRamdiskResultDialogListener owner = getOwner();
        if (owner != null) {
            owner.onConfirmUpdatedRamdisk(reboot);
        }
    }
}
