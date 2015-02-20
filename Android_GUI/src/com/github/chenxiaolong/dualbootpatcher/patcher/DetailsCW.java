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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.os.Bundle;
import android.support.v7.widget.CardView;
import android.view.View;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

import it.gmariotti.cardslib.library.view.CardViewNative;

public class DetailsCW {
    private static final String EXTRA_DETAILS_TEXT = "details_text";

    private CardView vCard;
    private TextView vDetails;

    private PatcherConfigState mPCS;

    private String mText;

    public DetailsCW(PatcherConfigState pcs, CardView card) {
        mPCS = pcs;

        vCard = card;
        vDetails = (TextView) card.findViewById(R.id.details_text);

        if (mText != null) {
            vDetails.setText(mText);
        }
    }

    public void setDetails(String text) {
        mText = text;
        vDetails.setText(text);
    }

    public void reset() {
        mText = "";
        vDetails.setText("");
    }

    public void onSaveInstanceState(Bundle outState) {
        outState.putString(EXTRA_DETAILS_TEXT, mText);
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        mText = savedInstanceState.getString(EXTRA_DETAILS_TEXT);
        vDetails.setText(mText);
    }

    public void refreshState() {
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
    }
}
