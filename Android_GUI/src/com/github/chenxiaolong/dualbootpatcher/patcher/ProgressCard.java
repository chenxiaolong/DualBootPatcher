package com.github.chenxiaolong.dualbootpatcher.patcher;

import it.gmariotti.cardslib.library.internal.Card;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class ProgressCard extends Card {
    private TextView mPercentage;
    private TextView mFiles;
    private ProgressBar mProgress;

    private int mCurProgress = 0;
    private int mMaxProgress = 0;

    public ProgressCard(Context context) {
        this(context, R.layout.cardcontent_progress);
    }

    public ProgressCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mPercentage = (TextView) view.findViewById(R.id.progress_percentage);
            mFiles = (TextView) view.findViewById(R.id.progress_files);
            mProgress = (ProgressBar) view.findViewById(R.id.progress_bar);

            updateProgress();
        }
    }

    private void updateProgress() {
        if (mProgress != null) {
            mProgress.setMax(mMaxProgress);
            mProgress.setProgress(mCurProgress);
        }
        if (mPercentage != null) {
            if (mMaxProgress == 0) {
                mPercentage.setText("0%");
            } else {
                mPercentage.setText(String.format("%.1f%%",
                        (double) mCurProgress / mMaxProgress * 100));
            }
        }
        if (mFiles != null) {
            mFiles.setText(String.format(
                    getContext().getString(R.string.overall_progress_files),
                    Integer.toString(mCurProgress),
                    Integer.toString(mMaxProgress)));
        }
    }

    public void setProgress(int i) {
        mCurProgress = i;
        updateProgress();
    }

    public void setMaxProgress(int i) {
        mMaxProgress = i;
        updateProgress();
    }

    public void onDetatch() {
        mPercentage = null;
        mFiles = null;
        mProgress = null;
    }

    public void reset() {
        mCurProgress = 0;
        mMaxProgress = 0;
        updateProgress();
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putInt("progress_max", mMaxProgress);
        outState.putInt("progress_current", mCurProgress);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        mMaxProgress = savedInstanceState.getInt("progress_max");
        mCurProgress = savedInstanceState.getInt("progress_current");
    }
}