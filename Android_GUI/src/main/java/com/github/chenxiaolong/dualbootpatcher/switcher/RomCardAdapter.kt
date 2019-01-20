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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageButton
import android.widget.ImageView
import android.widget.TextView
import androidx.cardview.widget.CardView
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.switcher.RomCardAdapter.BaseViewHolder
import com.squareup.picasso.Picasso
import java.io.File

class RomCardAdapter(
        private val roms: List<RomInformation>,
        private val listener: RomCardActionListener?
) : RecyclerView.Adapter<BaseViewHolder>() {
    var activeRomId: String? = null

    private val onRomCardClickListener = object : RomCardClickListener {
        override fun onCardClick(view: View, position: Int) {
            listener?.onSelectedRom(roms[position])
        }

        override fun onCardMenuClick(position: Int) {
            listener?.onSelectedRomMenu(roms[position])
        }
    }

    override fun getItemViewType(position: Int): Int {
        return if (position == roms.size) {
            ITEM_TYPE_SPACER
        } else {
            ITEM_TYPE_ROM
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): BaseViewHolder {
        val inflater = LayoutInflater.from(parent.context)
        val view: View

        return when (viewType) {
            ITEM_TYPE_ROM -> {
                view = inflater.inflate(R.layout.card_v7_rom, parent, false)
                RomCardViewHolder(view, onRomCardClickListener)
            }
            ITEM_TYPE_SPACER -> {
                view = inflater.inflate(R.layout.card_v7_rom_spacer, parent, false)
                SpacerViewHolder(view)
            }
            else -> throw IllegalStateException("Invalid view type: $viewType")
        }
    }

    override fun onBindViewHolder(holder: BaseViewHolder, position: Int) {
        if (holder is RomCardViewHolder) {
            val rom = roms[position]
            holder.vName.text = rom.name

            val version = rom.version
            val build = rom.build

            if (version == null || version.isEmpty()) {
                holder.vVersion.setText(R.string.rom_card_unknown_version)
            } else {
                holder.vVersion.text = rom.version
            }
            if (build == null || build.isEmpty()) {
                holder.vBuild.setText(R.string.rom_card_unknown_build)
            } else {
                holder.vBuild.text = rom.build
            }

            val f = File(rom.thumbnailPath!!)
            if (f.exists() && f.canRead()) {
                Picasso.get()
                        .load(f)
                        .error(rom.imageResId)
                        .into(holder.vThumbnail)
            } else {
                Picasso.get()
                        .load(rom.imageResId)
                        .into(holder.vThumbnail)
            }

            if (activeRomId != null && activeRomId == rom.id) {
                holder.vActive.visibility = View.VISIBLE
            } else {
                holder.vActive.visibility = View.GONE
            }
        }
    }

    override fun getItemCount(): Int {
        return roms.size + 1
    }

    open class BaseViewHolder internal constructor(itemView: View) : ViewHolder(itemView)

    private class SpacerViewHolder(v: View) : BaseViewHolder(v)

    private class RomCardViewHolder(
            v: View,
            private val listener: RomCardClickListener
    ) : BaseViewHolder(v) {
        var vCard: CardView = v as CardView
        var vThumbnail: ImageView = v.findViewById(R.id.rom_thumbnail)
        var vActive: ImageView = v.findViewById(R.id.rom_active)
        var vName: TextView = v.findViewById(R.id.rom_name)
        var vVersion: TextView = v.findViewById(R.id.rom_version)
        var vBuild: TextView = v.findViewById(R.id.rom_build)
        var vMenu: ImageButton = v.findViewById(R.id.rom_menu)

        init {
            vCard.setOnClickListener { view -> listener.onCardClick(view, adapterPosition) }

            vMenu.setOnClickListener { listener.onCardMenuClick(adapterPosition) }
        }
    }

    interface RomCardClickListener {
        fun onCardClick(view: View, position: Int)

        fun onCardMenuClick(position: Int)
    }

    interface RomCardActionListener {
        fun onSelectedRom(info: RomInformation)

        fun onSelectedRomMenu(info: RomInformation)
    }

    companion object {
        private const val ITEM_TYPE_ROM = 1
        private const val ITEM_TYPE_SPACER = 2
    }
}