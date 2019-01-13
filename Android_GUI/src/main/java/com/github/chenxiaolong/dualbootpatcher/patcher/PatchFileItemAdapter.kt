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

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.patcher.PatchFileItemViewHolder.PatchFileItemViewClickListener

class PatchFileItemAdapter(private val context: Context, private val items: List<PatchFileItem>,
                           private val listener: PatchFileItemClickListener?)
        : RecyclerView.Adapter<PatchFileItemViewHolder>(), PatchFileItemViewClickListener {
    interface PatchFileItemClickListener {
        fun onPatchFileItemClicked(item: PatchFileItem)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): PatchFileItemViewHolder {
        val view = LayoutInflater
                .from(parent.context)
                .inflate(R.layout.card_v7_patch_file_item, parent, false)
        return PatchFileItemViewHolder(view, this)
    }

    override fun onBindViewHolder(holder: PatchFileItemViewHolder, position: Int) {
        val item = items[position]

        holder.vTitle.text = item.displayName
        holder.vSubtitle1.text = context.getString(
                R.string.patcher_card_subtitle_target, item.device.id, item.romId)

        // Clickability
        holder.vCard.isClickable = item.state === PatchFileState.QUEUED

        when (item.state) {
            PatchFileState.QUEUED,
            PatchFileState.PENDING,
            PatchFileState.CANCELLED,
            PatchFileState.COMPLETED -> {
                holder.vSubtitle2.visibility = View.VISIBLE
                if (item.state === PatchFileState.QUEUED) {
                    holder.vSubtitle2.setText(R.string.patcher_card_subtitle_queued)
                } else if (item.state === PatchFileState.PENDING) {
                    holder.vSubtitle2.setText(R.string.patcher_card_subtitle_pending)
                } else if (item.state === PatchFileState.CANCELLED) {
                    holder.vSubtitle2.setText(R.string.patcher_card_subtitle_cancelled)
                } else if (item.state === PatchFileState.COMPLETED) {
                    if (item.successful) {
                        holder.vSubtitle2.setText(R.string.patcher_card_subtitle_succeeded)
                    } else {
                        holder.vSubtitle2.text = context.getString(
                                R.string.patcher_card_subtitle_failed, item.errorCode)
                    }
                }
                holder.vProgress.visibility = View.GONE
                holder.vProgressPercentage.visibility = View.GONE
                holder.vProgressFiles.visibility = View.GONE
            }
            PatchFileState.IN_PROGRESS -> {
                holder.vSubtitle2.visibility = View.GONE
                holder.vProgress.visibility = View.VISIBLE
                holder.vProgressPercentage.visibility = View.VISIBLE
                holder.vProgressFiles.visibility = View.VISIBLE
            }
        }

        // Normalize progress to 0-1000000 range to prevent integer overflow
        val normalize = 1000000
        val percentage: Double
        val value: Int
        val max: Int
        if (item.maxBytes == 0L) {
            percentage = 0.0
            value = 0
            max = 0
        } else {
            percentage = item.bytes.toDouble() / item.maxBytes
            value = (percentage * normalize).toInt()
            max = normalize
        }
        holder.vProgress.max = max
        holder.vProgress.progress = value
        holder.vProgress.isIndeterminate = item.maxBytes == 0L

        // Percentage progress text
        holder.vProgressPercentage.text = String.format("%.1f%%", 100.0 * percentage)

        // Files progress
        holder.vProgressFiles.text = context.getString(R.string.overall_progress_files,
                item.files, item.maxFiles)
    }

    override fun getItemCount(): Int {
        return items.size
    }

    override fun onPatchFileItemClicked(position: Int) {
        listener?.onPatchFileItemClicked(items[position])
    }
}