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

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackMultiChoice;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams.Action;

import java.util.ArrayList;

public class BackupRestoreTargetsSelectionDialog extends DialogFragment {
    private static final String ARG_ACTION = "action";

    public interface BackupRestoreTargetsSelectionDialogListener {
        void onSelectedBackupRestoreTargets(String[] targets);
    }

    public static BackupRestoreTargetsSelectionDialog newInstanceFromFragment(Fragment parent, Action action) {
        if (parent != null) {
            if (!(parent instanceof BackupRestoreTargetsSelectionDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement BackupRestoreTargetsSelectionDialogListener");
            }
        }

        Bundle args = new Bundle();
        args.putSerializable(ARG_ACTION, action);

        BackupRestoreTargetsSelectionDialog frag = new BackupRestoreTargetsSelectionDialog();
        frag.setTargetFragment(parent, 0);
        frag.setArguments(args);
        return frag;
    }

    public static BackupRestoreTargetsSelectionDialog newInstanceFromActivity(Action action) {
        Bundle args = new Bundle();
        args.putSerializable(ARG_ACTION, action);

        BackupRestoreTargetsSelectionDialog frag = new BackupRestoreTargetsSelectionDialog();
        frag.setArguments(args);
        return frag;
    }

    BackupRestoreTargetsSelectionDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof BackupRestoreTargetsSelectionDialogListener)) {
                throw new IllegalStateException(
                        "Parent activity must implement BackupRestoreTargetsSelectionDialogListener");
            }
            return (BackupRestoreTargetsSelectionDialogListener) getActivity();
        } else {
            return (BackupRestoreTargetsSelectionDialogListener) getTargetFragment();
        }
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final String[] items = new String[] {
                getString(R.string.br_target_system),
                getString(R.string.br_target_cache),
                getString(R.string.br_target_data),
                getString(R.string.br_target_boot),
                getString(R.string.br_target_config)
        };

        Action action = (Action) getArguments().getSerializable(ARG_ACTION);
        int descResid = 0;

        if (action == Action.BACKUP) {
            descResid = R.string.br_backup_targets_dialog_desc;
        } else if (action == Action.RESTORE) {
            descResid = R.string.br_restore_targets_dialog_desc;
        }

        final ArrayList<String> selected = new ArrayList<>(items.length);

        MaterialDialog dialog = new MaterialDialog.Builder(getActivity())
                .content(descResid)
                .items((CharSequence[]) items)
                .negativeText(R.string.cancel)
                .positiveText(R.string.ok)
                .itemsCallbackMultiChoice(null, new ListCallbackMultiChoice() {
                    @Override
                    public boolean onSelection(MaterialDialog dialog, Integer[] which,
                                               CharSequence[] text) {
                        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(which.length > 0);

                        selected.clear();
                        for (Integer arrIndex : which) {
                            if (arrIndex == 0) {
                                selected.add("system");
                            } else if (arrIndex == 1) {
                                selected.add("cache");
                            } else if (arrIndex == 2) {
                                selected.add("data");
                            } else if (arrIndex == 3) {
                                selected.add("boot");
                            } else if (arrIndex == 4) {
                                selected.add("config");
                            }
                        }

                        return true;
                    }
                })
                .alwaysCallMultiChoiceCallback()
                .onPositive(new SingleButtonCallback() {
                    @Override
                    public void onClick(@NonNull MaterialDialog dialog,
                                        @NonNull DialogAction which) {
                        BackupRestoreTargetsSelectionDialogListener owner = getOwner();
                        if (owner != null) {
                            String[] targets = selected.toArray(new String[selected.size()]);
                            owner.onSelectedBackupRestoreTargets(targets);
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
