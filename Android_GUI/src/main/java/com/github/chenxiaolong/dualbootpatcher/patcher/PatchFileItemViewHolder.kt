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

package com.github.chenxiaolong.dualbootpatcher.patcher

import android.view.View
import android.widget.ProgressBar
import android.widget.TextView
import androidx.cardview.widget.CardView
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import com.github.chenxiaolong.dualbootpatcher.R

class PatchFileItemViewHolder(v: View, listener: PatchFileItemViewClickListener) : ViewHolder(v) {
    internal var vCard: CardView = v as CardView
    internal var vTitle: TextView = v.findViewById(R.id.action_title)
    internal var vSubtitle1: TextView = v.findViewById(R.id.action_subtitle1)
    internal var vSubtitle2: TextView = v.findViewById(R.id.action_subtitle2)
    internal var vProgress: ProgressBar = v.findViewById(R.id.progress_bar)
    internal var vProgressPercentage: TextView = v.findViewById(R.id.progress_percentage)
    internal var vProgressFiles: TextView = v.findViewById(R.id.progress_files)

    init {
        vCard.setOnClickListener { listener.onPatchFileItemClicked(adapterPosition) }
    }

    interface PatchFileItemViewClickListener {
        fun onPatchFileItemClicked(position: Int)
    }
}