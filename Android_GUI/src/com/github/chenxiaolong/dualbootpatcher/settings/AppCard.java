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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.settings.AppListLoaderFragment.AppInformation;
import com.github.chenxiaolong.dualbootpatcher.settings.AppListLoaderFragment.RomInfoResult;

import java.util.ArrayList;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.CardExpand;
import it.gmariotti.cardslib.library.internal.CardThumbnail;
import it.gmariotti.cardslib.library.internal.CardThumbnail.CustomSource;
import it.gmariotti.cardslib.library.internal.ViewToClickToExpand;
import it.gmariotti.cardslib.library.internal.ViewToClickToExpand.CardElementUI;

public class AppCard extends Card {
    private final AppInformation mAppInfo;
    private final AppSharingConfigFile mConfig;
    private final RomInfoResult mRomInfos;

    public AppCard(Context context, AppSharingConfigFile config, AppInformation appInfo,
                   RomInfoResult romInfos) {
        super(context, R.layout.cardcontent_app);
        mConfig = config;
        mAppInfo = appInfo;
        mRomInfos = romInfos;

        CardThumbnail thumb = new CardThumbnail(getContext());
        thumb.setCustomSource(new BitmapSource());
        addCardThumbnail(thumb);

        AppCardExpand expand = new AppCardExpand(getContext());
        //expand.setTitle(mAppInfo.roms.toString());
        addCardExpand(expand);

        ViewToClickToExpand vtcte = ViewToClickToExpand.builder()
                .setupCardElement(CardElementUI.CARD);
        setViewToClickToExpand(vtcte);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        TextView title = (TextView) parent.findViewById(R.id.app_title);
        TextView subtitle = (TextView) parent.findViewById(R.id.app_subtitle);

        title.setText(mAppInfo.name);
        subtitle.setText(createInstalledInText());
    }

    private String createInstalledInText() {
        StringBuilder sb = new StringBuilder();
        sb.append(getContext().getString(R.string.installed_in_roms));
        sb.append(" ");

        for (int i = 0; i < mAppInfo.roms.size(); i++) {
            RomInformation rom = mAppInfo.roms.get(i);

            for (int j = 0; j < mRomInfos.roms.length; j++) {
                if (rom == mRomInfos.roms[j]) {
                    sb.append(mRomInfos.names[j]);
                }
            }

            if (i < mAppInfo.roms.size() - 1) {
                sb.append(", ");
            }
        }

        return sb.toString();
    }

    private class BitmapSource implements CustomSource {
        @Override
        public String getTag() {
            return mAppInfo.pkg;
        }

        @Override
        public Bitmap getBitmap() {
            return drawableToBitmap(mAppInfo.icon);
        }

        // From https://github.com/gabrielemariotti/cardslib/issues/26
        private Bitmap drawableToBitmap(Drawable drawable) {
            if (drawable instanceof BitmapDrawable) {
                return ((BitmapDrawable) drawable).getBitmap();
            }

            Bitmap bitmap = Bitmap.createBitmap(drawable.getIntrinsicWidth(),
                    drawable.getIntrinsicHeight(), Bitmap.Config.ARGB_8888);
            Canvas canvas = new Canvas(bitmap);
            drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
            drawable.draw(canvas);

            return bitmap;
        }
    }

    private class AppCardExpand extends CardExpand {
        private final LayoutInflater mInflater;

        public AppCardExpand(Context context) {
            super(context, R.layout.cardexpand_app);
            mInflater = LayoutInflater.from(context);
        }

        @Override
        public void setupInnerViewElements(ViewGroup parent, View view) {
            if (view == null) {
                return;
            }

            ArrayList<CheckBox> checkboxes;

            if (view.getTag() == null) {
                checkboxes = new ArrayList<CheckBox>();

                LinearLayout ll = (LinearLayout) view.findViewById(R.id.expanded_layout);

                for (int i = 0; i < mRomInfos.roms.length; i++) {
                    CheckBox checkbox = (CheckBox) mInflater.inflate(
                            R.layout.app_rom_checkbox, ll, false);

                    ll.addView(checkbox);
                    checkboxes.add(checkbox);
                }

                view.setTag(checkboxes);
            } else {
                checkboxes = (ArrayList<CheckBox>) view.getTag();
            }

            for (int i = 0; i < mRomInfos.roms.length; i++) {
                final RomInformation rom = mRomInfos.roms[i];
                final CheckBox checkbox = checkboxes.get(i);

                checkbox.setText(mRomInfos.names[i]);

                // Clear listener before setting checked status since the view is recycled
                checkbox.setOnCheckedChangeListener(null);

                checkbox.setChecked(mConfig.isRomSynced(mAppInfo.pkg, rom));

                checkbox.setOnCheckedChangeListener(new OnCheckedChangeListener() {
                    @Override
                    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                        mConfig.setRomSynced(mAppInfo.pkg, rom, isChecked);
                    }
                });
            }
        }
    }
}
