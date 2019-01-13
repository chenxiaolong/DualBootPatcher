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

package com.github.chenxiaolong.dualbootpatcher.views

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Paint.Style
import android.graphics.RectF
import android.os.Parcel
import android.os.Parcelable
import android.util.AttributeSet
import android.util.TypedValue
import android.view.View

import com.github.chenxiaolong.dualbootpatcher.R

import java.util.Random

class CircularProgressBar @JvmOverloads constructor(
        context: Context, attrs: AttributeSet? = null,
        defStyle: Int = R.attr.circularProgressBarStyle
) : View(context, attrs, defStyle) {
    // Paint
    private var backgroundPaint = Paint()
    private var progressPaint = Paint()
    private val borderPaint = Paint(Paint.ANTI_ALIAS_FLAG)

    private val mainBounds = RectF()

    private var borderWidth = -1f
    private var progressWidth = -1f

    private var outerBorderRadius = 0f
    private var innerBorderRadius = 0f

    // Colors
    private var _backgroundColor: Int = 0
    private var _progressColor: Int = 0

    var progress: Float = 0.toFloat()
        set(progress) {
            if (progress == this.progress) {
                return
            }

            field = if (progress >= 1) {
                1f
            } else {
                progress
            }

            invalidate()
        }
    private val randomRotation: Boolean

    private var translationOffsetX: Float = 0f
    private var translationOffsetY: Float = 0f

    private val random = Random()

    private val currentRotation: Float
        get() = 360 * progress

    var progressColor: Int
        get() = _progressColor
        set(color) {
            _progressColor = color

            updateProgressColor()
        }

    init {
        // load the styled attributes and set their properties
        val attributes = context.obtainStyledAttributes(attrs,
                R.styleable.CircularProgressBar, defStyle, 0)

        progressColor = attributes.getColor(
                R.styleable.CircularProgressBar_progress_color, Color.BLACK)
        setProgressBackgroundColor(attributes.getColor(
                R.styleable.CircularProgressBar_background_color, Color.WHITE))
        progress = attributes.getFloat(
                R.styleable.CircularProgressBar_progress, 0.0f)

        borderWidth = attributes.getDimension(
                R.styleable.CircularProgressBar_border_width, dpi2px(1f))
        progressWidth = attributes.getDimension(
                R.styleable.CircularProgressBar_progress_width, dpi2px(25f))
        randomRotation = attributes.getBoolean(
                R.styleable.CircularProgressBar_random_rotation, false)

        attributes.recycle()

        updateBackgroundColor()

        updateProgressColor()
    }

    override fun onRestoreInstanceState(state: Parcelable) {
        if (state !is SavedState) {
            super.onRestoreInstanceState(state)
            return
        }

        super.onRestoreInstanceState(state.superState)

        if (_backgroundColor != state.backgroundColor) {
            _backgroundColor = state.backgroundColor
            updateBackgroundColor()
        }

        if (_progressColor != state.progressColor) {
            _progressColor = state.progressColor
            updateProgressColor()
        }

        progress = state.progress
    }

    override fun onSaveInstanceState(): Parcelable? {
        val superState = super.onSaveInstanceState()
        val ss = SavedState(superState)
        ss.backgroundColor = _backgroundColor
        ss.progressColor = _progressColor
        ss.progress = progress
        ss.randomRotation = randomRotation
        return ss
    }

    override fun onDraw(canvas: Canvas) {
        canvas.translate(translationOffsetX, translationOffsetY)

        val progressRotation = currentRotation

        // Draw the borders
        borderPaint.color = _progressColor
        borderPaint.style = Style.STROKE
        borderPaint.strokeWidth = borderWidth

        canvas.drawCircle(0f, 0f, outerBorderRadius, borderPaint)
        canvas.drawCircle(0f, 0f, innerBorderRadius, borderPaint)

        var startAngle = 270
        if (randomRotation) {
            startAngle = random.nextInt(360)
        }

        // Draw the background
        canvas.drawArc(mainBounds, startAngle.toFloat(), -(360 - progressRotation), false, backgroundPaint)

        // Draw the progress
        canvas.drawArc(mainBounds, startAngle.toFloat(), progressRotation, false, progressPaint)
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val height = View.getDefaultSize(suggestedMinimumHeight, heightMeasureSpec)
        val width = View.getDefaultSize(suggestedMinimumWidth, widthMeasureSpec)
        val min = Math.min(width, height)
        setMeasuredDimension(min, height)

        val halfWidth = min * 0.5f
        var max = halfWidth

        // Outer border
        outerBorderRadius = max - borderWidth / 2
        max -= borderWidth

        // Draw the progress ring
        val progressRadius = max - progressWidth / 2
        mainBounds.set(-progressRadius, -progressRadius, progressRadius, progressRadius)
        max -= progressWidth

        // Inner border
        innerBorderRadius = max - borderWidth / 2
        max -= borderWidth

        translationOffsetX = halfWidth
        translationOffsetY = halfWidth
    }

    private fun updateBackgroundColor() {
        backgroundPaint = Paint(Paint.ANTI_ALIAS_FLAG)
        backgroundPaint.color = _backgroundColor
        backgroundPaint.style = Style.STROKE
        backgroundPaint.strokeWidth = progressWidth

        invalidate()
    }

    private fun updateProgressColor() {
        progressPaint = Paint(Paint.ANTI_ALIAS_FLAG)
        progressPaint.color = _progressColor
        progressPaint.style = Style.STROKE
        progressPaint.strokeWidth = progressWidth

        invalidate()
    }

    fun setProgressBackgroundColor(color: Int) {
        _backgroundColor = color

        updateBackgroundColor()
    }

    private fun dpi2px(dpi: Float): Float {
        return TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dpi,
                resources.displayMetrics)
    }

    class SavedState : View.BaseSavedState {
        internal var backgroundColor: Int = 0
        internal var progressColor: Int = 0
        internal var progress: Float = 0f
        internal var randomRotation: Boolean = false

        internal constructor(superState: Parcelable) : super(superState)

        private constructor(p: Parcel) : super(p) {
            backgroundColor = p.readInt()
            progressColor = p.readInt()
            progress = p.readFloat()
            randomRotation = p.readInt() != 0
        }

        override fun writeToParcel(out: Parcel, flags: Int) {
            super.writeToParcel(out, flags)
            out.writeInt(backgroundColor)
            out.writeInt(progressColor)
            out.writeFloat(progress)
            out.writeInt(if (randomRotation) 1 else 0)
        }

        companion object {
            @JvmField val CREATOR = object : Parcelable.Creator<SavedState> {
                override fun createFromParcel(p: Parcel): SavedState {
                    return SavedState(p)
                }

                override fun newArray(size: Int): Array<SavedState?> {
                    return arrayOfNulls(size)
                }
            }
        }
    }
}
