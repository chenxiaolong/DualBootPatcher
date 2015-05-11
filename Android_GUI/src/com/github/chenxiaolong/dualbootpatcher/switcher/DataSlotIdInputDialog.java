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
import android.app.DialogFragment;
import android.app.Fragment;
import android.os.Bundle;
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.widget.EditText;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.InputCallback;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.multibootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.multibootpatcher.patcher.PatcherUtils.InstallLocation;

public class DataSlotIdInputDialog extends DialogFragment {
    public static final String TAG = DataSlotIdInputDialog.class.getSimpleName();

    public interface DataSlotIdInputDialogListener {
        void onSelectedDataSlotRomId(String romId);
    }

    public static DataSlotIdInputDialog newInstance(Fragment parent) {
        if (parent != null) {
            if (!(parent instanceof DataSlotIdInputDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement DataSlotIdInputDialogListener");
            }
        }

        DataSlotIdInputDialog frag = new DataSlotIdInputDialog();
        frag.setTargetFragment(parent, 0);
        return frag;
    }

    DataSlotIdInputDialogListener getOwner() {
        return (DataSlotIdInputDialogListener) getTargetFragment();
    }

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final MaterialDialog dialog = new MaterialDialog.Builder(getActivity())
                .title(R.string.install_location_data_slot)
                .content(R.string.install_location_data_slot_id_hint)
                .inputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
                .input(R.string.install_location_data_slot_id_hint, 0, false,
                        new InputCallback() {
                            @Override
                            public void onInput(MaterialDialog dialog, CharSequence input) {
                                InstallLocation location = PatcherUtils.getDataSlotInstallLocation(
                                        getActivity(), input.toString());

                                DataSlotIdInputDialogListener owner = getOwner();
                                if (owner != null) {
                                    owner.onSelectedDataSlotRomId(location.id);
                                }
                            }
                        })
                .build();

        final EditText et = dialog.getInputEditText();
        if (et != null) {
            et.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                }

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                }

                @Override
                public void afterTextChanged(Editable s) {
                    String text = s.toString();

                    if (text.isEmpty()) {
                        et.setError(getString(
                                R.string.install_location_data_slot_id_error_is_empty));
                        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);
                    } else if (!text.matches("[a-z0-9]+")) {
                        et.setError(getString(
                                R.string.install_location_data_slot_id_error_invalid));
                        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);
                    } else {
                        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(true);
                    }

                    InstallLocation location = PatcherUtils.getDataSlotInstallLocation(
                            getActivity(), text);
                    dialog.setContent(location.description);
                }
            });
        }

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
