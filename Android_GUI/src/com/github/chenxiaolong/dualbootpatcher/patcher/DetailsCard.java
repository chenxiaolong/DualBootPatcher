package com.github.chenxiaolong.dualbootpatcher.patcher;

import it.gmariotti.cardslib.library.internal.Card;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class DetailsCard extends Card {
    private TextView mDetails;
    private String mText;

    public DetailsCard(Context context) {
        this(context, R.layout.cardcontent_details);
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

    public void onDetatchBlahBlah() {
        mDetails = null;
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
    }
}