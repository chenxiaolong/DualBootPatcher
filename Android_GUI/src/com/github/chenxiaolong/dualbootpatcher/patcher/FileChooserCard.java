package com.github.chenxiaolong.dualbootpatcher.patcher;

import it.gmariotti.cardslib.library.internal.Card;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class FileChooserCard extends Card {
    public static final int STATE_INITIAL = 0;
    public static final int STATE_CHOSE_FILE = 1;
    public static final int STATE_FINISHED_PATCHING = 2;

    private int mState;
    private boolean mShowProgress;

    private String mFilename;
    private boolean mSupported;
    private boolean mFailed;
    private String mMessage;
    private String mNewFile;

    private TextView mTitleView;
    private TextView mDescView;
    private ProgressBar mProgressBar;

    public FileChooserCard(Context context) {
        this(context, R.layout.cardcontent_file_chooser);
    }

    public FileChooserCard(Context context, int innerLayout) {
        super(context, innerLayout);
        mState = STATE_INITIAL;
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
        switch (mState) {
        case STATE_INITIAL:
            mTitleView.setText(R.string.filechooser_initial_title);
            mDescView.setText(R.string.filechooser_initial_desc);
            break;

        case STATE_CHOSE_FILE:
            int titleId;
            int descId;

            if (mSupported) {
                titleId = R.string.filechooser_ready_title;
                descId = R.string.filechooser_ready_desc;
            } else {
                titleId = R.string.filechooser_unsupported_title;
                descId = R.string.filechooser_unsupported_desc;
            }

            mTitleView.setText(titleId);
            mDescView.setText(String.format(getContext().getString(descId),
                    mFilename));
            break;

        case STATE_FINISHED_PATCHING:
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

    public void onFileChosen(String filename, boolean supported) {
        mState = STATE_CHOSE_FILE;
        mFilename = filename;
        mSupported = supported;
        displayMessage();
    }

    public void onFinishedPatching(boolean failed, String message,
            String newFile) {
        mState = STATE_FINISHED_PATCHING;
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
    }

    public void setProgressShowing(boolean show) {
        mShowProgress = show;
        displayProgress();
    }

    public boolean isFileSelected() {
        return mState == STATE_CHOSE_FILE;
    }

    public String getFilename() {
        return mFilename;
    }

    public boolean isSupported() {
        return mSupported;
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putInt("filechooser_state", mState);
        outState.putBoolean("filechooser_progress", mShowProgress);
        outState.putString("filechooser_filename", mFilename);
        outState.putBoolean("filechooser_supported", mSupported);
        outState.putBoolean("filechooser_failed", mFailed);
        outState.putString("filechooser_message", mMessage);
        outState.putString("filechooser_newfile", mNewFile);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        mState = savedInstanceState.getInt("filechooser_state");
        mShowProgress = savedInstanceState.getBoolean("filechooser_progress");
        mFilename = savedInstanceState.getString("filechooser_filename");
        mSupported = savedInstanceState.getBoolean("filechooser_supported");
        mFailed = savedInstanceState.getBoolean("filechooser_failed");
        mMessage = savedInstanceState.getString("filechooser_message");
        mNewFile = savedInstanceState.getString("filechooser_newfile");
    }
}