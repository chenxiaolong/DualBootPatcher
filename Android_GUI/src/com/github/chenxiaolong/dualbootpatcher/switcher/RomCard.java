package com.github.chenxiaolong.dualbootpatcher.switcher;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.CardThumbnail;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class RomCard extends Card {
    private TextView mTitle;
    private TextView mSubtitle;
    private TextView mMessage;
    private ProgressBar mProgressBar;

    private boolean mShowProgress;

    private final String mRom;
    private final String mName;
    private final String mVersion;
    private final int mImageResId;

    private int mStringResId;
    private boolean mShowMessage;

    private boolean mEnabled = true;

    public RomCard(Context context, String rom, String name, String version,
            int imageResId) {
        super(context, R.layout.cardcontent_switcher);
        mRom = rom;
        mName = name;
        mVersion = version;
        mImageResId = imageResId;

        CardThumbnail thumb = new CardThumbnail(getContext());
        thumb.setDrawableResource(mImageResId);
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

    private void updateMessage() {
        if (mShowMessage) {
            mMessage.setVisibility(View.VISIBLE);
            mMessage.setText(mStringResId);
        } else {
            mMessage.setVisibility(View.GONE);
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

    public String getRom() {
        return mRom;
    }

    public void setEnabled(boolean enabled) {
        mEnabled = enabled;

        if (getCardView() != null) {
            enableViews();
        }
    }

    public void onSaveInstanceState(Bundle outState) {
        String prefix = "romcard_" + mRom;
        outState.putBoolean(prefix + "_showprogress", mShowProgress);
        outState.putBoolean(prefix + "_showmessage", mShowMessage);
        outState.putInt(prefix + "_messageresid", mStringResId);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        String prefix = "romcard_" + mRom;
        mShowProgress = savedInstanceState.getBoolean(prefix + "_showprogress");
        mShowMessage = savedInstanceState.getBoolean(prefix + "_showmessage");
        mStringResId = savedInstanceState.getInt(prefix + "_messageresid");
    }
}
