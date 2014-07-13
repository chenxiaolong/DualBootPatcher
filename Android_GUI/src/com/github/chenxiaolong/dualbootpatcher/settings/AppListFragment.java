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

import android.app.AlertDialog;
import android.app.AlertDialog.Builder;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ProgressBar;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.SyncDaemonUtils;
import com.github.chenxiaolong.dualbootpatcher.settings.AppListLoaderFragment.AppInformation;
import com.github.chenxiaolong.dualbootpatcher.settings.AppListLoaderFragment.LoaderListener;
import com.github.chenxiaolong.dualbootpatcher.settings.AppListLoaderFragment.RomInfoResult;
import com.nhaarman.listviewanimations.swinginadapters.AnimationAdapter;
import com.nhaarman.listviewanimations.swinginadapters.prepared.AlphaInAnimationAdapter;

import java.util.ArrayList;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.CardArrayAdapter;
import it.gmariotti.cardslib.library.view.CardListView;

public class AppListFragment extends Fragment implements LoaderListener, OnDismissListener {
    private AppListLoaderFragment mLoaderFragment;

    private SharedPreferences mPrefs;
    private AlertDialog mDialog;

    private ProgressBar mProgressBar;
    private CardListView mAppsList;
    private ArrayList<AppInformation> mAppInfos;
    private ConfigFile mConfig;
    private RomInfoResult mRomInfos;

    private boolean mLoadedApps;
    private boolean mLoadedRoms;

    private ArrayList<Card> mCards;
    private CardArrayAdapter mCardArrayAdapter;

    private Bundle mSavedInstanceState;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mSavedInstanceState = savedInstanceState;
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
    public void onStop() {
        super.onStop();

        if (mDialog != null) {
            mDialog.dismiss();
            mDialog = null;
        }
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

        mCards = new ArrayList<Card>();
        mCardArrayAdapter = new CardArrayAdapter(getActivity(), mCards);
        mAppsList = (CardListView) getActivity().findViewById(R.id.apps);
        if (mAppsList != null) {
            AnimationAdapter animArrayAdapter = new AlphaInAnimationAdapter(mCardArrayAdapter);
            animArrayAdapter.setAbsListView(mAppsList);
            mAppsList.setExternalAdapter(animArrayAdapter, mCardArrayAdapter);
        }

        mPrefs = getActivity().getSharedPreferences("settings", 0);

        boolean dialogWasOpen = savedInstanceState != null
                && savedInstanceState.getBoolean("haveDialog");
        boolean showedDialog = savedInstanceState != null;
        boolean shouldShow = mPrefs.getBoolean("indiv_app_sync_show_dialog", true);
        if ((shouldShow && !showedDialog) || dialogWasOpen) {
            buildFirstTimeDialog();
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.app_list, container, false);
        mProgressBar = (ProgressBar) view.findViewById(R.id.loading);
        mAppsList = (CardListView) view.findViewById(R.id.apps);
        return view;
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        if (mDialog != null) {
            outState.putBoolean("haveDialog", true);
        }

        if (mCards.size() > 0) {
            boolean[] expanded = new boolean[mCards.size()];
            for (int i = 0; i < mCards.size(); i++) {
                expanded[i] = mCards.get(i).isExpanded();
            }
            outState.putBooleanArray("cardsExpanded", expanded);
        }
    }

    private void buildFirstTimeDialog() {
        if (mDialog == null) {
            View checkboxView = View.inflate(getActivity(), R.layout.dialog_checkbox_layout, null);
            CheckBox checkbox = (CheckBox) checkboxView.findViewById(R.id.checkbox);
            checkbox.setText(R.string.do_not_show_again);
            checkbox.setOnCheckedChangeListener(new OnCheckedChangeListener() {
                @Override
                public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                    Editor e = mPrefs.edit();
                    e.putBoolean("indiv_app_sync_show_dialog", !isChecked);
                    e.apply();
                }
            });

            AlertDialog.Builder builder = new Builder(getActivity());
            builder.setTitle(R.string.indiv_app_sharing_dialog_title);
            builder.setMessage(R.string.indiv_app_sharing_dialog_desc);
            builder.setView(checkboxView);
            builder.setPositiveButton(R.string.proceed, null);
            mDialog = builder.show();
            mDialog.setOnDismissListener(this);
        }
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

        mLoadedApps = true;
        ready();
    }

    @Override
    public void obtainedRomInfo(RomInfoResult result) {
        mRomInfos = result;

        mLoadedRoms = true;
        ready();
    }

    @Override
    public void onDismiss(DialogInterface dialogInterface) {
        if (mDialog == dialogInterface) {
            mDialog = null;
        }
    }

    private void ready() {
        if (mLoadedApps && mLoadedRoms) {
            for (AppInformation appInfo : mAppInfos) {
                AppCard card = new AppCard(getActivity(), mConfig, appInfo, mRomInfos);
                mCards.add(card);
            }

            if (mSavedInstanceState != null) {
                boolean[] expanded = mSavedInstanceState.getBooleanArray("cardsExpanded");
                if (expanded != null && expanded.length == mCards.size()) {
                    for (int i = 0; i < expanded.length; i++) {
                        mCards.get(i).setExpanded(expanded[i]);
                    }
                }
            }

            mCardArrayAdapter.notifyDataSetChanged();

            showAppList(true);
        }
    }
}
