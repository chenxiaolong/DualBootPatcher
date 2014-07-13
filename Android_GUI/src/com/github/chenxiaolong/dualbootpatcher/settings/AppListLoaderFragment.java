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
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public class AppListLoaderFragment extends Fragment {
    public static final String TAG = AppListLoaderFragment.class.getName();

    private PackageManager mPackageManager;
    private Resources mResources;
    private LoaderTask mTask;
    private ObtainRomInfoTask mRomTask;
    private LoaderListener mListener;

    private ArrayList<AppInformation> mSaveLoadedApps;
    private RomInfoResult mSaveRomInfo;

    public interface LoaderListener {
        public void loadedApps(ArrayList<AppInformation> appInfos);

        public void obtainedRomInfo(RomInfoResult result);
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
            mTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }

        if (mRomTask == null) {
            mRomTask = new ObtainRomInfoTask(getActivity().getApplicationContext());
            mRomTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        }
    }

    public synchronized void attachListenerAndResendEvents(LoaderListener listener) {
        mListener = listener;

        if (mSaveLoadedApps != null) {
            mListener.loadedApps(mSaveLoadedApps);
        }

        if (mSaveRomInfo != null) {
            mListener.obtainedRomInfo(mSaveRomInfo);
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

    private synchronized void onObtainedRomInfo(RomInfoResult result) {
        mSaveRomInfo = result;

        if (mListener != null) {
            mListener.obtainedRomInfo(result);
        }
    }

    public static class AppInformation {
        public String pkg;
        public String name;
        public Drawable icon;
        public ArrayList<RomInformation> roms = new ArrayList<RomInformation>();
    }

    private class LoaderTask extends AsyncTask<Void, Void, Void> {
        private static final int MAX_THREADS = 4;
        private ArrayList<AppInformation> mAppInfos = new ArrayList<AppInformation>();
        private HashMap<String, AppInformation> mAppInfosMap =
                new HashMap<String, AppInformation>();

        @Override
        protected Void doInBackground(Void... args) {
            HashMap<RomInformation, ArrayList<String>> apksMap = AppSharingUtils.getAllApks();

            if (apksMap == null) {
                return null;
            }

            for (Map.Entry<RomInformation, ArrayList<String>> entry : apksMap.entrySet()) {
                RomInformation rom = entry.getKey();
                ArrayList<String> filenames = entry.getValue();

                int partitionSize;
                if (filenames.size() % MAX_THREADS == 0) {
                    partitionSize = filenames.size() / MAX_THREADS;
                } else {
                    partitionSize = filenames.size() / MAX_THREADS + 1;
                }

                ArrayList<LoaderThread> threads = new ArrayList<LoaderThread>();

                for (int i = 0; i < filenames.size(); i += partitionSize) {
                    LoaderThread thread = new LoaderThread(rom, filenames, i,
                            i + Math.min(partitionSize, filenames.size() - i));
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
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            onLoadedApps(mAppInfos);
        }

        private class LoaderThread extends Thread {
            private final RomInformation mRomInfo;
            private final ArrayList<String> mFilenames;
            private final int mStart;
            private final int mStop;

            public LoaderThread(RomInformation rom, ArrayList<String> filenames,
                                int start, int stop) {
                mRomInfo = rom;
                mFilenames = filenames;
                mStart = start;
                mStop = stop;
            }

            @Override
            public void run() {
                for (int i = mStart; i < mStop; i++) {
                    String filename = mFilenames.get(i);

                    PackageInfo pi = mPackageManager.getPackageArchiveInfo(filename, 0);

                    if (pi == null) {
                        continue;
                    }

                    String pkg = pi.applicationInfo.packageName;
                    AppInformation appinfo = new AppInformation();

                    synchronized (mAppInfosMap) {
                        if (mAppInfosMap.containsKey(pkg)) {
                            mAppInfosMap.get(pkg).roms.add(mRomInfo);
                            continue;
                        }
                        mAppInfosMap.put(pkg, appinfo);
                    }

                    pi.applicationInfo.sourceDir = filename;
                    pi.applicationInfo.publicSourceDir = filename;

                    appinfo.pkg = pkg;
                    appinfo.name = (String) pi.applicationInfo.loadLabel(mPackageManager);
                    appinfo.icon = pi.applicationInfo.loadIcon(mPackageManager);
                    appinfo.roms.add(mRomInfo);

                    synchronized (mAppInfos) {
                        mAppInfos.add(appinfo);
                    }
                }
            }
        }
    }

    public class RomInfoResult {
        RomInformation[] roms;
        String[] names;
        String[] versions;
    }

    private class ObtainRomInfoTask extends AsyncTask<Void, Void, RomInfoResult> {
        private Context mContext;

        public ObtainRomInfoTask(Context context) {
            mContext = context;
        }

        @Override
        protected RomInfoResult doInBackground(Void... params) {
            final RomInfoResult result = new RomInfoResult();
            result.roms = RomUtils.getRoms();

            ArrayList<String> names = new ArrayList<String>();
            ArrayList<String> versions = new ArrayList<String>();

            for (RomInformation rom : result.roms) {
                names.add(RomUtils.getName(mContext, rom));
                versions.add(RomUtils.getVersion(rom));
            }

            result.names = names.toArray(new String[names.size()]);
            result.versions = versions.toArray(new String[versions.size()]);

            return result;
        }

        @Override
        protected void onPostExecute(RomInfoResult result) {
            onObtainedRomInfo(result);
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
