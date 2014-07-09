/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.app.Fragment;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Bundle;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.Locale;

public class AppListLoaderFragment extends Fragment {
    public static final String TAG = AppListLoaderFragment.class.getName();

    private PackageManager mPackageManager;
    private Resources mResources;
    private LoaderTask mTask;
    private LoaderListener mListener;

    private ArrayList<AppInformation> mSaveLoadedApps;

    public interface LoaderListener {
        public void loadedApps(ArrayList<AppInformation> appInfos);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setRetainInstance(true);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (mTask == null) {
            mPackageManager = getActivity().getPackageManager();
            mResources = getResources();

            mTask = new LoaderTask();
            mTask.execute();
        }
    }

    public synchronized void attachListenerAndResendEvents(LoaderListener listener) {
        mListener = listener;

        if (mSaveLoadedApps != null) {
            mListener.loadedApps(mSaveLoadedApps);
        }
    }

    public synchronized void detachListener() {
        mListener = null;
    }

    private synchronized void onLoadedApps(ArrayList<AppInformation> appInfos) {
        mSaveLoadedApps = appInfos;

        if (mListener != null) {
            mListener.loadedApps(appInfos);
        }
    }

    public static class AppInformation {
        public String pkg;
        public String name;
        public Drawable icon;
    }

    private class LoaderTask extends AsyncTask<Void, Void, Void> {
        private static final int MAX_THREADS = 4;
        private final HashSet<String> mPackageSet = new HashSet<String>();
        private ArrayList<AppInformation> mAppInfos = new ArrayList<AppInformation>();

        @Override
        protected Void doInBackground(Void... args) {
            String[] apks = AppSharingUtils.getAllApks();

            if (apks == null) {
                return null;
            }

            int partitionSize;
            if (apks.length % MAX_THREADS == 0) {
                partitionSize = apks.length / MAX_THREADS;
            } else {
                partitionSize = apks.length / MAX_THREADS + 1;
            }

            ArrayList<LoaderThread> threads = new ArrayList<LoaderThread>();

            for (int i = 0; i < apks.length; i += partitionSize) {
                LoaderThread thread = new LoaderThread(apks, i,
                        i + Math.min(partitionSize, apks.length - i));
                thread.start();

                threads.add(thread);
            }

            try {
                for (LoaderThread thread : threads) {
                    thread.join();
                }
            } catch (InterruptedException e) {
                e.printStackTrace();
            }

            Collections.sort(mAppInfos, new AppInformationComparator());

            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            synchronized (AppListLoaderFragment.this) {
                onLoadedApps(mAppInfos);
            }
        }

        private class LoaderThread extends Thread {
            private final String[] mFilenames;
            private final int mStart;
            private final int mStop;

            public LoaderThread(String[] filenames, int start, int stop) {
                mFilenames = filenames;
                mStart = start;
                mStop = stop;
            }

            @Override
            public void run() {
                for (int i = mStart; i < mStop; i++) {
                    String filename = mFilenames[i];

                    PackageInfo pi = mPackageManager.getPackageArchiveInfo(filename, 0);

                    if (pi == null) {
                        continue;
                    }

                    String pkg = pi.applicationInfo.packageName;

                    synchronized (mPackageSet) {
                        if (mPackageSet.contains(pkg)) {
                            continue;
                        }
                        mPackageSet.add(pkg);
                    }

                    pi.applicationInfo.sourceDir = filename;
                    pi.applicationInfo.publicSourceDir = filename;

                    AppInformation appinfo = new AppInformation();
                    appinfo.pkg = pkg;
                    appinfo.name = (String) pi.applicationInfo.loadLabel(mPackageManager);
                    appinfo.icon = pi.applicationInfo.loadIcon(mPackageManager);

                    // No need to synchronize, we're only adding to the list
                    mAppInfos.add(appinfo);
                }
            }
        }
    }

    private class AppInformationComparator implements Comparator<AppInformation> {
        @Override
        public int compare(AppInformation info1, AppInformation info2) {
            Locale locale = mResources.getConfiguration().locale;
            String name1 = info1.name.toLowerCase(locale);
            String name2 = info2.name.toLowerCase(locale);
            return name1.compareTo(name2);
        }
    }
}
