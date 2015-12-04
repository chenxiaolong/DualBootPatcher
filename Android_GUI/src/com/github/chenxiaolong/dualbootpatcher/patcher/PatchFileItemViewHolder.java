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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.support.v7.widget.CardView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class PatchFileItemViewHolder extends ViewHolder {
    CardView vCard;
    TextView vTitle;
    TextView vSubtitle1;
    TextView vSubtitle2;
    ProgressBar vProgress;
    TextView vProgressPercentage;
    TextView vProgressFiles;

    public PatchFileItemViewHolder(View v) {
        super(v);
        vCard = (CardView) v;
        vTitle = (TextView) v.findViewById(R.id.action_title);
        vSubtitle1 = (TextView) v.findViewById(R.id.action_subtitle1);
        vSubtitle2 = (TextView) v.findViewById(R.id.action_subtitle2);
        vProgress = (ProgressBar) v.findViewById(R.id.progress_bar);
        vProgressPercentage = (TextView) v.findViewById(R.id.progress_percentage);
        vProgressFiles = (TextView) v.findViewById(R.id.progress_files);
    }
}