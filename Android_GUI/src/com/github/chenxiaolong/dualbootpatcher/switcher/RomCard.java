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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.CardThumbnail;
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

public class RomCard extends Card {
    private TextView mTitle;
    private TextView mSubtitle;
    private TextView mMessage;
    private ProgressBar mProgressBar;

    private boolean mShowProgress;

    private final RomInformation mRomInfo;
    private final String mName;
    private final String mVersion;

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
            Picasso.with(getContext()).load(mImageResId).resize(96, 96).into((ImageView) viewImage);
        }
    }

    public RomCard(Context context, RomInformation info, String name, String version,
            int imageResId) {
        super(context, R.layout.cardcontent_switcher);
        mRomInfo = info;
        mName = name;
        mVersion = version;

        PicassoCardThumbnail thumb = new PicassoCardThumbnail(getContext(), imageResId);
        thumb.setExternalUsage(true);
        addCardThumbnail(thumb);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        mTitle = (TextView) parent.findViewById(R.id.switcher_title);
        mSubtitle = (TextView) parent.findViewById(R.id.switcher_subtitle);
        mMessage = (TextView) parent.findViewById(R.id.switcher_message);
        mProgressBar = (ProgressBar) parent
                .findViewById(R.id.switcher_progress);

        mTitle.setText(mName);
        mSubtitle.setText(mVersion);

        enableViews();
        updateViews();
        updateMessage();
    }

    private void updateViews() {
        if (mTitle != null) {
            if (mShowProgress) {
                mTitle.setVisibility(View.GONE);
                mSubtitle.setVisibility(View.GONE);
                mProgressBar.setVisibility(View.VISIBLE);
            } else {
                mTitle.setVisibility(View.VISIBLE);
                mSubtitle.setVisibility(View.VISIBLE);
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
        mSubtitle.setEnabled(mEnabled);
        setClickable(mEnabled);
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
            mStringResId = failed ? R.string.write_kernel_failure
                    : R.string.write_kernel_success;
        } else if (action == SwitcherListFragment.ACTION_SET_KERNEL) {
            mStringResId = failed ? R.string.back_up_kernel_failure
                    : R.string.back_up_kernel_success;
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

    public void onSaveInstanceState(Bundle outState) {
        String prefix = "romcard_" + mRomInfo.kernelId;
        outState.putBoolean(prefix + "_showprogress", mShowProgress);
        outState.putBoolean(prefix + "_showmessage", mShowMessage);
        outState.putInt(prefix + "_messageresid", mStringResId);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        String prefix = "romcard_" + mRomInfo.kernelId;
        mShowProgress = savedInstanceState.getBoolean(prefix + "_showprogress");
        mShowMessage = savedInstanceState.getBoolean(prefix + "_showmessage");
        mStringResId = savedInstanceState.getInt(prefix + "_messageresid");
    }
}
