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

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.view.CardView;

import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class DetailsCard extends Card {
    private PatcherConfigState mPCS;

    private TextView mDetails;
    private String mText;

    public DetailsCard(Context context, PatcherConfigState pcs) {
        this(context, R.layout.cardcontent_details);
        mPCS = pcs;
    }

    public DetailsCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mDetails = (TextView) view.findViewById(R.id.details_text);
            if (mText != null) {
                mDetails.setText(mText);
            }
        }
    }

    public void setDetails(String text) {
        mText = text;
        mDetails.setText(text);
    }

    public void reset() {
        mText = "";
        mDetails.setText("");
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putString("details_text", mText);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        mText = savedInstanceState.getString("details_text");
        mDetails.setText(mText);
    }

    public void refreshState() {
        switch (mPCS.mState) {
        case PatcherConfigState.STATE_PATCHING:
            if (getCardView() != null) {
                ((CardView) getCardView()).setVisibility(View.VISIBLE);
            }
            break;

        case PatcherConfigState.STATE_CHOSE_FILE:
        case PatcherConfigState.STATE_INITIAL:
        case PatcherConfigState.STATE_FINISHED:
            if (getCardView() != null) {
                ((CardView) getCardView()).setVisibility(View.GONE);
            }
            break;
        }
    }
}
