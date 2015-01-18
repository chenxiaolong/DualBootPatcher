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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.squareup.picasso.Picasso;

import java.io.File;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.CardThumbnail;

public class RomCard extends Card {
    private TextView mTitle;
    private TextView mMessage;
    private ProgressBar mProgressBar;

    private boolean mShowProgress;

    private final RomInformation mRomInfo;
    private String mName;
    private final File mFile;

    private int mStringResId;
    private boolean mShowMessage;

    private boolean mEnabled = true;

    private class PicassoCardThumbnail extends CardThumbnail {
        private int mImageResId;

        public PicassoCardThumbnail(Context context, int imageResId) {
            super(context);
            mImageResId = imageResId;
        }

        @Override
        public void setupInnerViewElements(ViewGroup parent, View viewImage) {
            if (mFile.exists() && mFile.canRead()) {
                // Don't cache the image since we may need to refresh it
                Picasso.with(getContext()).load(mFile).resize(96, 96).skipMemoryCache()
                        .error(mImageResId).into((ImageView) viewImage);
            } else {
                Picasso.with(getContext()).load(mImageResId).resize(96, 96)
                        .into((ImageView) viewImage);
            }
        }
    }

    public RomCard(Context context, RomInformation info, String name, int imageResId) {
        super(context, R.layout.cardcontent_switcher);
        mRomInfo = info;
        mName = name;

        mFile = new File(mRomInfo.thumbnailPath);

        PicassoCardThumbnail thumb = new PicassoCardThumbnail(getContext(), imageResId);
        thumb.setExternalUsage(true);
        addCardThumbnail(thumb);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        mTitle = (TextView) parent.findViewById(R.id.switcher_title);
        mMessage = (TextView) parent.findViewById(R.id.switcher_message);
        mProgressBar = (ProgressBar) parent
                .findViewById(R.id.switcher_progress);

        mTitle.setText(mName);

        enableViews();
        updateViews();
        updateMessage();
    }

    private void updateViews() {
        if (mTitle != null) {
            if (mShowProgress) {
                mTitle.setVisibility(View.GONE);
                mProgressBar.setVisibility(View.VISIBLE);
            } else {
                mTitle.setVisibility(View.VISIBLE);
                mProgressBar.setVisibility(View.GONE);
            }
        }
    }

    private void updateMessage() {
        if (mMessage != null) {
            if (mShowMessage) {
                mMessage.setVisibility(View.VISIBLE);
                mMessage.setText(mStringResId);
            } else {
                mMessage.setVisibility(View.GONE);
            }
        }
    }

    private void enableViews() {
        mTitle.setEnabled(mEnabled);
        setClickable(mEnabled);
        setLongClickable(mEnabled);

        //getCardView().setClickable(mEnabled);
        getCardView().setLongClickable(mEnabled);
    }

    public boolean isProgressShowing() {
        return mShowProgress;
    }

    public void setProgressShowing(boolean show) {
        mShowProgress = show;
        updateViews();
    }

    public void showCompletionMessage(int action, boolean failed) {
        if (action == SwitcherListFragment.ACTION_CHOOSE_ROM) {
            mStringResId = failed ? R.string.choose_rom_failure
                    : R.string.choose_rom_success;
        } else if (action == SwitcherListFragment.ACTION_SET_KERNEL) {
            mStringResId = failed ? R.string.set_kernel_failure
                    : R.string.set_kernel_success;
        }

        mShowMessage = true;
        updateMessage();
    }

    public void hideMessage() {
        mShowMessage = false;
        updateMessage();
    }

    public RomInformation getRom() {
        return mRomInfo;
    }

    public void setEnabled(boolean enabled) {
        mEnabled = enabled;

        if (getCardView() != null) {
            enableViews();
        }
    }

    public void setName(String name) {
        mName = name;
        mTitle.setText(name);
    }

    public void onSaveInstanceState(Bundle outState) {
        String prefix = "romcard_" + mRomInfo.id;
        outState.putBoolean(prefix + "_showprogress", mShowProgress);
        outState.putBoolean(prefix + "_showmessage", mShowMessage);
        outState.putInt(prefix + "_messageresid", mStringResId);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        String prefix = "romcard_" + mRomInfo.id;
        mShowProgress = savedInstanceState.getBoolean(prefix + "_showprogress");
        mShowMessage = savedInstanceState.getBoolean(prefix + "_showmessage");
        mStringResId = savedInstanceState.getInt(prefix + "_messageresid");
    }
}
