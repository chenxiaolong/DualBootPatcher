/*
 * Copyright (C) 2014-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.annotation.ColorInt
import androidx.recyclerview.widget.RecyclerView
import com.github.chenxiaolong.dualbootpatcher.FileUtils
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.views.CircularProgressBar

internal class MountInfoViewHolder(val v: View) : RecyclerView.ViewHolder(v) {
    private var vMountPoint: TextView = v.findViewById(R.id.mount_point)
    private var vTotalSize: TextView = v.findViewById(R.id.size_total)
    private var vAvailSize: TextView = v.findViewById(R.id.size_free)
    private var vProgress: CircularProgressBar = v.findViewById(R.id.mountpoint_usage)

    fun bindTo(mount: MountInfo, @ColorInt color: Int) {
        val strTotal = FileUtils.toHumanReadableSize(v.context, mount.totalSpace, 2)
        val strAvail = FileUtils.toHumanReadableSize(v.context, mount.availSpace, 2)

        vMountPoint.text = mount.mountPoint
        vTotalSize.text = strTotal
        vAvailSize.text = strAvail

        if (mount.totalSpace == 0L) {
            vProgress.progress = 0f
        } else {
            vProgress.progress = 1.0f - mount.availSpace.toFloat() / mount.totalSpace
        }

        vProgress.progressColor = color
    }

    companion object {
        fun create(parent: ViewGroup): MountInfoViewHolder {
            val view = LayoutInflater
                    .from(parent.context)
                    .inflate(R.layout.card_v7_free_space, parent, false)
            return MountInfoViewHolder(view)
        }
    }
}