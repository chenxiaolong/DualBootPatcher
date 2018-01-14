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

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackMultiChoice;
import com.github.chenxiaolong.dualbootpatcher.R;

import mbtool.daemon.v3.MbWipeTarget;

public class WipeTargetsSelectionDialog extends DialogFragment {
    public interface WipeTargetsSelectionDialogListener {
        void onSelectedWipeTargets(short[] targets);
    }

    public static WipeTargetsSelectionDialog newInstanceFromFragment(Fragment parent) {
        if (parent != null) {
            if (!(parent instanceof WipeTargetsSelectionDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement WipeTargetsSelectionDialogListener");
            }
        }

        WipeTargetsSelectionDialog frag = new WipeTargetsSelectionDialog();
        frag.setTargetFragment(parent, 0);
        return frag;
    }

    public static WipeTargetsSelectionDialog newInstanceFromActivity() {
        return new WipeTargetsSelectionDialog();
    }

    WipeTargetsSelectionDialogListener getOwner() {
        Fragment f = getTargetFragment();
        if (f == null) {
            if (!(getActivity() instanceof WipeTargetsSelectionDialogListener)) {
                throw new IllegalStateException(
                        "Parent activity must implement WipeTargetsSelectionDialogListener");
            }
            return (WipeTargetsSelectionDialogListener) getActivity();
        } else {
            return (WipeTargetsSelectionDialogListener) getTargetFragment();
        }
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final String[] items = new String[] {
                getString(R.string.wipe_target_system),
                getString(R.string.wipe_target_cache),
                getString(R.string.wipe_target_data),
                getString(R.string.wipe_target_dalvik_cache),
                getString(R.string.wipe_target_multiboot_files)
        };

        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .title(R.string.wipe_rom_dialog_title)
                .items(items)
                .negativeText(R.string.cancel)
                .positiveText(R.string.ok)
                .itemsCallbackMultiChoice(null, new ListCallbackMultiChoice() {
                    @Override
                    public boolean onSelection(MaterialDialog dialog, Integer[] which,
                                               CharSequence[] text) {
                        short[] targets = new short[which.length];

                        for (int i = 0; i < which.length; i++) {
                            int arrIndex = which[i];

                            if (arrIndex == 0) {
                                targets[i] = MbWipeTarget.SYSTEM;
                            } else if (arrIndex == 1) {
                                targets[i] = MbWipeTarget.CACHE;
                            } else if (arrIndex == 2) {
                                targets[i] = MbWipeTarget.DATA;
                            } else if (arrIndex == 3) {
                                targets[i] = MbWipeTarget.DALVIK_CACHE;
                            } else if (arrIndex == 4) {
                                targets[i] = MbWipeTarget.MULTIBOOT;
                            }
                        }

                        WipeTargetsSelectionDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onSelectedWipeTargets(targets);
                        }

                        return true;
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
