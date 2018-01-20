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

import android.arch.lifecycle.Observer
import android.arch.lifecycle.ViewModelProviders
import android.content.Context
import android.graphics.Color
import android.os.Bundle
import android.support.v4.app.Fragment
import android.support.v4.widget.SwipeRefreshLayout
import android.support.v7.widget.LinearLayoutManager
import android.support.v7.widget.RecyclerView
import android.support.v7.widget.RecyclerView.ViewHolder
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView

import com.github.chenxiaolong.dualbootpatcher.FileUtils
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.views.CircularProgressBar

import java.util.ArrayList
import java.util.Random

class FreeSpaceFragment : Fragment() {
    private val mounts = ArrayList<MountInfo>()
    private lateinit var adapter: MountInfoAdapter

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        val model = ViewModelProviders.of(this).get(FreeSpaceViewModel::class.java)
        model.mounts.observe(this, Observer { mounts ->
            this.mounts.clear()
            this.mounts.addAll(mounts!!)
            adapter.notifyDataSetChanged()
        })

        adapter = MountInfoAdapter(activity!!, mounts, COLORS)

        val rv = activity!!.findViewById<RecyclerView>(R.id.mountpoints)
        rv.setHasFixedSize(true)
        rv.adapter = adapter

        val llm = LinearLayoutManager(activity)
        llm.orientation = LinearLayoutManager.VERTICAL
        rv.layoutManager = llm

        val srl = activity!!.findViewById<SwipeRefreshLayout>(R.id.swiperefresh)
        srl.setOnRefreshListener {
            model.refresh()
            srl.isRefreshing = false
        }

        srl.setColorSchemeResources(
                R.color.swipe_refresh_color1,
                R.color.swipe_refresh_color2,
                R.color.swipe_refresh_color3,
                R.color.swipe_refresh_color4
        )
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_free_space, container, false)
    }

    internal class MountInfoViewHolder(v: View) : ViewHolder(v) {
        var vMountPoint: TextView = v.findViewById(R.id.mount_point)
        var vTotalSize: TextView = v.findViewById(R.id.size_total)
        var vAvailSize: TextView = v.findViewById(R.id.size_free)
        var vProgress: CircularProgressBar = v.findViewById(R.id.mountpoint_usage)
    }

    internal class MountInfoAdapter constructor(
            private val context: Context,
            private val mounts: List<MountInfo>,
            private val colors: IntArray
    ) : RecyclerView.Adapter<MountInfoViewHolder>() {
        // Choose random color to start from (and go down the list)
        private val startingColorIndex: Int = Random().nextInt(colors.size)

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): MountInfoViewHolder {
            val view = LayoutInflater
                    .from(parent.context)
                    .inflate(R.layout.card_v7_free_space, parent, false)
            return MountInfoViewHolder(view)
        }

        override fun onBindViewHolder(holder: MountInfoViewHolder, position: Int) {
            val mount = mounts[position]

            val strTotal = FileUtils.toHumanReadableSize(context, mount.totalSpace, 2)
            val strAvail = FileUtils.toHumanReadableSize(context, mount.availSpace, 2)

            holder.vMountPoint.text = mount.mountPoint
            holder.vTotalSize.text = strTotal
            holder.vAvailSize.text = strAvail

            if (mount.totalSpace == 0L) {
                holder.vProgress.progress = 0f
            } else {
                holder.vProgress.progress = 1.0f - mount.availSpace.toFloat() / mount.totalSpace
            }

            val color = COLORS[(startingColorIndex + position) % colors.size]
            holder.vProgress.progressColor = color
        }

        override fun getItemCount(): Int {
            return mounts.size
        }
    }

    companion object {
        val FRAGMENT_TAG: String = FreeSpaceFragment::class.java.simpleName

        private val COLORS = intArrayOf(
                Color.parseColor("#F44336"),
                Color.parseColor("#8E24AA"),
                Color.parseColor("#3F51B5"),
                Color.parseColor("#2196F3"),
                Color.parseColor("#4CAF50"),
                Color.parseColor("#FBC02D"),
                Color.parseColor("#E65100"),
                Color.parseColor("#607D8B")
        )

        fun newInstance(): FreeSpaceFragment {
            return FreeSpaceFragment()
        }
    }
}
