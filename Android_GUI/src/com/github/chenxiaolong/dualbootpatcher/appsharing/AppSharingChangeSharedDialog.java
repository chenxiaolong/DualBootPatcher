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
import android.app.DialogFragment;
import android.app.Fragment;
import android.os.Bundle;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackMultiChoice;
import com.github.chenxiaolong.dualbootpatcher.R;

import java.util.ArrayList;

public class AppSharingChangeSharedDialog extends DialogFragment {
    public static final String TAG = AppSharingChangeSharedDialog.class.getSimpleName();

    private static final String ARG_PKG = "pkg";
    private static final String ARG_NAME = "name";
    private static final String ARG_SHARE_APK = "share_apk";
    private static final String ARG_SHARE_DATA = "share_data";
    private static final String ARG_IS_SYSTEM_APP = "is_system_app";

    public interface AppSharingChangeSharedDialogListener {
        void onChangedShared(String pkg, boolean shareApk, boolean shareData);
    }

    public static AppSharingChangeSharedDialog newInstance(Fragment parent, String pkg, String name,
                                                           boolean shareApk, boolean shareData,
                                                           boolean isSystemApp) {
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
        args.putBoolean(ARG_SHARE_APK, shareApk);
        args.putBoolean(ARG_SHARE_DATA, shareData);
        args.putBoolean(ARG_IS_SYSTEM_APP, isSystemApp);
        frag.setArguments(args);
        return frag;
    }

    AppSharingChangeSharedDialogListener getOwner() {
        return (AppSharingChangeSharedDialogListener) getTargetFragment();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Bundle args = getArguments();
        final String pkg = args.getString(ARG_PKG);
        final String name = args.getString(ARG_NAME);
        final boolean shareApk = args.getBoolean(ARG_SHARE_APK);
        final boolean shareData = args.getBoolean(ARG_SHARE_DATA);
        final boolean isSystemApp = args.getBoolean(ARG_IS_SYSTEM_APP);

        final String shareApkItem = getString(R.string.indiv_app_sharing_dialog_share_apk);
        final String shareDataItem = getString(R.string.indiv_app_sharing_dialog_share_data);

        final int shareApkIndex;
        final int shareDataIndex;
        final String[] items;
        final String message;

        ArrayList<Integer> selected = new ArrayList<>();

        if (isSystemApp) {
            // Cannot share apk
            shareApkIndex = -1;
            shareDataIndex = 0;
            items = new String[] { shareDataItem };
            if (shareData) {
                selected.add(shareDataIndex);
            }
            message = getString(R.string.indiv_app_sharing_dialog_system_app_message);
        } else {
            shareApkIndex = 0;
            shareDataIndex = 1;
            items = new String[] { shareApkItem, shareDataItem };
            if (shareApk) {
                selected.add(shareApkIndex);
            }
            if (shareData) {
                selected.add(shareDataIndex);
            }
            message = getString(R.string.indiv_app_sharing_dialog_default_message);
        }

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
                        boolean shouldShareApk = false;
                        boolean shouldShareData = false;

                        for (int index : which) {
                            if (index == shareApkIndex) {
                                shouldShareApk = true;
                            } else if (index == shareDataIndex) {
                                shouldShareData = true;
                            }
                        }

                        AppSharingChangeSharedDialogListener owner = getOwner();
                        if (owner != null) {
                            owner.onChangedShared(pkg, shouldShareApk, shouldShareData);
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
