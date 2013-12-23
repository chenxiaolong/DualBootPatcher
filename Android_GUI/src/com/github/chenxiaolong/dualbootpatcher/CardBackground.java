package com.github.chenxiaolong.dualbootpatcher;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.util.DisplayMetrics;
import android.util.TypedValue;

// Design based on http://stackoverflow.com/questions/17571118/how-to-create-google-cards-ui-in-a-list-view/17571277#17571277

public class CardBackground extends Drawable {
    private DisplayMetrics mDisplayMetrics = null;
    private boolean showCard = true;

    // Padding
    private int mPaddingTop = 10;
    private int mPaddingBottom = 10;
    private int mPaddingLeft = 10;
    private int mPaddingRight = 10;

    public CardBackground(DisplayMetrics displayMetrics) {
        super();
        mDisplayMetrics = displayMetrics;
    }

    private float dpToPx(int dp) {
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp,
                mDisplayMetrics);
    }

    @Override
    public void draw(Canvas canvas) {
        Paint greyPaint = new Paint();
        greyPaint.setARGB(0xFF, 0xEE, 0xEE, 0xEE);
        greyPaint.setStyle(Style.FILL);
        Rect grey = new Rect(0, 0, canvas.getWidth(), canvas.getHeight());
        canvas.drawRect(grey, greyPaint);

        if (showCard) {
            Paint shadowPaint = new Paint();
            shadowPaint.setARGB(0x8C, 0x66, 0x66, 0x66);
            shadowPaint.setStyle(Style.FILL);
            RectF shadow = new RectF(dpToPx(mPaddingLeft), dpToPx(mPaddingTop),
                    canvas.getWidth() - dpToPx(mPaddingRight),
                    canvas.getHeight() - dpToPx(mPaddingBottom));
            canvas.drawRoundRect(shadow, dpToPx(3), dpToPx(3), shadowPaint);

            Paint whitePaint = new Paint();
            whitePaint.setARGB(0xFF, 0xFF, 0xFF, 0xFF);
            whitePaint.setStyle(Style.FILL);
            RectF white = new RectF(dpToPx(mPaddingLeft), dpToPx(mPaddingTop),
                    canvas.getWidth() - dpToPx(mPaddingRight),
                    canvas.getHeight() - dpToPx(mPaddingBottom + 2));
            canvas.drawRoundRect(white, dpToPx(3), dpToPx(3), whitePaint);
        }
    }

    @Override
    public int getOpacity() {
        return PixelFormat.TRANSLUCENT;
    }

    @Override
    public void setAlpha(int arg0) {
    }

    @Override
    public void setColorFilter(ColorFilter arg0) {
    }

    public void widgetOnTop() {
        mPaddingTop /= 2;
    }

    public void widgetOnBottom() {
        mPaddingBottom /= 2;
    }

    public void widgetOnLeft() {
        mPaddingLeft /= 2;
    }

    public void widgetOnRight() {
        mPaddingRight /= 2;
    }

    public void setNoCard(boolean b) {
        showCard = !b;
    }
}
