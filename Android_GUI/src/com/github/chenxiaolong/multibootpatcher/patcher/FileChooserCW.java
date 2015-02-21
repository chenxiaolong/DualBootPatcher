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

package com.github.chenxiaolong.multibootpatcher.patcher;

import android.content.Context;
import android.os.Bundle;
import android.support.v7.widget.CardView;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class FileChooserCW {
    private static final String EXTRA_PROGRESS = "filechooser_progress";
    private static final String EXTRA_FAILED = "filechooser_failed";
    private static final String EXTRA_MESSAGE = "filechooser_message";
    private static final String EXTRA_NEWFILE = "filechooser_newfile";

    private CardView vCard;
    private TextView vTitle;
    private TextView vMessage;
    private ProgressBar vProgressBar;

    private Context mContext;
    private PatcherConfigState mPCS;

    private boolean mShowProgress;
    private boolean mFailed;
    private String mMessage;
    private String mNewFile;

    public FileChooserCW(Context context, PatcherConfigState pcs, CardView card) {
        mContext = context;
        mPCS = pcs;

        vCard = card;
        vTitle = (TextView) card.findViewById(R.id.file_chooser_title);
        vMessage = (TextView) card.findViewById(R.id.file_chooser_message);
        vProgressBar = (ProgressBar) card.findViewById(R.id.file_chooser_progress);

        displayProgress();
        displayMessage();
    }

    private void displayProgress() {
        if (mShowProgress) {
            vTitle.setVisibility(View.GONE);
            vMessage.setVisibility(View.GONE);
            vProgressBar.setVisibility(View.VISIBLE);
        } else {
            vTitle.setVisibility(View.VISIBLE);
            vMessage.setVisibility(View.VISIBLE);
            vProgressBar.setVisibility(View.GONE);
        }
    }

    private void displayMessage() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_INITIAL:
            vTitle.setText(R.string.filechooser_initial_title);
            vMessage.setText(R.string.filechooser_initial_desc);
            break;

        case PatcherConfigState.STATE_PATCHING:
        case PatcherConfigState.STATE_CHOSE_FILE:
            int titleId;
            int descId;

            if (mPCS.mSupported != PatcherConfigState.NOT_SUPPORTED) {
                titleId = R.string.filechooser_ready_title;
                descId = R.string.filechooser_ready_desc;
            } else {
                titleId = R.string.filechooser_unsupported_title;
                descId = R.string.filechooser_unsupported_desc;
            }

            vTitle.setText(titleId);
            vMessage.setText(String.format(mContext.getString(descId), mPCS.mFilename));
            break;

        case PatcherConfigState.STATE_FINISHED:
            if (mFailed) {
                vTitle.setText(R.string.filechooser_failure_title);
                vMessage.setText(String.format(mContext.getString(R.string
                        .filechooser_failure_desc), mMessage));
            } else {
                vTitle.setText(R.string.filechooser_success_title);
                vMessage.setText(String.format(mContext.getString(R.string
                        .filechooser_success_desc), mNewFile));
            }
            break;
        }
    }

    public void onFileChosen() {
        displayMessage();
    }

    public void onFinishedPatching(boolean failed, String message, String newFile) {
        mFailed = failed;
        mMessage = message;
        mNewFile = newFile;
        displayMessage();
    }

    public void setEnabled(boolean enabled) {
        vTitle.setEnabled(enabled);
        vMessage.setEnabled(enabled);
        vCard.setClickable(enabled);
        vCard.setLongClickable(enabled);
    }

    public void setProgressShowing(boolean show) {
        mShowProgress = show;
        displayProgress();
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putBoolean(EXTRA_PROGRESS, mShowProgress);
        outState.putBoolean(EXTRA_FAILED, mFailed);
        outState.putString(EXTRA_MESSAGE, mMessage);
        outState.putString(EXTRA_NEWFILE, mNewFile);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        mShowProgress = savedInstanceState.getBoolean(EXTRA_PROGRESS);
        mFailed = savedInstanceState.getBoolean(EXTRA_FAILED);
        mMessage = savedInstanceState.getString(EXTRA_MESSAGE);
        mNewFile = savedInstanceState.getString(EXTRA_NEWFILE);

        displayMessage();
    }

    public void refreshState() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_PATCHING:
            setEnabled(false);
            break;

        case PatcherConfigState.STATE_INITIAL:
        case PatcherConfigState.STATE_CHOSE_FILE:
        case PatcherConfigState.STATE_FINISHED:
            setEnabled(true);
            break;
        }
    }
}
