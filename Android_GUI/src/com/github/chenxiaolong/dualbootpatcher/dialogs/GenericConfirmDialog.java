/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

public class GenericConfirmDialog extends DialogFragment implements SingleButtonCallback {
    private static final String ARG_BUILDER = "builder";
    private static final String ARG_TARGET = "target";
    private static final String ARG_TAG = "tag";

    private Builder mBuilder;
    private DialogListenerTarget mTarget;
    private String mTag;

    public interface GenericConfirmDialogListener {
        void onConfirmOk(@Nullable String tag);
    }

    @Nullable
    private GenericConfirmDialogListener getOwner() {
        switch (mTarget) {
        case ACTIVITY:
            return (GenericConfirmDialogListener) getActivity();
        case FRAGMENT:
            return (GenericConfirmDialogListener) getTargetFragment();
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

        if (mBuilder.mButtonText != null) {
            builder.positiveText(mBuilder.mButtonText);
        } else if (mBuilder.mButtonTextResId != 0) {
            builder.positiveText(mBuilder.mButtonTextResId);
        }

        builder.onPositive(this);

        Dialog dialog = builder.build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
    }

    @Override
    public void onClick(@NonNull MaterialDialog dialog, @NonNull DialogAction which) {
        GenericConfirmDialogListener owner = getOwner();
        if (owner != null) {
            owner.onConfirmOk(mTag);
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
        String mButtonText;
        @StringRes
        int mButtonTextResId;

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
        public Builder buttonText(@Nullable String text) {
            mButtonText = text;
            mButtonTextResId = 0;
            return this;
        }

        @NonNull
        public Builder buttonText(@StringRes int textResId) {
            mButtonText = null;
            mButtonTextResId = textResId;
            return this;
        }

        @NonNull
        public GenericConfirmDialog build() {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.NONE);
            args.putString(ARG_TAG, null);

            GenericConfirmDialog dialog = new GenericConfirmDialog();
            dialog.setArguments(args);
            return dialog;
        }

        @NonNull
        public GenericConfirmDialog buildFromFragment(@Nullable String tag,
                                                    @NonNull Fragment parent) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.FRAGMENT);
            args.putString(ARG_TAG, tag);

            GenericConfirmDialog dialog = new GenericConfirmDialog();
            dialog.setTargetFragment(parent, 0);
            dialog.setArguments(args);
            return dialog;
        }

        @NonNull
        public GenericConfirmDialog buildFromActivity(@Nullable String tag) {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);
            args.putSerializable(ARG_TARGET, DialogListenerTarget.ACTIVITY);
            args.putString(ARG_TAG, tag);

            GenericConfirmDialog dialog = new GenericConfirmDialog();
            dialog.setArguments(args);
            return dialog;
        }
    }
}
