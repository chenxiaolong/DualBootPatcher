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

public class ConfirmChecksumIssueDialog extends DialogFragment {
    public static final int CHECKSUM_INVALID = 1;
    public static final int CHECKSUM_MISSING = 2;

    private static final String ARG_ISSUE = "issue";
    private static final String ARG_ROM_ID = "rom_id";

    public interface ConfirmChecksumIssueDialogListener {
        void onConfirmChecksumIssue(String romId);
    }

    public static ConfirmChecksumIssueDialog newInstanceFromFragment(Fragment parent, int issue, String romId) {
        if (parent != null) {
            if (!(parent instanceof ConfirmChecksumIssueDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement ConfirmChecksumIssueDialogListener");
            }
        }

        switch (issue) {
        case CHECKSUM_INVALID:
        case CHECKSUM_MISSING:
            break;
        default:
            throw new IllegalArgumentException("Invalid value for issue param: " + issue);
        }

        ConfirmChecksumIssueDialog frag = new ConfirmChecksumIssueDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putInt(ARG_ISSUE, issue);
        args.putString(ARG_ROM_ID, romId);
        frag.setArguments(args);
        return frag;
    }

    public static ConfirmChecksumIssueDialog newInstanceFromActivity(int issue, String romId) {
        switch (issue) {
        case CHECKSUM_INVALID:
        case CHECKSUM_MISSING:
            break;
        default:
            throw new IllegalArgumentException("Invalid value for issue param: " + issue);
        }

        ConfirmChecksumIssueDialog frag = new ConfirmChecksumIssueDialog();
        Bundle args = new Bundle();
        args.putInt(ARG_ISSUE, issue);
        args.putString(ARG_ROM_ID, romId);
        frag.setArguments(args);
        return frag;
    }

    ConfirmChecksumIssueDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof ConfirmChecksumIssueDialogListener)) {
                throw new IllegalStateException(
                        "Parent activity must implement ConfirmChecksumIssueDialogListener");
            }
            return (ConfirmChecksumIssueDialogListener) getActivity();
        } else {
            return (ConfirmChecksumIssueDialogListener) getTargetFragment();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        int issue = getArguments().getInt(ARG_ISSUE);
        final String romId = getArguments().getString(ARG_ROM_ID);

        String message;
        switch (issue) {
        case CHECKSUM_INVALID:
            message = getString(R.string.checksums_invalid, romId);
            break;
        case CHECKSUM_MISSING:
            message = getString(R.string.checksums_missing, romId);
            break;
        default:
            // Not reached
            throw new IllegalArgumentException("Invalid value for issue param: " + issue);
        }

        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .content(message)
                .positiveText(R.string.proceed)
                .negativeText(R.string.cancel)
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        ConfirmChecksumIssueDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onConfirmChecksumIssue(romId);
                        }
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
