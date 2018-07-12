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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.content.Context
import android.graphics.Canvas
import android.graphics.drawable.Drawable
import android.support.annotation.ColorInt
import android.support.v4.content.ContextCompat
import android.support.v4.graphics.drawable.DrawableCompat
import android.support.v7.widget.RecyclerView
import android.util.TypedValue
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView

import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.BaseViewHolder
import com.squareup.picasso.Picasso

import java.io.File

class RomDetailAdapter internal constructor(
        private val items: List<Item>,
        private val listener: RomDetailAdapterListener?
) : RecyclerView.Adapter<BaseViewHolder>() {
    private val onItemClicked = object : RomDetailItemClickListener {
        override fun onActionItemClicked(view: View, position: Int) {
            listener?.onActionItemSelected(items[position] as ActionItem)
        }
    }

    open class Item(internal var type: Int)

    class RomCardItem(internal var romInfo: RomInformation) : Item(ITEM_TYPE_ROM_CARD)

    class InfoItem(
            internal var id: Int,
            internal var title: String,
            internal var value: String
    ) : Item(ITEM_TYPE_INFO)

    class ActionItem(
            internal var id: Int,
            internal var iconResId: Int,
            internal var title: String
    ) : Item(ITEM_TYPE_ACTION)

    abstract class BaseViewHolder internal constructor(itemView: View)
            : RecyclerView.ViewHolder(itemView) {
        internal var id: Int = 0

        abstract fun display(item: Item)
    }

    class CardViewHolder internal constructor(itemView: View) : BaseViewHolder(itemView) {
        private var vThumbnail: ImageView = itemView.findViewById(R.id.rom_thumbnail)
        private var vName: TextView = itemView.findViewById(R.id.rom_name)
        private var vVersion: TextView = itemView.findViewById(R.id.rom_version)
        private var vBuild: TextView = itemView.findViewById(R.id.rom_build)

        override fun display(item: Item) {
            val romCardItem = item as RomCardItem
            val romInfo = romCardItem.romInfo

            // Load thumbnail
            val f = File(romInfo.thumbnailPath!!)
            if (f.exists() && f.canRead()) {
                Picasso.get().load(f).error(romInfo.imageResId).into(vThumbnail)
            } else {
                Picasso.get().load(romInfo.imageResId).into(vThumbnail)
            }

            // Load name, version, build
            vName.text = romInfo.name
            vVersion.text = romInfo.version
            vBuild.text = romInfo.build
        }
    }

    class InfoViewHolder internal constructor(itemView: View) : BaseViewHolder(itemView) {
        private var vTitle: TextView = itemView.findViewById(R.id.title)
        private var vValue: TextView = itemView.findViewById(R.id.value)

        override fun display(item: Item) {
            val infoItem = item as InfoItem
            vTitle.text = infoItem.title
            vValue.text = infoItem.value
        }
    }

    class ActionViewHolder internal constructor(
            itemView: View,
            private val listener: RomDetailItemClickListener
    ) : BaseViewHolder(itemView) {
        private var vIcon: ImageView = itemView.findViewById(R.id.action_icon)
        private var vTitle: TextView = itemView.findViewById(R.id.action_title)

        init {
            itemView.setOnClickListener {
                view -> listener.onActionItemClicked(view, adapterPosition)
            }
        }

        override fun display(item: Item) {
            val actionItem = item as ActionItem
            // Tint drawable
            val context = vIcon.context
            val drawable = ContextCompat.getDrawable(context, actionItem.iconResId)
            val wrapped = DrawableCompat.wrap(drawable!!)
            DrawableCompat.setTint(wrapped, getThemeTextColor(context))
            vIcon.setImageDrawable(wrapped)
            //vIcon.setImageResource(actionItem.iconResId);
            vTitle.text = actionItem.title
        }

        @ColorInt
        private fun getThemeTextColor(context: Context): Int {
            val value = TypedValue()
            context.theme.resolveAttribute(android.R.attr.textColor, value, true)
            return value.data
        }
    }

    internal interface RomDetailItemClickListener {
        fun onActionItemClicked(view: View, position: Int)
    }

    interface RomDetailAdapterListener {
        fun onActionItemSelected(item: ActionItem)
    }

    override fun getItemViewType(position: Int): Int {
        return items[position].type
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): BaseViewHolder {
        val layoutId = when (viewType) {
            ITEM_TYPE_ROM_CARD -> R.layout.rom_detail_card_item
            ITEM_TYPE_INFO -> R.layout.rom_detail_info_item
            ITEM_TYPE_ACTION -> R.layout.rom_detail_action_item
            else -> throw IllegalStateException("Invalid viewType ID")
        }

        val view = LayoutInflater
                .from(parent.context)
                .inflate(layoutId, parent, false)

        return when (viewType) {
            ITEM_TYPE_ROM_CARD -> CardViewHolder(view)
            ITEM_TYPE_INFO -> InfoViewHolder(view)
            ITEM_TYPE_ACTION -> ActionViewHolder(view, onItemClicked)
            else -> throw IllegalStateException("Invalid viewType ID")
        }
    }

    override fun onBindViewHolder(holder: BaseViewHolder, position: Int) {
        holder.display(items[position])
    }

    override fun getItemCount(): Int {
        return items.size
    }

    class DividerItemDecoration(context: Context) : RecyclerView.ItemDecoration() {
        private val divider: Drawable?

        init {
            val styledAttributes = context.obtainStyledAttributes(ATTRS)
            divider = styledAttributes.getDrawable(0)
            styledAttributes.recycle()
        }

        override fun onDraw(c: Canvas, parent: RecyclerView, state: RecyclerView.State?) {
            val left = parent.paddingLeft
            val right = parent.width - parent.paddingRight

            val childCount = parent.childCount
            for (i in 0 until childCount) {
                val child = parent.getChildAt(i)

                val params = child.layoutParams as RecyclerView.LayoutParams

                val top = child.bottom + params.bottomMargin
                val bottom = top + divider!!.intrinsicHeight

                divider.setBounds(left, top, right, bottom)
                divider.draw(c)
            }
        }

        companion object {
            private val ATTRS = intArrayOf(android.R.attr.listDivider)
        }
    }

    companion object {
        const val ITEM_TYPE_ROM_CARD = 1
        const val ITEM_TYPE_INFO = 2
        const val ITEM_TYPE_ACTION = 3
    }
}