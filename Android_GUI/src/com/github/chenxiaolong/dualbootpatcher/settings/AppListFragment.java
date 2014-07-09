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

import android.app.FragmentManager;
import android.content.Context;
import android.os.Bundle;
import android.preference.PreferenceFragment;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.SectionIndexer;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.SyncDaemonUtils;
import com.github.chenxiaolong.dualbootpatcher.settings.AppListLoaderFragment.AppInformation;
import com.github.chenxiaolong.dualbootpatcher.settings.AppListLoaderFragment.LoaderListener;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Locale;

public class AppListFragment extends PreferenceFragment implements LoaderListener {
    private AppListLoaderFragment mLoaderFragment;

    private ProgressBar mProgressBar;
    private ListView mAppsList;
    private AppAdapter mAdapter;
    private ArrayList<AppInformation> mAppInfos;
    private ConfigFile mConfig;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mConfig = new ConfigFile();

        FragmentManager fm = getFragmentManager();
        mLoaderFragment = (AppListLoaderFragment) fm
                .findFragmentByTag(AppListLoaderFragment.TAG);

        if (mLoaderFragment == null) {
            mLoaderFragment = new AppListLoaderFragment();
            fm.beginTransaction().add(mLoaderFragment, AppListLoaderFragment.TAG).commit();
        }
    }

    @Override
    public void onDestroy() {
        final Context context = AppListFragment.this.getActivity().getApplicationContext();

        super.onDestroy();
        mConfig.save();

        new Thread() {
            @Override
            public void run() {
                SyncDaemonUtils.autoSpawn(context);
            }
        }.start();
    }

    @Override
    public void onResume() {
        super.onResume();
        mLoaderFragment.attachListenerAndResendEvents(this);
    }

    @Override
    public void onPause() {
        super.onPause();
        mLoaderFragment.detachListener();
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        showAppList(false);

        mAppInfos = new ArrayList<AppInformation>();
        mAdapter = new AppAdapter(getActivity(), mAppInfos);
        mAppsList.setAdapter(mAdapter);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.app_list, container, false);
        mProgressBar = (ProgressBar) view.findViewById(R.id.loading);
        mAppsList = (ListView) view.findViewById(R.id.apps);
        return view;
    }

    private void showAppList(boolean show) {
        mProgressBar.setVisibility(show ? View.GONE : View.VISIBLE);
        mAppsList.setVisibility(show ? View.VISIBLE : View.GONE);
    }

    @Override
    public void loadedApps(ArrayList<AppInformation> appInfos) {
        if (mAppInfos.size() != appInfos.size()) {
            mAppInfos.clear();
            mAppInfos.addAll(appInfos);
        }
        mAdapter.notifyDataSetChanged();
        showAppList(true);
    }

    // From AOSP's com.android.settings
    private static class AppViewHolder {
        public TextView appName;
        public ImageView appIcon;
        public ArrayList<Button> shareButtons;
    }

    private class AppAdapter extends ArrayAdapter<AppInformation> implements SectionIndexer {
        private final Context mContext;
        private final LayoutInflater mInflater;

        private final ArrayList<AppInformation> mInfos;
        private String[] mSections;
        private Integer[] mPositions;

        public AppAdapter(Context context, ArrayList<AppInformation> infos) {
            super(context, R.layout.app_checkable_item, infos);
            mContext = context;
            mInflater = LayoutInflater.from(context);
            mInfos = infos;

            reIndexList();
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            AppViewHolder holder;

            final RomInformation[] roms = RomUtils.getRoms();

            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.app_checkable_item, parent, false);

                holder = new AppViewHolder();
                holder.appName = (TextView) convertView.findViewById(R.id.name);
                holder.appIcon = (ImageView) convertView.findViewById(R.id.icon);
                holder.shareButtons = new ArrayList<Button>();

                GridLayout btnHolder = (GridLayout) convertView.findViewById(R.id.btn_holder);

                DisplayMetrics metrics = getResources().getDisplayMetrics();
                float scaleFactor = getResources().getDimension(R.dimen.all_caps_button_width);
                btnHolder.setColumnCount((int) ((float) metrics.widthPixels / scaleFactor));

                for (int i = 0; i < roms.length; i++) {
                    Button button = (Button) mInflater.inflate(R.layout.all_caps_button,
                            btnHolder, false);

                    btnHolder.addView(button);

                    holder.shareButtons.add(button);
                }

                convertView.setTag(holder);
            } else {
                holder = (AppViewHolder) convertView.getTag();
            }

            final AppInformation appinfo = mAppInfos.get(position);
            holder.appName.setText(appinfo.name);
            holder.appIcon.setImageDrawable(appinfo.icon);

            for (int i = 0; i < roms.length; i++) {
                final RomInformation info = roms[i];
                final Button button = holder.shareButtons.get(i);

                String name = RomUtils.getName(mContext, info);

                Locale locale = getResources().getConfiguration().locale;
                button.setText(name.toUpperCase(locale));

                button.setSelected(mConfig.isRomSynced(appinfo.pkg, info));

                button.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        boolean selected = button.isSelected();
                        mConfig.setRomSynced(appinfo.pkg, info, !selected);
                        button.setSelected(!selected);
                    }
                });
            }

            return convertView;
        }

        @Override
        public int getPositionForSection(int section) {
            return mPositions[section];
        }

        @Override
        public int getSectionForPosition(int position) {
            int index = Arrays.binarySearch(mPositions, position);

            return index >= 0 ? index : -index - 2;
        }

        @Override
        public Object[] getSections() {
            return mSections;
        }

        @Override
        public void notifyDataSetChanged() {
            super.notifyDataSetChanged();

            reIndexList();
        }

        private void reIndexList() {
            String prevFirstChar = "";

            ArrayList<String> sections = new ArrayList<String>();
            ArrayList<Integer> positions = new ArrayList<Integer>();

            // List is already sorted
            for (int i = 0; i < mInfos.size(); i++) {
                String name = mInfos.get(i).name;
                String firstChar = name.substring(0, 1).toUpperCase(getResources()
                        .getConfiguration().locale);
                if (!firstChar.equals(prevFirstChar)) {
                    prevFirstChar = firstChar;

                    sections.add(firstChar);
                    positions.add(i);
                }
            }

            mSections = new String[sections.size()];
            sections.toArray(mSections);
            mPositions = new Integer[sections.size()];
            positions.toArray(mPositions);
        }
    }
}
