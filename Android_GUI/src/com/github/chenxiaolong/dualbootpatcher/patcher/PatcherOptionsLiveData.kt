/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.annotation.SuppressLint
import android.arch.lifecycle.LiveData
import android.content.Context
import android.os.AsyncTask
import java.util.*

internal class PatcherOptionsLiveData(
        private val context: Context
) : LiveData<PatcherOptionsData>() {
    init {
        loadData()
    }

    private fun loadData() {
        PatcherOptionsLoadTask().execute()
    }

    @SuppressLint("StaticFieldLeak")
    private inner class PatcherOptionsLoadTask : AsyncTask<Void, Void, PatcherOptionsData>() {
        override fun doInBackground(vararg params: Void?): PatcherOptionsData {
            val devices = PatcherUtils.getDevices(context) ?: Collections.emptyList()
            val currentDevice = PatcherUtils.getCurrentDevice(context, devices)

            // Build list of fixed install locations
            val installLocations = ArrayList<InstallLocation>()
            installLocations.addAll(PatcherUtils.installLocations)
            installLocations.addAll(PatcherUtils.getInstalledTemplateLocations())

            // Build list of template install locations
            val templateLocations = ArrayList<TemplateLocation>()
            templateLocations.addAll(PatcherUtils.templateLocations)

            return PatcherOptionsData(currentDevice, devices, installLocations, templateLocations)
        }

        override fun onPostExecute(result: PatcherOptionsData) {
            value = result
        }
    }
}