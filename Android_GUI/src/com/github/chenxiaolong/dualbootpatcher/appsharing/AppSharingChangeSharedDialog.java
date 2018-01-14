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

package com.github.chenxiaolong.dualbootpatcher.appsharing;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackMultiChoice;
import com.github.chenxiaolong.dualbootpatcher.R;

import java.util.ArrayList;

public class AppSharingChangeSharedDialog extends DialogFragment {
    private static final String ARG_PKG = "pkg";
    private static final String ARG_NAME = "name";
    private static final String ARG_SHARE_DATA = "share_data";
    private static final String ARG_IS_SYSTEM_APP = "is_system_app";
    private static final String ARG_ROMS_USING_SHARED_DATA = "roms_using_shared_data";

    public interface AppSharingChangeSharedDialogListener {
        void onChangedShared(String pkg, boolean shareData);
    }

    public static AppSharingChangeSharedDialog newInstance(Fragment parent, String pkg, String name,
                                                           boolean shareData,
                                                           boolean isSystemApp,
                                                           ArrayList<String> romsUsingSharedData) {
        if (parent != null) {
            if (!(parent instanceof AppSharingChangeSharedDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement AppSharingChangeSharedDialogListener");
            }
        }

        AppSharingChangeSharedDialog frag = new AppSharingChangeSharedDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putString(ARG_PKG, pkg);
        args.putString(ARG_NAME, name);
        args.putBoolean(ARG_SHARE_DATA, shareData);
        args.putBoolean(ARG_IS_SYSTEM_APP, isSystemApp);
        args.putStringArrayList(ARG_ROMS_USING_SHARED_DATA, romsUsingSharedData);
        frag.setArguments(args);
        return frag;
    }

    AppSharingChangeSharedDialogListener getOwner() {
        return (AppSharingChangeSharedDialogListener) getTargetFragment();
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Bundle args = getArguments();
        final String pkg = args.getString(ARG_PKG);
        final String name = args.getString(ARG_NAME);
        final boolean shareData = args.getBoolean(ARG_SHARE_DATA);
        final ArrayList<String> romsUsingSharedData =
                args.getStringArrayList(ARG_ROMS_USING_SHARED_DATA);

        final String shareDataItem = getString(R.string.indiv_app_sharing_dialog_share_data);

        final String[] items = new String[] { shareDataItem };
        final StringBuilder message = new StringBuilder();

        if (!romsUsingSharedData.isEmpty()) {
            String fmt = getString(R.string.indiv_app_sharing_other_roms_using_shared_data);
            String roms = arrayListToString(romsUsingSharedData);
            message.append(String.format(fmt, roms));
            message.append("\n\n");
        }

        ArrayList<Integer> selected = new ArrayList<>();
        if (shareData) {
            selected.add(0);
        }
        message.append(getString(R.string.indiv_app_sharing_dialog_default_message));

        Integer[] selectedIndices = selected.toArray(new Integer[selected.size()]);

        MaterialDialog dialog = new MaterialDialog.Builder(getActivity())
                .title(name)
                .content(message)
                .items(items)
                .negativeText(R.string.cancel)
                .positiveText(R.string.ok)
                .itemsCallbackMultiChoice(selectedIndices, new ListCallbackMultiChoice() {
                    @Override
                    public boolean onSelection(MaterialDialog dialog, Integer[] which,
                                               CharSequence[] text) {
                        boolean shouldShareData = false;

                        for (int index : which) {
                            if (index == 0) {
                                shouldShareData = true;
                            }
                        }

                        AppSharingChangeSharedDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onChangedShared(pkg, shouldShareData);
                        }

                        return true;
                    }
                })
                .build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }

    private static String arrayListToString(ArrayList<String> array) {
        StringBuilder sb = new StringBuilder();
        boolean first = true;
        for (String s : array) {
            if (first) {
                first = false;
            } else {
                sb.append(", ");
            }
            sb.append(s);
        }
        return sb.toString();
    }
}
