/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.appsharing;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.LoaderManager.LoaderCallbacks;
import android.support.v4.content.AsyncTaskLoader;
import android.support.v4.content.Loader;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.SearchView;
import android.support.v7.widget.SearchView.OnQueryTextListener;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomConfig;
import com.github.chenxiaolong.dualbootpatcher.RomConfig.SharedItems;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppCardAdapter.AppCardActionListener;
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppListFragment.LoaderResult;
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppSharingChangeSharedDialog
        .AppSharingChangeSharedDialogListener;
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog.FirstUseDialogListener;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;

import org.apache.commons.io.IOUtils;

import java.text.Collator;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class AppListFragment extends Fragment implements
        FirstUseDialogListener, AppCardActionListener, AppSharingChangeSharedDialogListener,
        LoaderCallbacks<LoaderResult>, OnQueryTextListener {
    private static final String TAG = AppListFragment.class.getSimpleName();

    private static final String EXTRA_SEARCH_QUERY = "search_query";

    private static final String PREF_SHOW_FIRST_USE_DIALOG = "indiv_app_sync_first_use_show_dialog";

    private static final String CONFIRM_DIALOG_FIRST_USE =
            AppListFragment.class.getCanonicalName() + ".confirm.first_use";
    private static final String CONFIRM_DIALOG_A_S_SETTINGS =
            AppListFragment.class.getCanonicalName() + ".confirm.a_s_settings";

    private SharedPreferences mPrefs;

    private AppCardAdapter mAdapter;
    private RecyclerView mAppsList;
    private ProgressBar mProgressBar;

    private ArrayList<AppInformation> mAppInfos;
    private RomConfig mConfig;

    private String mSearchQuery;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        setHasOptionsMenu(true);

        mProgressBar = getActivity().findViewById(R.id.loading);

        mAppInfos = new ArrayList<>();
        mAdapter = new AppCardAdapter(mAppInfos, this);

        mAppsList = getActivity().findViewById(R.id.apps);
        mAppsList.setHasFixedSize(true);
        mAppsList.setAdapter(mAdapter);

        LinearLayoutManager llm = new LinearLayoutManager(getActivity());
        llm.setOrientation(LinearLayoutManager.VERTICAL);
        mAppsList.setLayoutManager(llm);

        showAppList(false);

        mPrefs = getActivity().getSharedPreferences("settings", 0);

        if (savedInstanceState == null) {
            boolean shouldShow = mPrefs.getBoolean(PREF_SHOW_FIRST_USE_DIALOG, true);
            if (shouldShow) {
                FirstUseDialog d = FirstUseDialog.newInstance(
                        this, /*R.string.indiv_app_sharing_intro_dialog_title*/0,
                        R.string.indiv_app_sharing_intro_dialog_desc);
                d.show(getFragmentManager(), CONFIRM_DIALOG_FIRST_USE);
            }
        } else {
            mSearchQuery = savedInstanceState.getString(EXTRA_SEARCH_QUERY);
        }

        getLoaderManager().initLoader(0, null, this);
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        inflater.inflate(R.menu.actionbar_app_list, menu);

        final MenuItem item = menu.findItem(R.id.action_search);
        final SearchView searchView = (SearchView) item.getActionView();
        searchView.setOnQueryTextListener(this);

        if (mSearchQuery != null) {
            searchView.setIconified(false);
            //item.expandActionView();
            searchView.setQuery(mSearchQuery, false);
            searchView.clearFocus();
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_app_list, container, false);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        if (mSearchQuery != null && !mSearchQuery.isEmpty()) {
            outState.putString(EXTRA_SEARCH_QUERY, mSearchQuery);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();

        if (mConfig == null) {
            // Destroyed before onLoadFinished ran
            return;
        }

        HashMap<String, SharedItems> sharedPkgs = new HashMap<>();

        for (AppInformation info : mAppInfos) {
            // Don't spam the config with useless entries
            if (info.shareData) {
                sharedPkgs.put(info.pkg, new SharedItems(info.shareData));
            }
        }

        mConfig.setIndivAppSharingPackages(sharedPkgs);
        mConfig.apply();

        Toast.makeText(getActivity(), R.string.indiv_app_sharing_reboot_needed_message,
                Toast.LENGTH_LONG).show();
    }

    private List<AppInformation> filter(List<AppInformation> infos, String query) {
        query = query.toLowerCase();

        final List<AppInformation> filteredInfos = new ArrayList<>();
        for (AppInformation info : infos) {
            final String text = info.name.toLowerCase();
            if (text.contains(query)) {
                filteredInfos.add(info);
            }
        }
        return filteredInfos;
    }

    @Override
    public boolean onQueryTextSubmit(String query) {
        return false;
    }

    @Override
    public boolean onQueryTextChange(String newText) {
        mSearchQuery = newText;

        final List<AppInformation> filteredInfoList = filter(mAppInfos, newText);
        mAdapter.animateTo(filteredInfoList);
        mAppsList.scrollToPosition(0);
        return true;
    }

    @Override
    public void onConfirmFirstUse(boolean dontShowAgain) {
        Editor e = mPrefs.edit();
        e.putBoolean(PREF_SHOW_FIRST_USE_DIALOG, !dontShowAgain);
        e.apply();
    }

    private void showAppList(boolean show) {
        mProgressBar.setVisibility(show ? View.GONE : View.VISIBLE);
        mAppsList.setVisibility(show ? View.VISIBLE : View.GONE);
    }

    @Override
    public Loader<LoaderResult> onCreateLoader(int id, Bundle args) {
        return new AppsLoader(getActivity());
    }

    @Override
    public void onLoadFinished(Loader<LoaderResult> loader, LoaderResult result) {
        mAppInfos.clear();

        if (result == null) {
            getActivity().finish();
            return;
        }

        if (result.appInfos != null) {
            mAppInfos.addAll(result.appInfos);
        }

        mConfig = result.config;

        // Reapply filter if needed
        if (mSearchQuery != null) {
            final List<AppInformation> filteredInfoList = filter(mAppInfos, mSearchQuery);
            mAdapter.setTo(filteredInfoList);
        } else {
            mAdapter.setTo(mAppInfos);
        }

        showAppList(true);
    }

    @Override
    public void onLoaderReset(Loader<LoaderResult> loader) {
    }

    @Override
    public void onSelectedApp(AppInformation info) {
        AppSharingChangeSharedDialog d = AppSharingChangeSharedDialog.newInstance(
                this, info.pkg, info.name, info.shareData, info.isSystem,
                info.romsThatShareData);
        d.show(getFragmentManager(), CONFIRM_DIALOG_A_S_SETTINGS);
    }

    @Override
    public void onChangedShared(String pkg, boolean shareData) {
        AppInformation appInfo = null;
        for (AppInformation ai : mAppInfos) {
            if (ai.pkg.equals(pkg)) {
                appInfo = ai;
            }
        }

        if (appInfo == null) {
            throw new IllegalStateException("Sharing state was changed for package " + pkg + ", " +
                    "which is not in the apps list");
        }

        if (appInfo.shareData != shareData) {
            Log.d(TAG, "Data sharing set to " + shareData + " for package " + pkg);
            appInfo.shareData = shareData;
        }

        mAdapter.animateTo(mAppInfos);
    }

    public static class AppInformation {
        public String pkg;
        public String name;
        public Drawable icon;
        public boolean isSystem;
        public boolean shareData;
        public ArrayList<String> romsThatShareData = new ArrayList<>();
    }

    protected static class LoaderResult {
        RomConfig config;
        ArrayList<AppInformation> appInfos;
    }

    private static class AppsLoader extends AsyncTaskLoader<LoaderResult> {
        private LoaderResult mResult;

        public AppsLoader(Context context) {
            super(context);
            onContentChanged();
        }

        @Override
        protected void onStartLoading() {
            if (mResult != null) {
                deliverResult(mResult);
            } else if (takeContentChanged()) {
                forceLoad();
            }
        }

        @Override
        public LoaderResult loadInBackground() {
            long start = System.currentTimeMillis(), stop;

            RomInformation info;
            RomInformation[] roms;

            MbtoolConnection conn = null;
            try {
                conn = new MbtoolConnection(getContext());
                MbtoolInterface iface = conn.getInterface();

                info = RomUtils.getCurrentRom(getContext(), iface);
                roms = RomUtils.getRoms(getContext(), iface);
            } catch (Exception e) {
                return null;
            } finally {
                IOUtils.closeQuietly(conn);
            }

            if (info == null) {
                return null;
            }

            // Get shared apps from the config file
            RomConfig config = RomConfig.getConfig(info.getConfigPath());

            if (!config.isIndivAppSharingEnabled()) {
                throw new IllegalStateException("Tried to open AppListFragment when " +
                        "individual app sharing is disabled");
            }

            HashMap<String, SharedItems> sharedPkgs = config.getIndivAppSharingPackages();
            HashMap<String, HashMap<String, SharedItems>> sharedPkgsMap =
                    getSharedPkgsForOtherRoms(roms, info);

            PackageManager pm = getContext().getPackageManager();
            List<ApplicationInfo> apps = pm.getInstalledApplications(0);

            ArrayList<AppInformation> appInfos = new ArrayList<>();

            int numThreads = Runtime.getRuntime().availableProcessors();
            int partitionSize = apps.size() / numThreads;
            if (apps.size() % numThreads != 0) {
                partitionSize++;
            }

            Log.d(TAG, "Loading apps with " + numThreads + " threads");
            Log.d(TAG, "Total apps: " + apps.size());

            ArrayList<LoaderThread> threads = new ArrayList<>();
            for (int i = 0; i < apps.size(); i += partitionSize) {
                int begin = i;
                int end = i + Math.min(partitionSize, apps.size() - i);

                LoaderThread thread = new LoaderThread(
                        pm, apps, sharedPkgs, sharedPkgsMap, appInfos, begin, end);
                thread.start();
                threads.add(thread);

                Log.d(TAG, "Loading partition [" + begin + ", " + end + ") on thread " +
                        threads.size());
            }

            for (LoaderThread thread : threads) {
                try {
                    thread.join();
                } catch (InterruptedException e) {
                    Log.e(TAG, "Thread was interrupted", e);
                }
            }

            Collections.sort(appInfos, new AppInformationComparator());

            mResult = new LoaderResult();
            mResult.appInfos = appInfos;
            mResult.config = config;

            stop = System.currentTimeMillis();
            Log.d(TAG, "Retrieving apps took: " + (stop - start) + "ms");

            return mResult;
        }

        private HashMap<String, HashMap<String, SharedItems>> getSharedPkgsForOtherRoms(
                RomInformation[] roms, RomInformation currentRom) {
            HashMap<String, HashMap<String, SharedItems>> sharedPkgsMap = new HashMap<>();

            for (RomInformation rom : roms) {
                if (rom.getId().equals(currentRom.getId())) {
                    continue;
                }

                RomConfig config = RomConfig.getConfig(rom.getConfigPath());
                sharedPkgsMap.put(rom.getId(), config.getIndivAppSharingPackages());
            }

            return sharedPkgsMap;
        }
    }

    private static class LoaderThread extends Thread {
        private final PackageManager mPM;
        private final List<ApplicationInfo> mApps;
        private final Map<String, SharedItems> mSharedPkgs;
        private final Map<String, HashMap<String, SharedItems>> mSharedPkgsMap;
        private final List<AppInformation> mAppInfos;
        private final int mStart;
        private final int mStop;

        public LoaderThread(PackageManager pm, List<ApplicationInfo> apps,
                            Map<String, SharedItems> sharedPkgs,
                            Map<String, HashMap<String, SharedItems>> sharedPkgsMap,
                            List<AppInformation> appInfos, int start, int stop) {
            mPM = pm;
            mApps = apps;
            mSharedPkgs = sharedPkgs;
            mSharedPkgsMap = sharedPkgsMap;
            mAppInfos = appInfos;
            mStart = start;
            mStop = stop;
        }

        @Override
        public void run() {
            for (int i = mStart; i < mStop; i++) {
                ApplicationInfo app = mApps.get(i);

                if (mPM.getLaunchIntentForPackage(app.packageName) == null) {
                    continue;
                }

                AppInformation appInfo = new AppInformation();
                appInfo.pkg = app.packageName;
                appInfo.name = app.loadLabel(mPM).toString();
                appInfo.icon = app.loadIcon(mPM);
                appInfo.isSystem = (app.flags & ApplicationInfo.FLAG_SYSTEM) != 0
                        || (app.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0;

                SharedItems sharedItems = mSharedPkgs.get(appInfo.pkg);
                if (sharedItems != null) {
                    appInfo.shareData = sharedItems.sharedData;
                }

                // Get list of other ROMs that have this package shared
                for (Map.Entry<String, HashMap<String, SharedItems>> entry :
                        mSharedPkgsMap.entrySet()) {
                    sharedItems = entry.getValue().get(appInfo.pkg);
                    if (sharedItems != null) {
                        if (sharedItems.sharedData) {
                            appInfo.romsThatShareData.add(entry.getKey());
                        }
                    }
                }

                synchronized (mAppInfos) {
                    mAppInfos.add(appInfo);
                }
            }
        }
    }

    private static class AppInformationComparator implements Comparator<AppInformation> {
        private final Collator sCollator = Collator.getInstance();

        @Override
        public int compare(AppInformation appInfo1, AppInformation appInfo2) {
            return sCollator.compare(appInfo1.name, appInfo2.name);
        }
    }
}
