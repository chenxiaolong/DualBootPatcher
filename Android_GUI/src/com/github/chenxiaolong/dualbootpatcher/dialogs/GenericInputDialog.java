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

package com.github.chenxiaolong.dualbootpatcher.dialogs;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;

import com.afollestad.materialdialogs.MaterialDialog;

import java.io.Serializable;

public class GenericInputDialog extends DialogFragment implements MaterialDialog.InputCallback {
    private static final String ARG_BUILDER = "builder";
    private static final String ARG_TARGET = "target";
    private static final String ARG_TAG = "tag";

    private Builder mBuilder;
    private DialogListenerTarget mTarget;
    private String mTag;

    public interface GenericInputDialogListener {
        void onConfirmInput(@Nullable String tag, String text);
    }

    @Nullable
    private GenericInputDialogListener getOwner() {
        switch (mTarget) {
        case ACTIVITY:
            return (GenericInputDialogListener) getActivity();
        case FRAGMENT:
            return (GenericInputDialogListener) getTargetFragment();
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

        CharSequence hint = null;
        CharSequence prefill = null;

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

        if (mBuilder.mHint != null) {
            hint = mBuilder.mHint;
        } else if (mBuilder.mHintResId != 0) {
            hint = getText(mBuilder.mHintResId);
        }

        if (mBuilder.mPrefill != null) {
            prefill = mBuilder.mPrefill;
        } else if (mBuilder.mPrefillResId != 0) {
            prefill = getText(mBuilder.mPrefillResId);
        }

        if (mBuilder.mInputType != 0) {
            builder.inputType(mBuilder.mInputType);
        }

        builder.input(hint, prefill, mBuilder.mAllowEmpty, this);

        Dialog dialog = builder.build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }

    @Override
    public void onInput(@NonNull MaterialDialog dialog, CharSequence input) {
        GenericInputDialogListener owner = getOwner();
        if (owner != null) {
            owner.onConfirmInput(mTag, input.toString());
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
        String mHint;
        @StringRes
        int mHintResId;
        @Nullable
        String mPrefill;
        @StringRes
        int mPrefillResId;
        boolean mAllowEmpty = true;
        int mInputType;

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
        public Builder hint(@Nullable String text) {
            mHint = text;
            mHintResId = 0;
            return this;
        }

        @NonNull
        public Builder hint(@StringRes int textResId) {
            mHint = null;
            mHintResId = textResId;
            return this;
        }

        @NonNull
        public Builder prefill(@Nullable String text) {
            mPrefill = text;
            mPrefillResId = 0;
            return this;
        }

        @NonNull
        public Builder prefill(@StringRes int textResId) {
            mPrefill = null;
            mPrefillResId = textResId;
            return this;
        }

        @NonNull
        public Builder allowEmpty(boolean allowEmpty) {
            mAllowEmpty = allowEmpty;
            return this;
        }

        @NonNull
        public Builder inputType(int type) {
            mInputType = type;
            return this;
        }

        @NonNull
        public GenericInputDialog buildFromFragment(@Nullable String tag,
                                                    @NonNull Fragment parent) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT);
            args.putString(ARG_TAG, tag);

            GenericInputDialog dialog = new GenericInputDialog();
            dialog.setTargetFragment(parent, 0);
            dialog.setArguments(args);
            return dialog;
        }

        @NonNull
        public GenericInputDialog buildFromActivity(@Nullable String tag) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY);
            args.putString(ARG_TAG, tag);

            GenericInputDialog dialog = new GenericInputDialog();
            dialog.setArguments(args);
            return dialog;
        }
    }
}