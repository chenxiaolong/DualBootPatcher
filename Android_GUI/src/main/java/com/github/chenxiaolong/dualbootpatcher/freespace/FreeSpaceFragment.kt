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

import android.graphics.Color
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.lifecycle.Observer
import androidx.lifecycle.ViewModelProviders
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.swiperefreshlayout.widget.SwipeRefreshLayout
import com.github.chenxiaolong.dualbootpatcher.R

class FreeSpaceFragment : Fragment() {
    private lateinit var adapter: MountInfoAdapter

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        val model = ViewModelProviders.of(this)[FreeSpaceViewModel::class.java]
        model.mounts.observe(this, Observer { mounts ->
            adapter.setList(mounts!!)
        })

        adapter = MountInfoAdapter(COLORS)

        val rv = activity!!.findViewById<RecyclerView>(R.id.mountpoints)
        rv.setHasFixedSize(true)
        rv.adapter = adapter

        val llm = LinearLayoutManager(activity)
        llm.orientation = RecyclerView.VERTICAL
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
