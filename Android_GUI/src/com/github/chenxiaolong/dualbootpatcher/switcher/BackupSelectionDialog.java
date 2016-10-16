/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.view.View;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackSingleChoice;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;

import java.io.File;
import java.util.ArrayList;

public class BackupSelectionDialog extends DialogFragment {
    private static final String ARG_BACKUP_DIR = "action";

    public interface BackupSelectionDialogListener {
        void onSelectedBackup(String backupDir, String backupName);
    }

    public static BackupSelectionDialog newInstanceFromFragment(Fragment parent, String backupDir) {
        if (parent != null) {
            if (!(parent instanceof BackupSelectionDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement BackupSelectionDialogListener");
            }
        }

        Bundle args = new Bundle();
        args.putSerializable(ARG_BACKUP_DIR, backupDir);

        BackupSelectionDialog frag = new BackupSelectionDialog();
        frag.setTargetFragment(parent, 0);
        frag.setArguments(args);
        return frag;
    }

    public static BackupSelectionDialog newInstanceFromActivity(String backupDir) {
        Bundle args = new Bundle();
        args.putSerializable(ARG_BACKUP_DIR, backupDir);

        BackupSelectionDialog frag = new BackupSelectionDialog();
        frag.setArguments(args);
        return frag;
    }

    BackupSelectionDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof BackupSelectionDialogListener)) {
                throw new IllegalStateException(
                        "Parent activity must implement BackupSelectionDialogListener");
            }
            return (BackupSelectionDialogListener) getActivity();
        } else {
            return (BackupSelectionDialogListener) getTargetFragment();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final String backupDir = getArguments().getString(ARG_BACKUP_DIR);
        final ArrayList<String> filenames = new ArrayList<>();
        File[] files = new File(backupDir).listFiles();

        if (files != null) {
            for (File file : files) {
                if (file.isDirectory()) {
                    filenames.add(file.getName());
                }
            }
        }

        MaterialDialog dialog = new MaterialDialog.Builder(getActivity())
                .content(R.string.in_app_flashing_select_backup_dialog_desc)
                .items(filenames)
                .negativeText(R.string.cancel)
                .positiveText(R.string.ok)
                .itemsCallbackSingleChoice(-1, new ListCallbackSingleChoice() {
                    @Override
                    public boolean onSelection(MaterialDialog dialog, View itemView, int which,
                                               CharSequence text) {
                        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(true);
                        return true;
                    }
                })
                .alwaysCallSingleChoiceCallback()
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        BackupSelectionDialogListener owner = getOwner();
                        if (owner != null) {
                            String name = filenames.get(dialog.getSelectedIndex());
                            owner.onSelectedBackup(backupDir, name);
                        }
                    }
                })
                .build();

        // No targets selected by default, so don't enable OK button
        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
