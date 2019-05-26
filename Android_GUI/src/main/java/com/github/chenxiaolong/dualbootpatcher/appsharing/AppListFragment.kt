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

package com.github.chenxiaolong.dualbootpatcher.appsharing

import android.content.Context
import android.content.SharedPreferences
import android.content.pm.ApplicationInfo
import android.content.pm.PackageManager
import android.graphics.drawable.Drawable
import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ProgressBar
import android.widget.Toast
import androidx.appcompat.widget.SearchView
import androidx.appcompat.widget.SearchView.OnQueryTextListener
import androidx.core.content.edit
import androidx.fragment.app.Fragment
import androidx.loader.app.LoaderManager
import androidx.loader.app.LoaderManager.LoaderCallbacks
import androidx.loader.content.AsyncTaskLoader
import androidx.loader.content.Loader
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomConfig
import com.github.chenxiaolong.dualbootpatcher.RomConfig.SharedItems
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppCardAdapter.AppCardActionListener
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppListFragment.LoaderResult
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppSharingChangeSharedDialog.AppSharingChangeSharedDialogListener
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog.FirstUseDialogListener
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import java.text.Collator
import java.util.*

class AppListFragment : Fragment(), FirstUseDialogListener, AppCardActionListener,
        AppSharingChangeSharedDialogListener, LoaderCallbacks<LoaderResult>, OnQueryTextListener {
    private lateinit var prefs: SharedPreferences

    private lateinit var adapter: AppCardAdapter
    private lateinit var appsList: RecyclerView
    private lateinit var progressBar: ProgressBar

    private var appInfos = ArrayList<AppInformation>()
    private var config: RomConfig? = null

    private var searchQuery: String? = null

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        setHasOptionsMenu(true)

        progressBar = activity!!.findViewById(R.id.loading)

        adapter = AppCardAdapter(appInfos, this)

        appsList = activity!!.findViewById(R.id.apps)
        appsList.setHasFixedSize(true)
        appsList.adapter = adapter

        val llm = LinearLayoutManager(activity)
        llm.orientation = RecyclerView.VERTICAL
        appsList.layoutManager = llm

        showAppList(false)

        prefs = activity!!.getSharedPreferences("settings", 0)

        if (savedInstanceState == null) {
            val shouldShow = prefs.getBoolean(PREF_SHOW_FIRST_USE_DIALOG, true)
            if (shouldShow) {
                val d = FirstUseDialog.newInstance(
                        this, /* R.string.indiv_app_sharing_intro_dialog_title */0,
                        R.string.indiv_app_sharing_intro_dialog_desc)
                d.show(fragmentManager!!, CONFIRM_DIALOG_FIRST_USE)
            }
        } else {
            searchQuery = savedInstanceState.getString(EXTRA_SEARCH_QUERY)
        }

        LoaderManager.getInstance(this).initLoader(0, null, this)
    }

    override fun onCreateOptionsMenu(menu: Menu, inflater: MenuInflater) {
        inflater.inflate(R.menu.actionbar_app_list, menu)

        val item = menu.findItem(R.id.action_search)
        val searchView = item.actionView as SearchView
        searchView.setOnQueryTextListener(this)

        if (searchQuery != null) {
            searchView.isIconified = false
            //item.expandActionView();
            searchView.setQuery(searchQuery, false)
            searchView.clearFocus()
        }
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_app_list, container, false)
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        if (searchQuery != null && !searchQuery!!.isEmpty()) {
            outState.putString(EXTRA_SEARCH_QUERY, searchQuery)
        }
    }

    override fun onDestroy() {
        super.onDestroy()

        if (config == null) {
            // Destroyed before onLoadFinished ran
            return
        }

        val sharedPkgs = HashMap<String, SharedItems>()

        appInfos
                .filter {
                    // Don't spam the config with useless entries
                    it.shareData
                }
                .forEach { sharedPkgs[it.pkg] = SharedItems(it.shareData) }

        config!!.indivAppSharingPackages = sharedPkgs
        config!!.apply()

        Toast.makeText(activity, R.string.indiv_app_sharing_reboot_needed_message,
                Toast.LENGTH_LONG).show()
    }

    private fun filter(infos: List<AppInformation>?, query: String): List<AppInformation> {
        val lowerQuery = query.toLowerCase()

        return infos!!.filter { it.name.toLowerCase().contains(lowerQuery) }
    }

    override fun onQueryTextSubmit(query: String): Boolean {
        return false
    }

    override fun onQueryTextChange(newText: String): Boolean {
        searchQuery = newText

        val filteredInfoList = filter(appInfos, newText)
        adapter.setTo(filteredInfoList)
        appsList.scrollToPosition(0)
        return true
    }

    override fun onConfirmFirstUse(dontShowAgain: Boolean) {
        prefs.edit {
            putBoolean(PREF_SHOW_FIRST_USE_DIALOG, !dontShowAgain)
        }
    }

    private fun showAppList(show: Boolean) {
        progressBar.visibility = if (show) View.GONE else View.VISIBLE
        appsList.visibility = if (show) View.VISIBLE else View.GONE
    }

    override fun onCreateLoader(id: Int, args: Bundle?): Loader<LoaderResult> {
        return AppsLoader(activity!!)
    }

    override fun onLoadFinished(loader: Loader<LoaderResult>, result: LoaderResult?) {
        appInfos.clear()

        if (result == null) {
            activity!!.finish()
            return
        }

        appInfos.addAll(result.appInfos)

        config = result.config

        // Reapply filter if needed
        if (searchQuery != null) {
            val filteredInfoList = filter(appInfos, searchQuery!!)
            adapter.setTo(filteredInfoList)
        } else {
            adapter.setTo(appInfos)
        }

        showAppList(true)
    }

    override fun onLoaderReset(loader: Loader<LoaderResult>) {}

    override fun onSelectedApp(info: AppInformation) {
        val d = AppSharingChangeSharedDialog.newInstance(
                this, info.pkg, info.name, info.shareData, info.isSystem,
                info.romsThatShareData)
        d.show(fragmentManager!!, CONFIRM_DIALOG_A_S_SETTINGS)
    }

    override fun onChangedShared(pkg: String, shareData: Boolean) {
        val appInfo: AppInformation = appInfos.firstOrNull { it.pkg == pkg } ?:
                throw IllegalStateException("Sharing state was changed for package $pkg, " +
                        "which is not in the apps list")

        if (appInfo.shareData != shareData) {
            Log.d(TAG, "Data sharing set to $shareData for package $pkg")
            appInfo.shareData = shareData
        }

        adapter.setTo(appInfos)
    }

    data class AppInformation(
            val pkg: String,
            val name: String,
            val icon: Drawable,
            val isSystem: Boolean,
            var shareData: Boolean,
            val romsThatShareData: ArrayList<String>
    )

    class LoaderResult(
            internal var config: RomConfig,
            internal var appInfos: ArrayList<AppInformation>
    )

    private class AppsLoader(context: Context) : AsyncTaskLoader<LoaderResult>(context) {
        private var result: LoaderResult? = null

        init {
            onContentChanged()
        }

        override fun onStartLoading() {
            if (result != null) {
                deliverResult(result)
            } else if (takeContentChanged()) {
                forceLoad()
            }
        }

        override fun loadInBackground(): LoaderResult? {
            val start = System.currentTimeMillis()

            var info: RomInformation? = null
            var roms: Array<RomInformation> = emptyArray()

            try {
                MbtoolConnection(context).use {
                    val iface = it.`interface`!!

                    info = RomUtils.getCurrentRom(context, iface)
                    roms = RomUtils.getRoms(context, iface)
                }
            } catch (e: Exception) {
                return null
            }

            if (info == null) {
                return null
            }

            // Get shared apps from the config file
            val config = RomConfig.getConfig(info!!.configPath!!)

            if (!config.isIndivAppSharingEnabled) {
                throw IllegalStateException("Tried to open AppListFragment when " +
                        "individual app sharing is disabled")
            }

            val sharedPkgs = config.indivAppSharingPackages
            val sharedPkgsMap = getSharedPkgsForOtherRoms(roms, info)

            val pm = context.packageManager
            val apps = pm.getInstalledApplications(0)

            val appInfos = ArrayList<AppInformation>()

            val numThreads = Runtime.getRuntime().availableProcessors()
            var partitionSize = apps.size / numThreads
            if (apps.size % numThreads != 0) {
                partitionSize++
            }

            Log.d(TAG, "Loading apps with $numThreads threads")
            Log.d(TAG, "Total apps: ${apps.size}")

            val threads = ArrayList<LoaderThread>()
            var i = 0
            while (i < apps.size) {
                val begin = i
                val end = i + Math.min(partitionSize, apps.size - i)

                val thread = LoaderThread(
                        pm, apps, sharedPkgs, sharedPkgsMap, appInfos, begin, end)
                thread.start()
                threads.add(thread)

                Log.d(TAG, "Loading partition [$begin, $end) on thread ${threads.size}")
                i += partitionSize
            }

            for (thread in threads) {
                try {
                    thread.join()
                } catch (e: InterruptedException) {
                    Log.e(TAG, "Thread was interrupted", e)
                }
            }

            Collections.sort(appInfos, AppInformationComparator())

            result = LoaderResult(config, appInfos)

            val stop = System.currentTimeMillis()
            Log.d(TAG, "Retrieving apps took: ${stop - start}ms")

            return result
        }

        private fun getSharedPkgsForOtherRoms(roms: Array<RomInformation>,
                                              currentRom: RomInformation?):
                HashMap<String, HashMap<String, SharedItems>> {
            val sharedPkgsMap = HashMap<String, HashMap<String, SharedItems>>()

            for (rom in roms) {
                if (rom.id == currentRom!!.id) {
                    continue
                }

                val config = RomConfig.getConfig(rom.configPath!!)
                sharedPkgsMap[rom.id!!] = config.indivAppSharingPackages
            }

            return sharedPkgsMap
        }
    }

    private class LoaderThread(
            private val pm: PackageManager,
            private val apps: List<ApplicationInfo>,
            private val sharedPkgs: Map<String, SharedItems>,
            private val sharedPkgsMap: Map<String, HashMap<String, SharedItems>>,
            private val appInfos: MutableList<AppInformation>,
            private val start: Int,
            private val stop: Int
    ) : Thread() {
        override fun run() {
            for (i in start until stop) {
                val app = apps[i]

                if (pm.getLaunchIntentForPackage(app.packageName) == null) {
                    continue
                }

                val isSystem = app.flags and ApplicationInfo.FLAG_SYSTEM != 0
                        || app.flags and ApplicationInfo.FLAG_UPDATED_SYSTEM_APP != 0
                var sharedItems = sharedPkgs[app.packageName]
                val shareData = sharedItems?.sharedData ?: false
                val romsThatShareData = ArrayList<String>()

                // Get list of other ROMs that have this package shared
                for ((key, value) in sharedPkgsMap) {
                    sharedItems = value[app.packageName]
                    if (sharedItems?.sharedData == true) {
                        romsThatShareData.add(key)
                    }
                }

                val appInfo = AppInformation(app.packageName, app.loadLabel(pm).toString(),
                        app.loadIcon(pm), isSystem, shareData, romsThatShareData)

                synchronized(appInfos) {
                    appInfos.add(appInfo)
                }
            }
        }
    }

    private class AppInformationComparator : Comparator<AppInformation> {
        private val collator = Collator.getInstance()

        override fun compare(appInfo1: AppInformation, appInfo2: AppInformation): Int {
            return collator.compare(appInfo1.name, appInfo2.name)
        }
    }

    companion object {
        private val TAG = AppListFragment::class.java.simpleName

        private const val EXTRA_SEARCH_QUERY = "search_query"

        private const val PREF_SHOW_FIRST_USE_DIALOG = "indiv_app_sync_first_use_show_dialog"

        private val CONFIRM_DIALOG_FIRST_USE =
                "${AppListFragment::class.java.name}.confirm.first_use"
        private val CONFIRM_DIALOG_A_S_SETTINGS =
                "${AppListFragment::class.java.name}.confirm.a_s_settings"
    }
}
