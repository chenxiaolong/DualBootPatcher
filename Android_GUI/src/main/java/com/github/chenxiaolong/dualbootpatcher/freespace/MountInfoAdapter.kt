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

import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.RecyclerView
import java.util.*

internal class MountInfoAdapter constructor(
        private val colors: IntArray
) : RecyclerView.Adapter<MountInfoViewHolder>() {
    private var mounts = emptyList<MountInfo>()

    // Choose random color to start from (and go down the list)
    private val startingColorIndex: Int = Random().nextInt(colors.size)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): MountInfoViewHolder =
            MountInfoViewHolder.create(parent)

    override fun onBindViewHolder(holder: MountInfoViewHolder, position: Int) {
        val mount = mounts[position]
        val color = colors[(startingColorIndex + position) % colors.size]

        holder.bindTo(mount, color)
    }

    override fun getItemCount(): Int {
        return mounts.size
    }

    fun setList(mounts: List<MountInfo>) {
        val result = DiffUtil.calculateDiff(object : DiffUtil.Callback() {
            override fun getOldListSize(): Int {
                return this@MountInfoAdapter.mounts.size
            }

            override fun getNewListSize(): Int {
                return mounts.size
            }

            override fun areItemsTheSame(oldItemPosition: Int, newItemPosition: Int): Boolean {
                return this@MountInfoAdapter.mounts[oldItemPosition].mountPoint ==
                        mounts[newItemPosition].mountPoint
            }

            override fun areContentsTheSame(oldItemPosition: Int,
                                            newItemPosition: Int): Boolean {
                return this@MountInfoAdapter.mounts[oldItemPosition] == mounts[newItemPosition]
            }
        })

        this.mounts = mounts
        result.dispatchUpdatesTo(this)
    }
}