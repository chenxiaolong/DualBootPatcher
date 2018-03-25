/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.pathchooser

import android.support.v7.widget.RecyclerView.ViewHolder
import android.view.View
import android.widget.TextView

internal class PathChooserItemViewHolder(v: View, listener: PathChooserItemViewClickListener) : ViewHolder(v) {
    var vTitle: TextView = v.findViewById(android.R.id.title)

    init {
        v.setOnClickListener { listener.onPathChooserItemClicked(adapterPosition) }
    }

    internal interface PathChooserItemViewClickListener {
        fun onPathChooserItemClicked(position: Int)
    }
}