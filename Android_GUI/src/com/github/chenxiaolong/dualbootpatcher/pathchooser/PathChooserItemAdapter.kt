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

import android.support.v7.widget.RecyclerView
import android.view.LayoutInflater
import android.view.ViewGroup

import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.pathchooser.PathChooserItemViewHolder.PathChooserItemViewClickListener

internal class PathChooserItemAdapter(
        private val items: List<String>,
        private val listener: PathChooserItemClickListener?
) : RecyclerView.Adapter<PathChooserItemViewHolder>(), PathChooserItemViewClickListener {
    internal interface PathChooserItemClickListener {
        fun onPathChooserItemClicked(position: Int, item: String)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): PathChooserItemViewHolder {
        val view = LayoutInflater
                .from(parent.context)
                .inflate(R.layout.list_path_chooser_item, parent, false)
        return PathChooserItemViewHolder(view, this)
    }

    override fun onBindViewHolder(holder: PathChooserItemViewHolder, position: Int) {
        val item = items[position]

        holder.vTitle.text = item
    }

    override fun getItemCount(): Int {
        return items.size
    }

    override fun onPathChooserItemClicked(position: Int) {
        listener?.onPathChooserItemClicked(position, items[position])
    }
}