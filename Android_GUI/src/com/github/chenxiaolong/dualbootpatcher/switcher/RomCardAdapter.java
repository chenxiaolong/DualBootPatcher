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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.content.Context;
import android.support.annotation.Nullable;
import android.support.v7.widget.CardView;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomCardAdapter.BaseViewHolder;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomCardAdapter.RomCardViewHolder;
import com.squareup.picasso.Picasso;

import java.io.File;
import java.util.List;

public class RomCardAdapter extends RecyclerView.Adapter<BaseViewHolder> {
    private static final int ITEM_TYPE_ROM = 1;
    private static final int ITEM_TYPE_SPACER = 2;

    private final Context mContext;
    private List<RomInformation> mRoms;
    private String mActiveRomId;
    private RomCardActionListener mListener;

    private RomCardClickListener onRomCardClickListener = new RomCardClickListener() {
        @Override
        public void onCardClick(View view, int position) {
            if (mListener != null) {
                mListener.onSelectedRom(mRoms.get(position));
            }
        }

        @Override
        public void onCardMenuClick(int position) {
            if (mListener != null) {
                mListener.onSelectedRomMenu(mRoms.get(position));
            }
        }
    };

    public RomCardAdapter(Context context, List<RomInformation> roms,
                          RomCardActionListener listener) {
        mContext = context;
        mRoms = roms;
        mListener = listener;
    }

    @Override
    public int getItemViewType(int position) {
        if (position == mRoms.size()) {
            return ITEM_TYPE_SPACER;
        } else {
            return ITEM_TYPE_ROM;
        }
    }

    @Override
    public BaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        LayoutInflater inflater = LayoutInflater.from(parent.getContext());
        View view;

        switch (viewType) {
        case ITEM_TYPE_ROM:
            view = inflater.inflate(R.layout.card_v7_rom, parent, false);
            return new RomCardViewHolder(view, onRomCardClickListener);
        case ITEM_TYPE_SPACER:
            view = inflater.inflate(R.layout.card_v7_rom_spacer, parent, false);
            return new SpacerViewHolder(view);
        default:
            throw new IllegalStateException("Invalid view type: " + viewType);
        }
    }

    @Override
    public void onBindViewHolder(BaseViewHolder holder, int position) {
        if (holder instanceof RomCardViewHolder) {
            RomCardViewHolder romVh = (RomCardViewHolder) holder;

            RomInformation rom = mRoms.get(position);
            romVh.vName.setText(rom.getName());

            String version = rom.getVersion();
            String build = rom.getBuild();

            if (version == null || version.isEmpty()) {
                romVh.vVersion.setText(R.string.rom_card_unknown_version);
            } else {
                romVh.vVersion.setText(rom.getVersion());
            }
            if (build == null || build.isEmpty()) {
                romVh.vBuild.setText(R.string.rom_card_unknown_build);
            } else {
                romVh.vBuild.setText(rom.getBuild());
            }

            File f = new File(rom.getThumbnailPath());
            if (f.exists() && f.canRead()) {
                Picasso.with(mContext)
                        .load(f)
                        .error(rom.getImageResId())
                        .into(romVh.vThumbnail);
            } else {
                Picasso.with(mContext)
                        .load(rom.getImageResId())
                        .into(romVh.vThumbnail);
            }

            if (mActiveRomId != null && mActiveRomId.equals(rom.getId())) {
                romVh.vActive.setVisibility(View.VISIBLE);
            } else {
                romVh.vActive.setVisibility(View.GONE);
            }
        }
    }

    @Override
    public int getItemCount() {
        return mRoms.size() + 1;
    }

    @Nullable
    public String getActiveRomId() {
        return mActiveRomId;
    }

    public void setActiveRomId(@Nullable String activeRomId) {
        mActiveRomId = activeRomId;
    }

    protected static class BaseViewHolder extends ViewHolder {
        BaseViewHolder(View itemView) {
            super(itemView);
        }
    }

    protected static class SpacerViewHolder extends BaseViewHolder {
        public SpacerViewHolder(View v) {
            super(v);
        }
    }

    protected static class RomCardViewHolder extends BaseViewHolder {
        private RomCardClickListener mListener;

        protected CardView vCard;
        protected ImageView vThumbnail;
        protected ImageView vActive;
        protected TextView vName;
        protected TextView vVersion;
        protected TextView vBuild;
        protected ImageButton vMenu;

        public RomCardViewHolder(View v, RomCardClickListener listener) {
            super(v);
            vCard = (CardView) v;
            vThumbnail = (ImageView) v.findViewById(R.id.rom_thumbnail);
            vActive = (ImageView) v.findViewById(R.id.rom_active);
            vName = (TextView) v.findViewById(R.id.rom_name);
            vVersion = (TextView) v.findViewById(R.id.rom_version);
            vBuild = (TextView) v.findViewById(R.id.rom_build);
            vMenu = (ImageButton) v.findViewById(R.id.rom_menu);

            mListener = listener;

            vCard.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    mListener.onCardClick(view, getAdapterPosition());
                }
            });

            vMenu.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    mListener.onCardMenuClick(getAdapterPosition());
                }
            });
        }
    }

    private interface RomCardClickListener {
        void onCardClick(View view, int position);

        void onCardMenuClick(int position);
    }

    public interface RomCardActionListener {
        void onSelectedRom(RomInformation info);

        void onSelectedRomMenu(RomInformation info);
    }
}