/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.support.v7.widget.CardView;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class FileChooserCW implements PatcherUIListener {
    private CardView vCard;
    private TextView vTitle;
    private TextView vMessage;

    private Context mContext;
    private PatcherConfigState mPCS;

    public FileChooserCW(Context context, PatcherConfigState pcs, CardView card) {
        mContext = context;
        mPCS = pcs;

        vCard = card;
        vTitle = (TextView) card.findViewById(R.id.file_chooser_title);
        vMessage = (TextView) card.findViewById(R.id.file_chooser_message);

        displayMessage();
    }

    private void displayMessage() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_INITIAL:
            vTitle.setText(R.string.filechooser_initial_title);
            vMessage.setText(R.string.filechooser_initial_desc);
            break;

        case PatcherConfigState.STATE_PATCHING:
        case PatcherConfigState.STATE_CHOSE_FILE:
            vTitle.setText(R.string.filechooser_ready_title);
            vMessage.setText(mContext.getString(R.string.filechooser_ready_desc, mPCS.mFilename));
            break;

        case PatcherConfigState.STATE_FINISHED:
            if (mPCS.mPatcherFailed) {
                vTitle.setText(R.string.filechooser_failure_title);
                vMessage.setText(String.format(mContext.getString(R.string
                        .filechooser_failure_desc), mPCS.mPatcherError));
            } else {
                vTitle.setText(R.string.filechooser_success_title);
                vMessage.setText(String.format(mContext.getString(R.string
                        .filechooser_success_desc), mPCS.mPatcherNewFile));
            }
            break;
        }
    }

    public void setEnabled(boolean enabled) {
        vTitle.setEnabled(enabled);
        vMessage.setEnabled(enabled);
        vCard.setClickable(enabled);
        vCard.setLongClickable(enabled);
    }

    @Override
    public void onCardCreate() {
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

        displayMessage();
    }

    @Override
    public void onRestoreCardState(Bundle savedInstanceState) {
    }

    @Override
    public void onSaveCardState(Bundle outState) {
    }

    @Override
    public void onChoseFile() {
        displayMessage();
    }

    @Override
    public void onStartedPatching() {
        setEnabled(false);
    }

    @Override
    public void onFinishedPatching() {
        setEnabled(true);
        displayMessage();
    }
}
