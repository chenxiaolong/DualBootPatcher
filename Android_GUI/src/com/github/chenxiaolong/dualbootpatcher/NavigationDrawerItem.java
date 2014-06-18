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

public class NavigationDrawerItem extends DrawerItem {
    private String mText;
    private int mImageResId;
    private boolean mShowProgress;
    private boolean mVisible = true;

    public NavigationDrawerItem(String text, int imageResId) {
        mText = text;
        mImageResId = imageResId;
    }

    public String getText() {
        return mText;
    }

    public void setText(String text) {
        mText = text;
    }

    public int getImageResourceId() {
        return mImageResId;
    }

    public void setImageResourceId(int imageResId) {
        mImageResId = imageResId;
    }

    public boolean isProgressShowing() {
        return mShowProgress;
    }

    public void setProgressShowing(boolean show) {
        mShowProgress = show;
    }

    public boolean isVisible() {
        return mVisible;
    }

    public void setVisible(boolean visible) {
        mVisible = visible;
    }
}
