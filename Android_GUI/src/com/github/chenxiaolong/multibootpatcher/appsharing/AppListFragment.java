/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.multibootpatcher.appsharing;

import android.app.Fragment;
import android.app.LoaderManager.LoaderCallbacks;
import android.content.AsyncTaskLoader;
import android.content.Context;
import android.content.Loader;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.FirstUseDialog;
import com.github.chenxiaolong.dualbootpatcher.switcher.FirstUseDialog.FirstUseDialogListener;
import com.github.chenxiaolong.multibootpatcher.RomConfig;
import com.github.chenxiaolong.multibootpatcher.RomConfig.SharedItems;
import com.github.chenxiaolong.multibootpatcher.appsharing.AppCardAdapter.AppCardActionListener;
import com.github.chenxiaolong.multibootpatcher.appsharing.AppListFragment.LoaderResult;
import com.github.chenxiaolong.multibootpatcher.appsharing.AppSharingChangeSharedDialog
        .AppSharingChangeSharedDialogListener;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

public class AppListFragment extends Fragment implements
        FirstUseDialogListener, AppCardActionListener, AppSharingChangeSharedDialogListener,
        LoaderCallbacks<LoaderResult> {
    private static final String TAG = AppListFragment.class.getSimpleName();

    private static final String PREF_SHOW_FIRST_USE_DIALOG = "indiv_app_sync_first_use_show_dialog";

    private SharedPreferences mPrefs;

    private AppCardAdapter mAdapter;
    private RecyclerView mAppsList;
    private ProgressBar mProgressBar;

    private ArrayList<AppInformation> mAppInfos;
    private RomConfig mConfig;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mProgressBar = (ProgressBar) getActivity().findViewById(R.id.loading);

        mAppInfos = new ArrayList<>();
        mAdapter = new AppCardAdapter(mAppInfos, this);

        mAppsList = (RecyclerView) getActivity().findViewById(R.id.apps);
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
                d.show(getFragmentManager(), FirstUseDialog.TAG);
            }
        }

        getActivity().getLoaderManager().initLoader(0, null, this);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_app_list, container, false);
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
            if (info.shareApk || info.shareData) {
                sharedPkgs.put(info.pkg, new SharedItems(info.shareApk, info.shareData));
            }
        }

        mConfig.setIndivAppSharingPackages(sharedPkgs);
        mConfig.apply();
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

        mAdapter.notifyDataSetChanged();

        showAppList(true);
    }

    @Override
    public void onLoaderReset(Loader<LoaderResult> loader) {
    }

    @Override
    public void onSelectedApp(AppInformation info) {
        AppSharingChangeSharedDialog d = AppSharingChangeSharedDialog.newInstance(
                this, info.pkg, info.name, info.shareApk, info.shareData, info.isSystem);
        d.show(getFragmentManager(), AppSharingChangeSharedDialog.TAG);
    }

    @Override
    public void onChangedShared(String pkg, boolean shareApk, boolean shareData) {
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

        if (appInfo.shareApk != shareApk) {
            Log.d(TAG, "APK sharing set to " + shareApk + " for package " + pkg);
            appInfo.shareApk = shareApk;
        }
        if (appInfo.shareData != shareData) {
            Log.d(TAG, "Data sharing set to " + shareData + " for package " + pkg);
            appInfo.shareData = shareData;
        }

        mAdapter.notifyDataSetChanged();
    }

    public static class AppInformation {
        public String pkg;
        public String name;
        public Drawable icon;
        public boolean isSystem;
        public boolean shareApk;
        public boolean shareData;
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

            // Get shared apps from the config file
            RomInformation info = RomUtils.getCurrentRom(getContext());
            if (info == null) {
                return null;
            }

            RomConfig config = RomConfig.getConfig(info.getConfigPath());

            if (!config.isIndivAppSharingEnabled()) {
                throw new IllegalStateException("Tried to open AppListFragment when " +
                        "individual app sharing is disabled");
            }

            HashMap<String, SharedItems> sharedPkgs = config.getIndivAppSharingPackages();

            PackageManager pm = getContext().getPackageManager();
            List<ApplicationInfo> apps = pm.getInstalledApplications(0);

            Collections.sort(apps, new ApplicationInfo.DisplayNameComparator(pm));

            ArrayList<AppInformation> appInfos = new ArrayList<>();

            for (ApplicationInfo app : apps) {
                if (pm.getLaunchIntentForPackage(app.packageName) == null) {
                    continue;
                }

                AppInformation appInfo = new AppInformation();
                appInfo.pkg = app.packageName;
                appInfo.name = app.loadLabel(pm).toString();
                appInfo.icon = app.loadIcon(pm);
                appInfo.isSystem = (app.flags & ApplicationInfo.FLAG_SYSTEM) != 0
                        || (app.flags & ApplicationInfo.FLAG_UPDATED_SYSTEM_APP) != 0;

                SharedItems sharedItems = sharedPkgs.get(appInfo.pkg);
                if (sharedItems != null) {
                    appInfo.shareApk = sharedItems.sharedApk;
                    appInfo.shareData = sharedItems.sharedData;
                }

                // Make sure we're not sharing the apk if it's a system app or an update to a system
                // app. Of course, mbtool will skip it anyway, but we don't want to confuse the user
                if (appInfo.isSystem) {
                    appInfo.shareApk = false;
                }

                appInfos.add(appInfo);
            }

            mResult = new LoaderResult();
            mResult.appInfos = appInfos;
            mResult.config = config;

            stop = System.currentTimeMillis();
            Log.d(TAG, "Retrieving apps took: " + (stop - start) + "ms");

            return mResult;
        }
    }
}
