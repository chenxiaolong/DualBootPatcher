/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

import it.gmariotti.cardslib.library.internal.Card;

public class FileChooserCard extends Card {
    private static final String EXTRA_PROGRESS = "filechooser_progress";
    private static final String EXTRA_FAILED = "filechooser_failed";
    private static final String EXTRA_MESSAGE = "filechooser_message";
    private static final String EXTRA_NEWFILE = "filechooser_newfile";

    private PatcherConfigState mPCS;

    private boolean mShowProgress;

    private boolean mFailed;
    private String mMessage;
    private String mNewFile;

    private TextView mTitleView;
    private TextView mDescView;
    private ProgressBar mProgressBar;

    public FileChooserCard(Context context, PatcherConfigState pcs) {
        this(context, R.layout.cardcontent_file_chooser);
        mPCS = pcs;
    }

    public FileChooserCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mTitleView = (TextView) view.findViewById(R.id.file_chooser_title);
            mDescView = (TextView) view.findViewById(R.id.file_chooser_message);
            mProgressBar = (ProgressBar) view
                    .findViewById(R.id.file_chooser_progress);

            displayProgress();
            displayMessage();
        }
    }

    private void displayProgress() {
        if (mShowProgress) {
            mTitleView.setVisibility(View.GONE);
            mDescView.setVisibility(View.GONE);
            mProgressBar.setVisibility(View.VISIBLE);
        } else {
            mTitleView.setVisibility(View.VISIBLE);
            mDescView.setVisibility(View.VISIBLE);
            mProgressBar.setVisibility(View.GONE);
        }
    }

    private void displayMessage() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_INITIAL:
            mTitleView.setText(R.string.filechooser_initial_title);
            mDescView.setText(R.string.filechooser_initial_desc);
            break;

        case PatcherConfigState.STATE_PATCHING:
        case PatcherConfigState.STATE_CHOSE_FILE:
            int titleId;
            int descId;

            // TODO: PARTCONFIG
            if (mPCS.mSupported != PatcherConfigState.NOT_SUPPORTED) {
                titleId = R.string.filechooser_ready_title;
                descId = R.string.filechooser_ready_desc;
            } else {
                titleId = R.string.filechooser_unsupported_title;
                descId = R.string.filechooser_unsupported_desc;
            }

            mTitleView.setText(titleId);
            mDescView.setText(String.format(getContext().getString(descId), mPCS.mFilename));
            break;

        case PatcherConfigState.STATE_FINISHED:
            if (mFailed) {
                mTitleView.setText(R.string.filechooser_failure_title);
                mDescView.setText(String.format(
                        getContext().getString(
                                R.string.filechooser_failure_desc), mMessage));
            } else {
                mTitleView.setText(R.string.filechooser_success_title);
                mDescView.setText(String.format(
                        getContext().getString(
                                R.string.filechooser_success_desc), mNewFile));
            }
            break;
        }
    }

    public void onFileChosen() {
        displayMessage();

        if (getCardView() != null) {
            getCardView().refreshCard(this);
        }
    }

    public void onFinishedPatching(boolean failed, String message,
            String newFile) {
        mFailed = failed;
        mMessage = message;
        mNewFile = newFile;
        displayMessage();
    }

    public void setEnabled(boolean enabled) {
        mTitleView.setEnabled(enabled);
        mDescView.setEnabled(enabled);
        setClickable(enabled);
        setLongClickable(enabled);

        if (getCardView() != null) {
            getCardView().refreshCard(this);
        }
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
