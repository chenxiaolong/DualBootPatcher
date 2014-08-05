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

package com.github.chenxiaolong.dualbootpatcher.freespace;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

import java.text.DecimalFormat;

import it.gmariotti.cardslib.library.internal.Card;

public class FreeSpaceCard extends Card {
    private final String mMountPoint;
    private final long mTotalSize;
    private final long mAvailSpace;
    private final int mColor;

    public FreeSpaceCard(Context context, String mountpoint,
             long totalSize, long availSpace, int color) {
        super(context, R.layout.cardcontent_free_space);
        mMountPoint = mountpoint;
        mTotalSize = totalSize;
        mAvailSpace = availSpace;
        mColor = color;
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        TextView mountPoint = (TextView) parent.findViewById(R.id.mount_point);
        TextView total = (TextView) parent.findViewById(R.id.size_total);
        TextView free = (TextView) parent.findViewById(R.id.size_free);
        CircularProgressBar progress =
                (CircularProgressBar) parent.findViewById(R.id.mountpoint_usage);

        DecimalFormat sizeFormat = new DecimalFormat("#.##");
        String strTotal;
        String strAvail;

        if (mTotalSize < 1e3) {
            strTotal = String.format(mContext.getString(R.string.format_bytes), mTotalSize);
            strAvail = String.format(mContext.getString(R.string.format_bytes), mAvailSpace);
        } else if (mTotalSize < 1e6) {
            strTotal = String.format(mContext.getString(R.string.format_kilobytes),
                    sizeFormat.format((double) mTotalSize / 1e3));
            strAvail = String.format(mContext.getString(R.string.format_kilobytes),
                    sizeFormat.format((double) mAvailSpace / 1e3));
        } else if (mTotalSize < 1e9) {
            strTotal = String.format(mContext.getString(R.string.format_megabytes),
                    sizeFormat.format((double) mTotalSize / 1e6));
            strAvail = String.format(mContext.getString(R.string.format_megabytes),
                    sizeFormat.format((double) mAvailSpace / 1e6));
        } else {
            strTotal = String.format(mContext.getString(R.string.format_gigabytes),
                    sizeFormat.format((double) mTotalSize / 1e9));
            strAvail = String.format(mContext.getString(R.string.format_gigabytes),
                    sizeFormat.format((double) mAvailSpace / 1e9));
        }

        mountPoint.setText(mMountPoint);
        total.setText(strTotal);
        free.setText(strAvail);

        if (mTotalSize == 0) {
            progress.setProgress(0);
        } else {
            progress.setProgress(1.0f - (float) mAvailSpace / mTotalSize);
        }

        progress.setProgressColor(mColor);
    }
}
