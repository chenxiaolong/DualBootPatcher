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

package com.github.chenxiaolong.dualbootpatcher.appsharing;

import android.support.v7.widget.CardView;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppCardAdapter.AppCardViewHolder;
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppListFragment.AppInformation;

import java.util.ArrayList;
import java.util.List;

public class AppCardAdapter extends RecyclerView.Adapter<AppCardViewHolder> {
    private List<AppInformation> mApps;
    private AppCardActionListener mListener;

    private AppCardClickListener onAppCardClickListener = new AppCardClickListener() {
        @Override
        public void onCardClick(View view, int position) {
            if (mListener != null) {
                mListener.onSelectedApp(mApps.get(position));
            }
        }
    };

    public AppCardAdapter(List<AppInformation> apps, AppCardActionListener listener) {
        mApps = new ArrayList<>(apps);
        mListener = listener;
    }

    @Override
    public AppCardViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext())
                .inflate(R.layout.card_v7_app, parent, false);
        return new AppCardViewHolder(view, onAppCardClickListener);
    }

    @Override
    public void onBindViewHolder(AppCardViewHolder holder, int position) {
        AppInformation app = mApps.get(position);
        holder.vName.setText(app.name);
        holder.vThumbnail.setImageDrawable(app.icon);
        holder.vPkg.setText(app.pkg);
        holder.vSystem.setVisibility(app.isSystem ? View.VISIBLE : View.GONE);

        if (app.shareApk && app.shareData) {
            holder.vShared.setText(R.string.indiv_app_sharing_shared_apk_and_data);
        } else if (app.shareApk) {
            holder.vShared.setText(R.string.indiv_app_sharing_shared_apk);
        } else if (app.shareData) {
            holder.vShared.setText(R.string.indiv_app_sharing_shared_data);
        } else {
            holder.vShared.setText(R.string.indiv_app_sharing_not_shared);
        }
    }

    @Override
    public int getItemCount() {
        return mApps.size();
    }

    protected static class AppCardViewHolder extends ViewHolder {
        private AppCardClickListener mListener;

        protected CardView vCard;
        protected ImageView vThumbnail;
        protected TextView vName;
        protected TextView vPkg;
        protected TextView vShared;
        protected ImageView vSystem;

        public AppCardViewHolder(View v, AppCardClickListener listener) {
            super(v);
            vCard = (CardView) v;
            vThumbnail = (ImageView) v.findViewById(R.id.app_thumbnail);
            vName = (TextView) v.findViewById(R.id.app_name);
            vPkg = (TextView) v.findViewById(R.id.app_pkg);
            vShared = (TextView) v.findViewById(R.id.app_shared);
            vSystem = (ImageView) v.findViewById(R.id.app_system_icon);

            mListener = listener;

            vCard.setOnClickListener(new OnClickListener() {
                @Override
                public void onClick(View view) {
                    mListener.onCardClick(view, getAdapterPosition());
                }
            });
        }
    }

    private interface AppCardClickListener {
        void onCardClick(View view, int position);
    }

    public interface AppCardActionListener {
        void onSelectedApp(AppInformation info);
    }

    public void setTo(List<AppInformation> infos) {
        mApps.clear();
        mApps.addAll(infos);
        notifyDataSetChanged();
    }

    public void animateTo(List<AppInformation> infos) {
        applyAndAnimateRemovals(infos);
        applyAndAnimateAdditions(infos);
        applyAndAnimateMovedItems(infos);
    }

    private void applyAndAnimateRemovals(List<AppInformation> newInfos) {
        for (int i = mApps.size() - 1; i >= 0; i--) {
            final AppInformation info = mApps.get(i);
            if (!newInfos.contains(info)) {
                removeItem(i);
            }
        }
    }

    private void applyAndAnimateAdditions(List<AppInformation> newInfos) {
        for (int i = 0, count = newInfos.size(); i < count; i++) {
            final AppInformation info = newInfos.get(i);
            if (!mApps.contains(info)) {
                addItem(i, info);
            }
        }
    }

    private void applyAndAnimateMovedItems(List<AppInformation> newInfos) {
        for (int toPosition = newInfos.size() - 1; toPosition >= 0; toPosition--) {
            final AppInformation info = newInfos.get(toPosition);
            final int fromPosition = mApps.indexOf(info);
            if (fromPosition >= 0 && fromPosition != toPosition) {
                moveItem(fromPosition, toPosition);
            }
        }
    }

    public AppInformation removeItem(int position) {
        final AppInformation info = mApps.remove(position);
        notifyItemRemoved(position);
        return info;
    }

    public void addItem(int position, AppInformation info) {
        mApps.add(position, info);
        notifyItemInserted(position);
    }

    public void moveItem(int fromPosition, int toPosition) {
        final AppInformation info = mApps.remove(fromPosition);
        mApps.add(toPosition, info);
        notifyItemMoved(fromPosition, toPosition);
    }
}