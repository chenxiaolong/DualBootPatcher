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
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;

import com.afollestad.materialdialogs.DialogAction;
import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.SingleButtonCallback;

import java.io.Serializable;

public class GenericYesNoDialog extends DialogFragment implements SingleButtonCallback {
    private static final String ARG_BUILDER = "builder";
    private static final String ARG_TARGET = "target";
    private static final String ARG_TAG = "tag";

    private Builder mBuilder;
    private DialogListenerTarget mTarget;
    private String mTag;

    public interface GenericYesNoDialogListener {
        void onConfirmYesNo(@Nullable String tag, boolean choice);
    }

    @Nullable
    private GenericYesNoDialogListener getOwner() {
        switch (mTarget) {
        case ACTIVITY:
            return (GenericYesNoDialogListener) getActivity();
        case FRAGMENT:
            return (GenericYesNoDialogListener) getTargetFragment();
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

        builder.onPositive(this);
        builder.onNegative(this);

        Dialog dialog = builder.build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }

    @Override
    public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        GenericYesNoDialogListener owner = getOwner();
        if (owner == null) {
            return;
        }

        switch (which) {
        case POSITIVE:
            owner.onConfirmYesNo(mTag, true);
            break;
        case NEGATIVE:
            owner.onConfirmYesNo(mTag, false);
            break;
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
        public GenericYesNoDialog buildFromFragment(@Nullable String tag,
                                                    @NonNull Fragment parent) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT);
            args.putString(ARG_TAG, tag);

            GenericYesNoDialog dialog = new GenericYesNoDialog();
            dialog.setTargetFragment(parent, 0);
            dialog.setArguments(args);
            return dialog;
        }

        @NonNull
        public GenericYesNoDialog buildFromActivity(@Nullable String tag) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY);
            args.putString(ARG_TAG, tag);

            GenericYesNoDialog dialog = new GenericYesNoDialog();
            dialog.setArguments(args);
            return dialog;
        }
    }
}
