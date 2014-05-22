package com.github.chenxiaolong.dualbootpatcher.switcher;

import it.gmariotti.cardslib.library.internal.Card;
import android.content.Context;

import com.github.chenxiaolong.dualbootpatcher.R;

public class NoRootCard extends Card {
    public NoRootCard(Context context) {
        this(context, R.layout.cardcontent_noroot);
    }

    public NoRootCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }
}
