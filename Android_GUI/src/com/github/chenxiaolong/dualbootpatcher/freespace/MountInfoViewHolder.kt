/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.freespace

import android.support.v7.widget.RecyclerView
import android.view.View
import android.widget.TextView
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.views.CircularProgressBar

internal class MountInfoViewHolder(v: View) : RecyclerView.ViewHolder(v) {
    var vMountPoint: TextView = v.findViewById(R.id.mount_point)
    var vTotalSize: TextView = v.findViewById(R.id.size_total)
    var vAvailSize: TextView = v.findViewById(R.id.size_free)
    var vProgress: CircularProgressBar = v.findViewById(R.id.mountpoint_usage)
}