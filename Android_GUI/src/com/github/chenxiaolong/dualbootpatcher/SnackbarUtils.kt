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

package com.github.chenxiaolong.dualbootpatcher

import android.content.Context
import android.support.annotation.StringRes
import android.support.design.widget.Snackbar
import android.support.v4.content.ContextCompat
import android.view.View
import android.widget.TextView

object SnackbarUtils {
    fun createSnackbar(context: Context, view: View, text: String, duration: Int): Snackbar {
        val snackbar = Snackbar.make(view, text, duration)
        snackbarSetTextColor(context, snackbar)
        snackbarSetBackgroundColor(context, snackbar)
        return snackbar
    }

    fun createSnackbar(context: Context, view: View, @StringRes resId: Int, duration: Int): Snackbar {
        val snackbar = Snackbar.make(view, resId, duration)
        snackbarSetTextColor(context, snackbar)
        snackbarSetBackgroundColor(context, snackbar)
        return snackbar
    }

    private fun snackbarSetTextColor(context: Context, snackbar: Snackbar) {
        val colorResId = if (MainApplication.useDarkTheme) {
            R.color.snackbar_text_dark
        } else {
            R.color.snackbar_text_light
        }

        val view = snackbar.view
        val textView: TextView = view.findViewById(android.support.design.R.id.snackbar_text)
        textView.setTextColor(ContextCompat.getColor(context, colorResId))
    }

    private fun snackbarSetBackgroundColor(context: Context, snackbar: Snackbar) {
        val colorResId = if (MainApplication.useDarkTheme) {
            R.color.snackbar_background_dark
        } else {
            R.color.snackbar_background_light
        }

        val view = snackbar.view
        view.setBackgroundColor(ContextCompat.getColor(context, colorResId))
    }
}