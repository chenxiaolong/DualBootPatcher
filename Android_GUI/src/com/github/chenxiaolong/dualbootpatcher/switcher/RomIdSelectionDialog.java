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
import android.app.DialogFragment;
import android.app.Fragment;
import android.os.Bundle;
import android.view.View;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackSingleChoice;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

import java.util.ArrayList;

public class RomIdSelectionDialog extends DialogFragment {
    enum RomIdType {
        BUILT_IN_ROM_ID,
        NAMED_DATA_SLOT,
        NAMED_EXTSD_SLOT
    }

    private static final String ARG_INFOS = "infos";

    public interface RomIdSelectionDialogListener {
        void onSelectedRomId(RomIdType type, String romId);
    }

    public static RomIdSelectionDialog newInstance(Fragment parent,
                                                   ArrayList<RomInformation> infos) {
        if (parent != null) {
            if (!(parent instanceof RomIdSelectionDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement RomIdSelectionDialogListener");
            }
        }

        RomIdSelectionDialog frag = new RomIdSelectionDialog();
        frag.setTargetFragment(parent, 0);
        Bundle args = new Bundle();
        args.putParcelableArrayList(ARG_INFOS, infos);
        frag.setArguments(args);
        return frag;
    }

    RomIdSelectionDialogListener getOwner() {
        return (RomIdSelectionDialogListener) getTargetFragment();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final ArrayList<RomInformation> infos = getArguments().getParcelableArrayList(ARG_INFOS);
        final String[] names = new String[infos.size() + 2];

        for (int i = 0; i < infos.size(); i++) {
            names[i] = infos.get(i).getDefaultName();
        }

        names[infos.size()] = getString(R.string.install_location_data_slot);
        names[infos.size() + 1] = getString(R.string.install_location_extsd_slot);

        Dialog dialog = new MaterialDialog.Builder(getActivity())
                .title(R.string.in_app_flashing_dialog_installation_location)
                .items(names)
                .negativeText(R.string.cancel)
                .itemsCallbackSingleChoice(-1, new ListCallbackSingleChoice() {
                    @Override
                    public boolean onSelection(MaterialDialog dialog, View view, int which,
                                               CharSequence text) {
                        RomIdSelectionDialogListener owner = getOwner();
                        if (owner != null) {
                            if (which == infos.size()) {
                                owner.onSelectedRomId(RomIdType.NAMED_DATA_SLOT, null);
                            } else if (which == infos.size() + 1) {
                                owner.onSelectedRomId(RomIdType.NAMED_EXTSD_SLOT, null);
                            } else {
                                RomInformation info = infos.get(which);
                                owner.onSelectedRomId(RomIdType.BUILT_IN_ROM_ID, info.getId());
                            }
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
