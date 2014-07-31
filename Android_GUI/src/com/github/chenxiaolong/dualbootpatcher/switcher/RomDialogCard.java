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

import android.content.Context;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.squareup.picasso.Picasso;

import java.io.File;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.CardThumbnail;

public class RomDialogCard extends Card {
    private EditText mEditText;
    private String mName;
    private OnClickListener mListener;
    private File mCacheFile;

    public class PicassoCardThumbnail extends CardThumbnail {
        private int mImageResId;

        public PicassoCardThumbnail(Context context, int imageResId) {
            super(context);
            mImageResId = imageResId;
        }

        @Override
        public void setupInnerViewElements(ViewGroup parent, View viewImage) {
            if (mCacheFile != null && mCacheFile.exists() && mCacheFile.canRead()) {
                // Don't cache the image since we may need to refresh it
                Picasso.with(getContext()).load(mCacheFile).resize(96, 96).skipMemoryCache()
                        .error(mImageResId).into((ImageView) viewImage);
            } else {
                Picasso.with(getContext()).load(mImageResId).resize(96, 96)
                        .into((ImageView) viewImage);
            }

            viewImage.setClickable(true);
            viewImage.setOnClickListener(mListener);
        }
    }

    public RomDialogCard(Context context, RomInformation romInfo, String name,
             int imageResId, OnClickListener listener) {
        super(context, R.layout.cardcontent_romdialog);
        mName = name;
        mListener = listener;

        mCacheFile = RomUtils.getThumbnailTempFile(getContext(), romInfo);

        PicassoCardThumbnail thumb = new PicassoCardThumbnail(getContext(), imageResId);
        thumb.setExternalUsage(true);
        addCardThumbnail(thumb);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        mEditText = (EditText) parent.findViewById(R.id.romdialog_name);
        mEditText.setText(mName);
    }

    public String getName() {
        return mEditText.getText().toString();
    }

    public void setName(String name) {
        mEditText.setText(name);
    }
}
