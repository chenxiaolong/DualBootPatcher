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
import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.widget.EditText;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.InputCallback;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils.InstallLocation;

public class NamedSlotIdInputDialog extends DialogFragment {
    private static final String ARG_TYPE = "type";

    public static final int DATA_SLOT = 1;
    public static final int EXTSD_SLOT = 2;

    public interface NamedSlotIdInputDialogListener {
        void onSelectedNamedSlotRomId(String romId);
    }

    public static NamedSlotIdInputDialog newInstance(Fragment parent, int type) {
        if (parent != null) {
            if (!(parent instanceof NamedSlotIdInputDialogListener)) {
                throw new IllegalStateException(
                        "Parent fragment must implement NamedSlotIdInputDialogListener");
            }
        }

        Bundle args = new Bundle();
        args.putInt(ARG_TYPE, type);

        NamedSlotIdInputDialog frag = new NamedSlotIdInputDialog();
        frag.setArguments(args);
        frag.setTargetFragment(parent, 0);
        return frag;
    }

    NamedSlotIdInputDialogListener getOwner() {
        return (NamedSlotIdInputDialogListener) getTargetFragment();
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        final int type = getArguments().getInt(ARG_TYPE);

        final MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity())
                .content(R.string.install_location_named_slot_id_hint)
                .inputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS)
                .input(R.string.install_location_named_slot_id_hint, 0, false,
                        new InputCallback() {
                            @Override
                            public void onInput(@NonNull MaterialDialog dialog,
                                                CharSequence input) {
                                InstallLocation location;

                                if (type == DATA_SLOT) {
                                    location = PatcherUtils.getDataSlotInstallLocation(
                                            getActivity(), input.toString());
                                } else if (type == EXTSD_SLOT) {
                                    location = PatcherUtils.getExtsdSlotInstallLocation(
                                            getActivity(), input.toString());
                                } else {
                                    return;
                                }

                                NamedSlotIdInputDialogListener owner = getOwner();
                                if (owner != null) {
                                    owner.onSelectedNamedSlotRomId(location.id);
                                }
                            }
                        });

        if (type == DATA_SLOT) {
            builder.title(R.string.install_location_data_slot);
        } else if (type == EXTSD_SLOT) {
            builder.title(R.string.install_location_extsd_slot);
        }

        final MaterialDialog dialog = builder.build();

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
                                R.string.install_location_named_slot_id_error_is_empty));
                        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);
                    } else if (!text.matches("[a-z0-9]+")) {
                        et.setError(getString(
                                R.string.install_location_named_slot_id_error_invalid));
                        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(false);
                    } else {
                        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(true);
                    }

                    InstallLocation location;
                    if (type == DATA_SLOT) {
                        location = PatcherUtils.getDataSlotInstallLocation(
                                getActivity(), text);
                    } else if (type == EXTSD_SLOT) {
                        location = PatcherUtils.getExtsdSlotInstallLocation(
                                getActivity(), text);
                    } else {
                        return;
                    }
                    dialog.setContent(location.description);
                }
            });
        }

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }
}
