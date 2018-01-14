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

import com.afollestad.materialdialogs.MaterialDialog;

import java.io.Serializable;

public class GenericProgressDialog extends DialogFragment {
    private static final String ARG_BUILDER = "builder";

    private Builder mBuilder;

    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        Bundle args = getArguments();
        mBuilder = (Builder) args.getSerializable(ARG_BUILDER);

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

        builder.progress(true, 0);

        Dialog dialog = builder.build();

        setCancelable(false);
        dialog.setCanceledOnTouchOutside(false);

        return dialog;
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
        public GenericProgressDialog build() {
            Bundle args = new Bundle();
            args.putSerializable(ARG_BUILDER, this);

            GenericProgressDialog dialog = new GenericProgressDialog();
            dialog.setArguments(args);
            return dialog;
        }
    }
}
