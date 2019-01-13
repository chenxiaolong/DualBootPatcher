/*
 * Copyright (C) 2015-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import androidx.core.graphics.drawable.DrawableCompat
import android.view.Menu
import android.view.MenuItem

object MenuUtils {
    fun tintAllMenuIcons(menu: Menu, color: Int) {
        (0 until menu.size())
                .map { menu.getItem(it) }
                .forEach { tintMenuItemIcon(it, color) }
    }

    fun tintMenuItemIcon(item: MenuItem, color: Int) {
        val drawable = item.icon
        if (drawable != null) {
            val wrapped = DrawableCompat.wrap(drawable)
            DrawableCompat.setTint(wrapped, color)
        }
    }
}