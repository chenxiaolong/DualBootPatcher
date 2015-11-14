/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.drawable.Drawable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.BaseViewHolder;
import com.squareup.picasso.Picasso;

import java.io.File;
import java.util.List;

public class RomDetailAdapter extends RecyclerView.Adapter<BaseViewHolder> {
    public static final int ITEM_TYPE_ROM_CARD = 1;
    public static final int ITEM_TYPE_INFO = 2;
    public static final int ITEM_TYPE_ACTION = 3;

    public static class Item {
        int type;

        public Item(int type) {
            this.type = type;
        }
    }

    public static class RomCardItem extends Item {
        RomInformation romInfo;

        public RomCardItem(RomInformation romInfo) {
            super(ITEM_TYPE_ROM_CARD);
            this.romInfo = romInfo;
        }
    }

    public static class InfoItem extends Item {
        int id;
        String title;
        String value;

        public InfoItem(int id, String title, String value) {
            super(ITEM_TYPE_INFO);
            this.id = id;
            this.title = title;
            this.value = value;
        }
    }

    public static class ActionItem extends Item {
        int id;
        int iconResId;
        String title;

        public ActionItem(int id, int iconResId, String title) {
            super(ITEM_TYPE_ACTION);
            this.id = id;
            this.iconResId = iconResId;
            this.title = title;
        }
    }

    public static abstract class BaseViewHolder extends RecyclerView.ViewHolder {
        int id;

        BaseViewHolder(View itemView) {
            super(itemView);
        }

        public abstract void display(Item item);
    }

    public static class CardViewHolder extends BaseViewHolder {
        ImageView vThumbnail;
        TextView vName;
        TextView vVersion;
        TextView vBuild;

        CardViewHolder(View itemView) {
            super(itemView);
            vThumbnail = (ImageView) itemView.findViewById(R.id.rom_thumbnail);
            vName = (TextView) itemView.findViewById(R.id.rom_name);
            vVersion = (TextView) itemView.findViewById(R.id.rom_version);
            vBuild = (TextView) itemView.findViewById(R.id.rom_build);
        }

        @Override
        public void display(Item item) {
            RomCardItem romCardItem = (RomCardItem) item;
            RomInformation romInfo = romCardItem.romInfo;

            // Load thumbnail
            Context context = vThumbnail.getContext();
            File f = new File(romInfo.getThumbnailPath());
            if (f.exists() && f.canRead()) {
                Picasso.with(context).load(f).error(romInfo.getImageResId()).into(vThumbnail);
            } else {
                Picasso.with(context).load(romInfo.getImageResId()).into(vThumbnail);
            }

            // Load name, version, build
            vName.setText(romInfo.getName());
            vVersion.setText(romInfo.getVersion());
            vBuild.setText(romInfo.getBuild());
        }
    }

    public static class InfoViewHolder extends BaseViewHolder {
        TextView vTitle;
        TextView vValue;

        InfoViewHolder(View itemView) {
            super(itemView);
            vTitle = (TextView) itemView.findViewById(R.id.title);
            vValue = (TextView) itemView.findViewById(R.id.value);
        }

        @Override
        public void display(Item item) {
            InfoItem infoItem = (InfoItem) item;
            vTitle.setText(infoItem.title);
            vValue.setText(infoItem.value);
        }
    }

    public static class ActionViewHolder extends BaseViewHolder {
        private RomDetailItemClickListener mListener;

        ImageView vIcon;
        TextView vTitle;

        ActionViewHolder(View itemView, RomDetailItemClickListener listener) {
            super(itemView);
            mListener = listener;
            vIcon = (ImageView) itemView.findViewById(R.id.action_icon);
            vTitle = (TextView) itemView.findViewById(R.id.action_title);

            itemView.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    mListener.onActionItemClicked(view, getAdapterPosition());
                }
            });
        }

        @Override
        public void display(Item item) {
            ActionItem actionItem = (ActionItem) item;
            vIcon.setImageResource(actionItem.iconResId);
            vTitle.setText(actionItem.title);
        }
    }

    private RomDetailItemClickListener mOnItemClicked = new RomDetailItemClickListener() {
        @Override
        public void onActionItemClicked(View view, int position) {
            if (mListener != null) {
                mListener.onActionItemSelected((ActionItem) mItems.get(position));
            }
        }
    };

    private interface RomDetailItemClickListener {
        void onActionItemClicked(View view, int position);
    }

    public interface RomDetailAdapterListener {
        void onActionItemSelected(ActionItem item);
    }

    private List<Item> mItems;
    private RomDetailAdapterListener mListener;

    RomDetailAdapter(List<Item> items, RomDetailAdapterListener listener) {
        mItems = items;
        mListener = listener;
    }

    @Override
    public int getItemViewType(int position) {
        return mItems.get(position).type;
    }

    @Override
    public BaseViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        int layoutId;

        switch (viewType) {
        case ITEM_TYPE_ROM_CARD:
            layoutId = R.layout.rom_detail_card_item;
            break;
        case ITEM_TYPE_INFO:
            layoutId = R.layout.rom_detail_info_item;
            break;
        case ITEM_TYPE_ACTION:
            layoutId = R.layout.rom_detail_action_item;
            break;
        default:
            throw new IllegalStateException("Invalid viewType ID");
        }

        View view = LayoutInflater
                .from(parent.getContext())
                .inflate(layoutId, parent, false);

        switch (viewType) {
        case ITEM_TYPE_ROM_CARD:
            return new CardViewHolder(view);
        case ITEM_TYPE_INFO:
            return new InfoViewHolder(view);
        case ITEM_TYPE_ACTION:
            return new ActionViewHolder(view, mOnItemClicked);
        default:
            throw new IllegalStateException("Invalid viewType ID");
        }
    }

    @Override
    public void onBindViewHolder(BaseViewHolder holder, int position) {
        holder.display(mItems.get(position));
    }

    @Override
    public int getItemCount() {
        return mItems.size();
    }

    public static class DividerItemDecoration extends RecyclerView.ItemDecoration {
        private static final int[] ATTRS = new int[]{ android.R.attr.listDivider };
        private Drawable mDivider;

        public DividerItemDecoration(Context context) {
            final TypedArray styledAttributes = context.obtainStyledAttributes(ATTRS);
            mDivider = styledAttributes.getDrawable(0);
            styledAttributes.recycle();
        }

        @Override
        public void onDraw(Canvas c, RecyclerView parent, RecyclerView.State state) {
            int left = parent.getPaddingLeft();
            int right = parent.getWidth() - parent.getPaddingRight();

            int childCount = parent.getChildCount();
            for (int i = 0; i < childCount; i++) {
                View child = parent.getChildAt(i);

                RecyclerView.LayoutParams params =
                        (RecyclerView.LayoutParams) child.getLayoutParams();

                int top = child.getBottom() + params.bottomMargin;
                int bottom = top + mDivider.getIntrinsicHeight();

                mDivider.setBounds(left, top, right, bottom);
                mDivider.draw(c);
            }
        }
    }
}