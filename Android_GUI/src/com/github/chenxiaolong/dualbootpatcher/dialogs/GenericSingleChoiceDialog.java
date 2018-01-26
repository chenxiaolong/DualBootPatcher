/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.dialogs;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.ArrayRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.view.View;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackSingleChoice;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;

import java.io.Serializable;

public class GenericSingleChoiceDialog extends DialogFragment implements ListCallbackSingleChoice, SingleButtonCallback {
    private static final String ARG_BUILDER = "builder";
    private static final String ARG_TARGET = "target";
    private static final String ARG_TAG = "tag";

    private static final String EXTRA_SELECTED_INDEX = "selected_index";
    private static final String EXTRA_SELECTED_TEXT = "selected_text";

    private Builder mBuilder;
    private DialogListenerTarget mTarget;
    private String mTag;

    private String mSelectedText;

    public interface GenericSingleChoiceDialogListener {
        void onConfirmSingleChoice(@Nullable String tag, int index, String text);
    }

    @Nullable
    private GenericSingleChoiceDialogListener getOwner() {
        switch (mTarget) {
        case ACTIVITY:
            return (GenericSingleChoiceDialogListener) getActivity();
        case FRAGMENT:
            return (GenericSingleChoiceDialogListener) getTargetFragment();
        case NONE:
        default:
            return null;
        }
    }

    @Override
    @NonNull
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Bundle args = getArguments();
        mBuilder = (Builder) args.getSerializable(ARG_BUILDER);
        mTarget = (DialogListenerTarget) args.getSerializable(ARG_TARGET);
        mTag = args.getString(ARG_TAG);

        int selectedIndex = -1;

        if (savedInstanceState != null) {
            selectedIndex = savedInstanceState.getInt(EXTRA_SELECTED_INDEX, -1);
            mSelectedText = savedInstanceState.getString(EXTRA_SELECTED_TEXT);
        }

        MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity());

        if (mBuilder.mTitle != null) {
            builder.title(mBuilder.mTitle);
        } else if (mBuilder.mTitleResId != 0) {
            builder.title(mBuilder.mTitleResId);
        }

        if (mBuilder.mMessage != null) {
            builder.content(mBuilder.mMessage);
        } else if (mBuilder.mMessageResId != 0) {
            builder.content(mBuilder.mMessageResId);
        }

        if (mBuilder.mPositive != null) {
            builder.positiveText(mBuilder.mPositive);
        } else if (mBuilder.mPositiveResId != 0) {
            builder.positiveText(mBuilder.mPositiveResId);
        }

        if (mBuilder.mNegative != null) {
            builder.negativeText(mBuilder.mNegative);
        } else if (mBuilder.mNegativeResId != 0) {
            builder.negativeText(mBuilder.mNegativeResId);
        }

        if (mBuilder.mChoices != null) {
            builder.items((CharSequence[]) mBuilder.mChoices);
        } else if (mBuilder.mChoicesResId != 0) {
            builder.items(mBuilder.mChoicesResId);
        }

        builder.itemsCallbackSingleChoice(selectedIndex, this);
        builder.alwaysCallSingleChoiceCallback();

        builder.onPositive(this);

        MaterialDialog dialog = builder.build();

        // Nothing selected by default, so don't enable OK button
        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(selectedIndex >= 0);

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        MaterialDialog dialog = (MaterialDialog) getDialog();
        if (dialog != null) {
            outState.putInt(EXTRA_SELECTED_INDEX, dialog.getSelectedIndex());
        }
        outState.putString(EXTRA_SELECTED_TEXT, mSelectedText);
    }

    @Override
    public boolean onSelection(MaterialDialog dialog, View itemView, int which, CharSequence text) {
        dialog.getActionButton(DialogAction.POSITIVE).setEnabled(true);
        mSelectedText = text.toString();
        return true;
    }

    @Override
    public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        GenericSingleChoiceDialogListener owner = getOwner();
        if (owner != null) {
            int index = dialog.getSelectedIndex();
            owner.onConfirmSingleChoice(mTag, index, mSelectedText);
        }
    }

    public static class Builder implements Serializable {
        @Nullable
        String mTitle;
        @StringRes
        int mTitleResId;
        @Nullable
        String mMessage;
        @StringRes
        int mMessageResId;
        @Nullable
        String mPositive;
        @StringRes
        int mPositiveResId;
        @Nullable
        String mNegative;
        @StringRes
        int mNegativeResId;
        @Nullable
        String[] mChoices;
        @ArrayRes
        int mChoicesResId;

        @NonNull
        public Builder title(@Nullable String title) {
            mTitle = title;
            mTitleResId = 0;
            return this;
        }

        @NonNull
        public Builder title(@StringRes int titleResId) {
            mTitle = null;
            mTitleResId = titleResId;
            return this;
        }

        @NonNull
        public Builder message(@Nullable String message) {
            mMessage = message;
            mMessageResId = 0;
            return this;
        }

        @NonNull
        public Builder message(@StringRes int messageResId) {
            mMessage = null;
            mMessageResId = messageResId;
            return this;
        }

        @NonNull
        public Builder positive(@Nullable String text) {
            mPositive = text;
            mPositiveResId = 0;
            return this;
        }

        @NonNull
        public Builder positive(@StringRes int textResId) {
            mPositive = null;
            mPositiveResId = textResId;
            return this;
        }

        @NonNull
        public Builder negative(@Nullable String text) {
            mNegative = text;
            mNegativeResId = 0;
            return this;
        }

        @NonNull
        public Builder negative(@StringRes int textResId) {
            mNegative = null;
            mNegativeResId = textResId;
            return this;
        }

        @NonNull
        public Builder choices(@Nullable String... choices) {
            mChoices = choices;
            mChoicesResId = 0;
            return this;
        }

        @NonNull
        public Builder choices(@ArrayRes int choicesResId) {
            mChoices = null;
            mChoicesResId = choicesResId;
            return this;
        }

        @NonNull
        public GenericSingleChoiceDialog buildFromFragment(@Nullable String tag,
                                                           @NonNull Fragment parent) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT);
            args.putString(ARG_TAG, tag);

            GenericSingleChoiceDialog dialog = new GenericSingleChoiceDialog();
            dialog.setTargetFragment(parent, 0);
            dialog.setArguments(args);
            return dialog;
        }

        @NonNull
        public GenericSingleChoiceDialog buildFromActivity(@Nullable String tag) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY);
            args.putString(ARG_TAG, tag);

            GenericSingleChoiceDialog dialog = new GenericSingleChoiceDialog();
            dialog.setArguments(args);
            return dialog;
        }
    }
}
