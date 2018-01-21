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

package com.github.chenxiaolong.dualbootpatcher.appsharing

import android.support.v7.util.DiffUtil
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
        val result = DiffUtil.calculateDiff(object : DiffUtil.Callback() {
            override fun areItemsTheSame(oldItemPosition: Int, newItemPosition: Int): Boolean {
                return apps[oldItemPosition].pkg == infos[newItemPosition].pkg
            }

            override fun getOldListSize(): Int {
                return apps.size
            }

            override fun getNewListSize(): Int {
                return infos.size
            }

            override fun areContentsTheSame(oldItemPosition: Int, newItemPosition: Int): Boolean {
                return apps[oldItemPosition] == infos[newItemPosition]
            }
        })

        apps.clear()
        apps.addAll(infos)
        result.dispatchUpdatesTo(this)
    }
}