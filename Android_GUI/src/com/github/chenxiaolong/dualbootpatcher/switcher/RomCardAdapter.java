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
import android.support.v7.widget.CardView;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.PopupMenu;
import android.widget.PopupMenu.OnMenuItemClickListener;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomCardAdapter.RomCardViewHolder;
import com.squareup.picasso.MemoryPolicy;
import com.squareup.picasso.Picasso;

import java.io.File;
import java.util.List;

public class RomCardAdapter extends RecyclerView.Adapter<RomCardViewHolder> {
    private static final int MENU_SET_KERNEL = Menu.FIRST;
    private static final int MENU_ADD_TO_HOME_SCREEN = Menu.FIRST + 1;
    private static final int MENU_EDIT_NAME = Menu.FIRST + 2;
    private static final int MENU_CHANGE_IMAGE = Menu.FIRST + 3;
    private static final int MENU_RESET_IMAGE = Menu.FIRST + 4;
    private static final int MENU_WIPE_ROM = Menu.FIRST + 5;

    private final Context mContext;
    private List<RomInformation> mRoms;
    private RomCardActionListener mListener;

    private RomCardClickListener onRomCardClickListener = new RomCardClickListener() {
        @Override
        public void onCardClick(View view, int position) {
            if (mListener != null) {
                mListener.onSelectedRom(mRoms.get(position));
            }
        }

        @Override
        public void onCardPopupClick(View view, int position, MenuItem item) {
            RomInformation rom = mRoms.get(position);

            if (mListener != null) {
                switch (item.getItemId()) {
                case MENU_SET_KERNEL:
                    mListener.onSelectedSetKernel(rom);
                    break;
                case MENU_ADD_TO_HOME_SCREEN:
                    mListener.onSelectedAddToHomeScreen(rom);
                    break;
                case MENU_EDIT_NAME:
                    mListener.onSelectedEditName(rom);
                    break;
                case MENU_CHANGE_IMAGE:
                    mListener.onSelectedChangeImage(rom);
                    break;
                case MENU_RESET_IMAGE:
                    mListener.onSelectedResetImage(rom);
                    break;
                case MENU_WIPE_ROM:
                    mListener.onSelectedWipeRom(rom);
                    break;
                }
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
    public RomCardViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater
                .from(parent.getContext())
                .inflate(R.layout.card_v7_rom, parent, false);
        RomCardViewHolder vh = new RomCardViewHolder(view, mContext, onRomCardClickListener);

        Menu menu = vh.mPopup.getMenu();
        menu.add(0, MENU_SET_KERNEL, Menu.NONE, R.string.rom_menu_set_kernel);
        menu.add(0, MENU_ADD_TO_HOME_SCREEN, Menu.NONE, R.string.rom_menu_add_to_home_screen);
        menu.add(0, MENU_EDIT_NAME, Menu.NONE, R.string.rom_menu_edit_name);
        menu.add(0, MENU_CHANGE_IMAGE, Menu.NONE, R.string.rom_menu_change_image);
        menu.add(0, MENU_RESET_IMAGE, Menu.NONE, R.string.rom_menu_reset_image);
        menu.add(0, MENU_WIPE_ROM, Menu.NONE, R.string.rom_menu_wipe_rom);

        return vh;
    }

    @Override
    public void onBindViewHolder(RomCardViewHolder holder, int position) {
        RomInformation rom = mRoms.get(position);
        holder.vName.setText(rom.getName());

        String version = rom.getVersion();
        String build = rom.getBuild();

        if (version == null || version.isEmpty()) {
            holder.vVersion.setText(R.string.rom_card_unknown_version);
        } else {
            holder.vVersion.setText(rom.getVersion());
        }
        if (build == null || build.isEmpty()) {
            holder.vBuild.setText(R.string.rom_card_unknown_build);
        } else {
            holder.vBuild.setText(rom.getBuild());
        }

        File f = new File(rom.getThumbnailPath());
        if (f.exists() && f.canRead()) {
            // Don't cache the image since we may need to refresh it
            Picasso.with(mContext)
                    .load(f)
                    .memoryPolicy(MemoryPolicy.NO_CACHE)
                    .error(rom.getImageResId())
                    .into(holder.vThumbnail);
        } else {
            Picasso.with(mContext)
                    .load(rom.getImageResId())
                    .into(holder.vThumbnail);
        }
    }

    @Override
    public int getItemCount() {
        return mRoms.size();
    }

    protected static class RomCardViewHolder extends ViewHolder {
        private RomCardClickListener mListener;
        protected PopupMenu mPopup;

        protected CardView vCard;
        protected ImageView vThumbnail;
        protected TextView vName;
        protected TextView vVersion;
        protected TextView vBuild;
        protected ImageButton vMenu;

        public RomCardViewHolder(View v, Context context, RomCardClickListener listener) {
            super(v);
            vCard = (CardView) v;
            vThumbnail = (ImageView) v.findViewById(R.id.rom_thumbnail);
            vName = (TextView) v.findViewById(R.id.rom_name);
            vVersion = (TextView) v.findViewById(R.id.rom_version);
            vBuild = (TextView) v.findViewById(R.id.rom_build);
            vMenu = (ImageButton) v.findViewById(R.id.rom_menu);

            mListener = listener;
            mPopup = new PopupMenu(context, vMenu);

            vCard.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    mListener.onCardClick(view, getAdapterPosition());
                }
            });

            vMenu.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    mPopup.show();
                }
            });

            mPopup.setOnMenuItemClickListener(new OnMenuItemClickListener() {
                @Override
                public boolean onMenuItemClick(MenuItem menuItem) {
                    mListener.onCardPopupClick(vCard, getAdapterPosition(), menuItem);
                    return true;
                }
            });
        }
    }

    private interface RomCardClickListener {
        void onCardClick(View view, int position);

        void onCardPopupClick(View view, int position, MenuItem item);
    }

    public interface RomCardActionListener {
        void onSelectedRom(RomInformation info);

        void onSelectedSetKernel(RomInformation info);

        void onSelectedAddToHomeScreen(RomInformation info);

        void onSelectedEditName(RomInformation info);

        void onSelectedChangeImage(RomInformation info);

        void onSelectedResetImage(RomInformation info);

        void onSelectedWipeRom(RomInformation info);
    }
}