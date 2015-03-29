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

public class ProgressCW implements PatcherUIListener {
    private static final String EXTRA_PROGRESS_BYTES = "bytes";
    private static final String EXTRA_PROGRESS_MAX_BYTES = "max_bytes";
    private static final String EXTRA_PROGRESS_FILES = "files";
    private static final String EXTRA_PROGRESS_MAX_FILES = "max_files";

    private CardView vCard;
    private TextView vPercentage;
    private TextView vFiles;
    private ProgressBar vProgress;

    private Context mContext;
    private PatcherConfigState mPCS;

    private long mBytes = 0;
    private long mMaxBytes = 0;
    private long mFiles = 0;
    private long mMaxFiles = 0;

    public ProgressCW(Context context, PatcherConfigState pcs, CardView card) {
        mContext = context;
        mPCS = pcs;

        vCard = card;
        vPercentage = (TextView) card.findViewById(R.id.progress_percentage);
        vFiles = (TextView) card.findViewById(R.id.progress_files);
        vProgress = (ProgressBar) card.findViewById(R.id.progress_bar);
    }

    private void updateProgress() {
        if (vProgress != null) {
            vProgress.setMax((int) mMaxBytes);
            vProgress.setProgress((int) mBytes);
        }
        if (vPercentage != null) {
            if (mMaxBytes == 0) {
                vPercentage.setText("0%");
            } else {
                vPercentage.setText(String.format("%.1f%%", 100.0 * mBytes / mMaxBytes));
            }
        }
        if (vFiles != null) {
            vFiles.setText(String.format(mContext.getString(R.string.overall_progress_files),
                    Long.toString(mFiles), Long.toString(mMaxFiles)));
        }
    }

    public void setProgress(long bytes, long maxBytes) {
        mBytes = bytes;
        mMaxBytes = maxBytes;
        updateProgress();
    }

    public void setFiles(long files, long maxFiles) {
        mFiles = files;
        mMaxFiles = maxFiles;
        updateProgress();
    }

    @Override
    public void onCardCreate() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_PATCHING:
            vCard.setVisibility(View.VISIBLE);
            break;

        case PatcherConfigState.STATE_CHOSE_FILE:
        case PatcherConfigState.STATE_INITIAL:
        case PatcherConfigState.STATE_FINISHED:
            vCard.setVisibility(View.GONE);
            break;
        }

        updateProgress();
    }

    @Override
    public void onRestoreCardState(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            mBytes = savedInstanceState.getLong(EXTRA_PROGRESS_BYTES);
            mMaxBytes = savedInstanceState.getLong(EXTRA_PROGRESS_MAX_BYTES);
            mFiles = savedInstanceState.getLong(EXTRA_PROGRESS_FILES);
            mMaxFiles = savedInstanceState.getLong(EXTRA_PROGRESS_MAX_FILES);
            updateProgress();
        }
    }

    @Override
    public void onSaveCardState(Bundle outState) {
        outState.putLong(EXTRA_PROGRESS_BYTES, mBytes);
        outState.putLong(EXTRA_PROGRESS_MAX_BYTES, mMaxBytes);
        outState.putLong(EXTRA_PROGRESS_FILES, mFiles);
        outState.putLong(EXTRA_PROGRESS_MAX_FILES, mMaxFiles);
    }

    @Override
    public void onChoseFile() {
    }

    @Override
    public void onStartedPatching() {
        vCard.setVisibility(View.VISIBLE);
    }

    @Override
    public void onFinishedPatching() {
        vCard.setVisibility(View.GONE);

        mBytes = 0;
        mMaxBytes = 0;
        mFiles = 0;
        mMaxFiles = 0;
        updateProgress();
    }
}
