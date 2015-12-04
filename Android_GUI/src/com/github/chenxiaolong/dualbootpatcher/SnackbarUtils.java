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

package com.github.chenxiaolong.dualbootpatcher;

import android.content.Context;
import android.support.annotation.StringRes;
import android.support.design.widget.Snackbar;
import android.support.v4.content.ContextCompat;
import android.view.View;
import android.widget.TextView;

public class SnackbarUtils {
    public static Snackbar createSnackbar(Context context, View view, String text, int duration) {
        Snackbar snackbar = Snackbar.make(view, text, duration);
        snackbarSetTextColor(context, snackbar);
        return snackbar;
    }

    public static Snackbar createSnackbar(Context context, View view, @StringRes int resId, int duration) {
        Snackbar snackbar = Snackbar.make(view, resId, duration);
        snackbarSetTextColor(context, snackbar);
        return snackbar;
    }

    private static void snackbarSetTextColor(Context context, Snackbar snackbar) {
        View view = snackbar.getView();
        TextView textView = (TextView) view.findViewById(android.support.design.R.id.snackbar_text);
        textView.setTextColor(ContextCompat.getColor(context, R.color.text_color_primary));
    }
}
