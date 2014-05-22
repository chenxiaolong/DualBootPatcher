package com.github.chenxiaolong.dualbootpatcher;

public class NavigationDrawerItem {
    private String mText;
    private int mImageResId;
    private boolean mShowProgress;

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
}
