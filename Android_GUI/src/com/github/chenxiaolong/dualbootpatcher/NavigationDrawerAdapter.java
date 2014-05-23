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

package com.github.chenxiaolong.dualbootpatcher;

import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.TextView;

public class NavigationDrawerAdapter extends ArrayAdapter<NavigationDrawerItem> {
    private final Context mContext;
    private final List<NavigationDrawerItem> mItems;
    private final int mLayoutResId;

    public NavigationDrawerAdapter(Context context, int resource,
            List<NavigationDrawerItem> items) {
        super(context, resource, items);
        mContext = context;
        mItems = items;
        mLayoutResId = resource;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        LayoutInflater inflater = ((Activity) mContext).getLayoutInflater();
        View rowView = inflater.inflate(mLayoutResId, parent, false);

        TextView textView = (TextView) rowView.findViewById(R.id.drawer_text);
        ImageView imageView = (ImageView) rowView
                .findViewById(R.id.drawer_icon);
        ProgressBar progressBar = (ProgressBar) rowView
                .findViewById(R.id.drawer_progress);

        NavigationDrawerItem item = mItems.get(position);
        textView.setText(item.getText());
        imageView.setImageResource(item.getImageResourceId());
        // imageView.setImageDrawable(rowView.getResources().getDrawable(
        // item.getImageResourceId()));

        if (item.isProgressShowing()) {
            progressBar.setVisibility(View.VISIBLE);
        } else {
            progressBar.setVisibility(View.GONE);
        }

        return rowView;
    }
}
