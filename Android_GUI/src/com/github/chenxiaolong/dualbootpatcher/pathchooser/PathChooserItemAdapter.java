/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.pathchooser;

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.pathchooser.PathChooserItemViewHolder
        .PathChooserItemViewClickListener;

import java.util.List;

class PathChooserItemAdapter extends RecyclerView.Adapter<PathChooserItemViewHolder>
        implements PathChooserItemViewClickListener {
    private List<String> mItems;
    private PathChooserItemClickListener mListener;

    interface PathChooserItemClickListener {
        void onPathChooserItemClicked(int position, String item);
    }

    PathChooserItemAdapter(List<String> items, PathChooserItemClickListener listener) {
        mItems = items;
        mListener = listener;
    }

    @Override
    public PathChooserItemViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater
                .from(parent.getContext())
                .inflate(R.layout.list_path_chooser_item, parent, false);
        return new PathChooserItemViewHolder(view, this);
    }

    @Override
    public void onBindViewHolder(PathChooserItemViewHolder holder, int position) {
        String item = mItems.get(position);

        holder.vTitle.setText(item);
    }

    @Override
    public int getItemCount() {
        return mItems.size();
    }

    @Override
    public void onPathChooserItemClicked(int position) {
        if (mListener != null) {
            mListener.onPathChooserItemClicked(position, mItems.get(position));
        }
    }
}