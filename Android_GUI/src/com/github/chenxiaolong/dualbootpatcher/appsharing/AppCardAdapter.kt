/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.appsharing

import android.support.v7.widget.CardView
import android.support.v7.widget.RecyclerView
import android.support.v7.widget.RecyclerView.ViewHolder
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView

import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppCardAdapter.AppCardViewHolder
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppListFragment.AppInformation

import java.util.ArrayList

class AppCardAdapter(
        appsList: List<AppInformation>,
        private val listener: AppCardActionListener?
) : RecyclerView.Adapter<AppCardViewHolder>() {
    private val apps: MutableList<AppInformation>

    init {
        apps = ArrayList(appsList)
    }

    private val onAppCardClickListener = object : AppCardClickListener {
        override fun onCardClick(view: View, position: Int) {
            listener?.onSelectedApp(apps[position])
        }
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): AppCardViewHolder {
        val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.card_v7_app, parent, false)
        return AppCardViewHolder(view, onAppCardClickListener)
    }

    override fun onBindViewHolder(holder: AppCardViewHolder, position: Int) {
        val app = apps[position]
        holder.vName.text = app.name
        holder.vThumbnail.setImageDrawable(app.icon)
        holder.vPkg.text = app.pkg
        holder.vSystem.visibility = if (app.isSystem) View.VISIBLE else View.GONE

        if (app.shareData) {
            holder.vShared.setText(R.string.indiv_app_sharing_shared_data)
        } else {
            holder.vShared.setText(R.string.indiv_app_sharing_not_shared)
        }
    }

    override fun getItemCount(): Int {
        return apps.size
    }

    class AppCardViewHolder(
            v: View,
            private val listener: AppCardClickListener
    ) : ViewHolder(v) {
        private var vCard: CardView = v as CardView
        var vThumbnail: ImageView = v.findViewById(R.id.app_thumbnail)
        var vName: TextView = v.findViewById(R.id.app_name)
        var vPkg: TextView = v.findViewById(R.id.app_pkg)
        var vShared: TextView = v.findViewById(R.id.app_shared)
        var vSystem: ImageView = v.findViewById(R.id.app_system_icon)

        init {
            vCard.setOnClickListener { view -> listener.onCardClick(view, adapterPosition) }
        }
    }

    interface AppCardClickListener {
        fun onCardClick(view: View, position: Int)
    }

    interface AppCardActionListener {
        fun onSelectedApp(info: AppInformation)
    }

    fun setTo(infos: List<AppInformation>) {
        apps.clear()
        apps.addAll(infos)
        notifyDataSetChanged()
    }

    fun animateTo(infos: List<AppInformation>) {
        applyAndAnimateRemovals(infos)
        applyAndAnimateAdditions(infos)
        applyAndAnimateMovedItems(infos)
    }

    private fun applyAndAnimateRemovals(newInfos: List<AppInformation>) {
        for (i in apps.indices.reversed()) {
            val info = apps[i]
            if (!newInfos.contains(info)) {
                removeItem(i)
            }
        }
    }

    private fun applyAndAnimateAdditions(newInfos: List<AppInformation>) {
        var i = 0
        val count = newInfos.size
        while (i < count) {
            val info = newInfos[i]
            if (!apps.contains(info)) {
                addItem(i, info)
            }
            i++
        }
    }

    private fun applyAndAnimateMovedItems(newInfos: List<AppInformation>) {
        for (toPosition in newInfos.indices.reversed()) {
            val info = newInfos[toPosition]
            val fromPosition = apps.indexOf(info)
            if (fromPosition >= 0 && fromPosition != toPosition) {
                moveItem(fromPosition, toPosition)
            }
        }
    }

    fun removeItem(position: Int): AppInformation {
        val info = apps.removeAt(position)
        notifyItemRemoved(position)
        return info
    }

    fun addItem(position: Int, info: AppInformation) {
        apps.add(position, info)
        notifyItemInserted(position)
    }

    fun moveItem(fromPosition: Int, toPosition: Int) {
        val info = apps.removeAt(fromPosition)
        apps.add(toPosition, info)
        notifyItemMoved(fromPosition, toPosition)
    }
}