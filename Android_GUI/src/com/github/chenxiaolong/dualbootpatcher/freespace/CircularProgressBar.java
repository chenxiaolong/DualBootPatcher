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
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.RectF;
import android.os.Parcel;
import android.os.Parcelable;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;

import com.github.chenxiaolong.dualbootpatcher.R;

import java.util.Random;

public class CircularProgressBar extends View {
    // Paint
    private Paint mBackgroundPaint = new Paint();
    private Paint mProgressPaint = new Paint();
    private Paint mBorderPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

    private final RectF mMainBounds = new RectF();

    private float mBorderWidth = -1;
    private float mProgressWidth = -1;

    private float mOuterBorderRadius = 0;
    private float mInnerBorderRadius = 0;

    // Colors
    private int mBackgroundColor;
    private int mProgressColor;

    private float mProgress;
    private boolean mRandomRotation;

    private float mTranslationOffsetX;
    private float mTranslationOffsetY;

    private Random mRandom = new Random();

    public CircularProgressBar(final Context context) {
        this(context, null);
    }

    public CircularProgressBar(final Context context, final AttributeSet attrs) {
        this(context, attrs, R.attr.circularProgressBarStyle);
    }

    public CircularProgressBar(final Context context, final AttributeSet attrs,
                               final int defStyle) {
        super(context, attrs, defStyle);

        // load the styled attributes and set their properties
        final TypedArray attributes = context.obtainStyledAttributes(attrs,
                R.styleable.CircularProgressBar, defStyle, 0);

        setProgressColor(attributes.getColor(
                R.styleable.CircularProgressBar_progress_color, Color.BLACK));
        setProgressBackgroundColor(attributes.getColor(
                R.styleable.CircularProgressBar_background_color, Color.WHITE));
        setProgress(attributes.getFloat(
                R.styleable.CircularProgressBar_progress, 0.0f));

        mBorderWidth = attributes.getDimension(
                R.styleable.CircularProgressBar_border_width, dpi2px(1));
        mProgressWidth = attributes.getDimension(
                R.styleable.CircularProgressBar_progress_width, dpi2px(25));
        mRandomRotation = attributes.getBoolean(
                R.styleable.CircularProgressBar_random_rotation, false);

        attributes.recycle();

        updateBackgroundColor();

        updateProgressColor();
    }

    @Override
    protected void onRestoreInstanceState(final Parcelable state) {
        if (!(state instanceof SavedState)) {
            super.onRestoreInstanceState(state);
            return;
        }

        SavedState ss = (SavedState) state;
        super.onRestoreInstanceState(ss.getSuperState());

        if (mBackgroundColor != ss.backgroundColor) {
            mBackgroundColor = ss.backgroundColor;
            updateBackgroundColor();
        }

        if (mProgressColor != ss.progressColor) {
            mProgressColor = ss.progressColor;
            updateProgressColor();
        }

        setProgress(ss.progress);
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        Parcelable superState = super.onSaveInstanceState();
        SavedState ss = new SavedState(superState);
        ss.backgroundColor = mBackgroundColor;
        ss.progressColor = mProgressColor;
        ss.progress = mProgress;
        return ss;
    }

    @Override
    protected void onDraw(final Canvas canvas) {
        canvas.translate(mTranslationOffsetX, mTranslationOffsetY);

        final float progressRotation = getCurrentRotation();

        // Draw the borders
        mBorderPaint.setColor(mProgressColor);
        mBorderPaint.setStyle(Style.STROKE);
        mBorderPaint.setStrokeWidth(mBorderWidth);

        canvas.drawCircle(0, 0, mOuterBorderRadius, mBorderPaint);
        canvas.drawCircle(0, 0, mInnerBorderRadius, mBorderPaint);

        int startAngle = 270;
        if (mRandomRotation) {
            startAngle = mRandom.nextInt(360);
        }

        // Draw the background
        canvas.drawArc(mMainBounds, startAngle, -(360 - progressRotation), false, mBackgroundPaint);

        // Draw the progress
        canvas.drawArc(mMainBounds, startAngle, progressRotation, false, mProgressPaint);
    }

    @Override
    protected void onMeasure(final int widthMeasureSpec, final int heightMeasureSpec) {
        final int height = getDefaultSize(getSuggestedMinimumHeight(), heightMeasureSpec);
        final int width = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
        final int min = Math.min(width, height);
        setMeasuredDimension(min, height);

        final float halfWidth = min * 0.5f;
        float max = halfWidth;

        // Outer border
        mOuterBorderRadius = max - mBorderWidth / 2;
        max -= mBorderWidth;

        // Draw the progress ring
        float progressRadius = max - mProgressWidth / 2;
        mMainBounds.set(-progressRadius, -progressRadius, progressRadius, progressRadius);
        max -= mProgressWidth;

        // Inner border
        mInnerBorderRadius = max - mBorderWidth / 2;
        max -= mBorderWidth;

        mTranslationOffsetX = halfWidth;
        mTranslationOffsetY = halfWidth;
    }

    private float getCurrentRotation() {
        return 360 * mProgress;
    }

    private void updateBackgroundColor() {
        mBackgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mBackgroundPaint.setColor(mBackgroundColor);
        mBackgroundPaint.setStyle(Style.STROKE);
        mBackgroundPaint.setStrokeWidth(mProgressWidth);

        invalidate();
    }

    private void updateProgressColor() {
        mProgressPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        mProgressPaint.setColor(mProgressColor);
        mProgressPaint.setStyle(Style.STROKE);
        mProgressPaint.setStrokeWidth(mProgressWidth);

        invalidate();
    }

    public float getProgress() {
        return mProgress;
    }

    public int getProgressColor() {
        return mProgressColor;
    }

    public void setProgress(final float progress) {
        if (progress == mProgress) {
            return;
        }

        if (progress >= 1) {
            mProgress = 1;
        } else {
            mProgress = progress;
        }

        invalidate();
    }

    public void setProgressBackgroundColor(final int color) {
        mBackgroundColor = color;

        updateBackgroundColor();
    }

    public void setProgressColor(final int color) {
        mProgressColor = color;

        updateProgressColor();
    }

    private float dpi2px(float dpi) {
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dpi,
                getResources().getDisplayMetrics());
    }

    public static class SavedState extends BaseSavedState {
        private int backgroundColor;
        private int progressColor;
        private float progress;
        private boolean randomRotation;

        SavedState(Parcelable superState) {
            super(superState);
        }

        private SavedState(Parcel in) {
            super(in);
            backgroundColor = in.readInt();
            progressColor = in.readInt();
            progress = in.readFloat();
            randomRotation = in.readInt() != 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            super.writeToParcel(out, flags);
            out.writeInt(backgroundColor);
            out.writeInt(progressColor);
            out.writeFloat(progress);
            out.writeInt(randomRotation ? 1 : 0);
        }

        public static final Creator<SavedState> CREATOR = new Creator<SavedState>() {
            public SavedState createFromParcel(Parcel in) {
                return new SavedState(in);
            }

            public SavedState[] newArray(int size) {
                return new SavedState[size];
            }
        };
    }
}
