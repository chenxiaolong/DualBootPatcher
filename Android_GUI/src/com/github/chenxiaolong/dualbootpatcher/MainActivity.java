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

package com.github.chenxiaolong.dualbootpatcher;

import java.util.ArrayList;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.Configuration;
import android.os.Bundle;
import android.support.v4.app.ActionBarDrawerToggle;
import android.support.v4.widget.DrawerLayout;
import android.view.Gravity;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.patcher.PatchFileFragment;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherListFragment;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;

public class MainActivity extends Activity {
    private static final String PATCH_FILE = "com.github.chenxiaolong.dualbootpatcher.PATCH_FILE";

    private String[] mNavTitles;
    private String[] mNavTitles2;
    private DrawerLayout mDrawerLayout;
    private RelativeLayout mDrawerView;
    private ListView mDrawerList;
    private ListView mDrawerList2;
    private ArrayList<NavigationDrawerItem> mDrawerItems;
    private ArrayList<NavigationDrawerItem> mDrawerItems2;
    private NavigationDrawerAdapter mDrawerAdapter;
    private NavigationDrawerAdapter mDrawerAdapter2;
    private ActionBarDrawerToggle mDrawerToggle;
    private int mTitle;

    private boolean mAutomated;
    private Bundle mAutomatedData;

    private static final int NAV_CHOOSE_ROM = 0;
    private static final int NAV_SET_KERNEL = 1;
    private static final int NAV_PATCH_FILE = 2;

    private static final int NAV2_REBOOT = 0;
    private static final int NAV2_ABOUT = 1;

    public static final int FRAGMENT_CHOOSE_ROM = 1;
    public static final int FRAGMENT_SET_KERNEL = 2;
    public static final int FRAGMENT_PATCH_FILE = 3;
    public static final int FRAGMENT_ABOUT = 4;

    private int mFragment;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.drawer_layout);

        // Get the intent that started this activity
        Intent intent = getIntent();
        if (PATCH_FILE.equals(intent.getAction())) {
            mAutomated = true;
            mAutomatedData = intent.getExtras();
        }

        if (savedInstanceState != null) {
            mTitle = savedInstanceState.getInt("title");
        }

        mNavTitles = getResources().getStringArray(R.array.nav_titles);
        mNavTitles2 = getResources().getStringArray(R.array.nav_titles2);
        mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
        mDrawerToggle = new ActionBarDrawerToggle(this, mDrawerLayout,
                R.drawable.ic_drawer, R.string.drawer_open,
                R.string.drawer_close) {
            @Override
            public void onDrawerClosed(View view) {
                super.onDrawerClosed(view);
                updateTitle();
            }

            @Override
            public void onDrawerOpened(View drawerView) {
                super.onDrawerOpened(drawerView);
                updateTitle();
            }
        };

        mDrawerLayout.setDrawerListener(mDrawerToggle);
        mDrawerLayout.setDrawerShadow(R.drawable.drawer_shadow, Gravity.START);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);

        mDrawerView = (RelativeLayout) findViewById(R.id.left_drawer);
        mDrawerList = (ListView) findViewById(R.id.drawer_list);
        mDrawerList2 = (ListView) findViewById(R.id.drawer_list2);

        // Top list

        mDrawerItems = new ArrayList<NavigationDrawerItem>();

        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_CHOOSE_ROM],
                R.drawable.check));
        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_SET_KERNEL],
                R.drawable.pin));
        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_PATCH_FILE],
                R.drawable.split));

        mDrawerAdapter = new NavigationDrawerAdapter(this,
                R.layout.drawer_list_item, mDrawerItems);
        mDrawerList.setAdapter(mDrawerAdapter);

        mDrawerList.setOnItemClickListener(new DrawerItemClickListener(
                mDrawerList));

        // Bottom list

        mDrawerItems2 = new ArrayList<NavigationDrawerItem>();

        mDrawerItems2.add(new NavigationDrawerItem(mNavTitles2[NAV2_REBOOT],
                R.drawable.refresh));
        mDrawerItems2.add(new NavigationDrawerItem(mNavTitles2[NAV2_ABOUT],
                R.drawable.about));

        mDrawerAdapter2 = new NavigationDrawerAdapter(this,
                R.layout.drawer_list_item, mDrawerItems2);
        mDrawerList2.setAdapter(mDrawerAdapter2);

        mDrawerList2.setOnItemClickListener(new DrawerItemClickListener(
                mDrawerList2));

        if (mAutomated) {
            // Don't allow navigating to other parts of the app
            lockNavigation();

            mFragment = FRAGMENT_PATCH_FILE;
            showFragment();
            return;
        }

        if (savedInstanceState != null) {
            mFragment = savedInstanceState.getInt("fragment");

            showFragment();

            // Progress icons
            boolean[] progressState = savedInstanceState
                    .getBooleanArray("progressState");
            for (int i = 0; i < progressState.length; i++) {
                mDrawerItems.get(i).setProgressShowing(progressState[i]);
            }
            mDrawerAdapter.notifyDataSetChanged();

            boolean[] progressState2 = savedInstanceState
                    .getBooleanArray("progressState2");
            for (int i = 0; i < progressState2.length; i++) {
                mDrawerItems2.get(i).setProgressShowing(progressState2[i]);
            }
            mDrawerAdapter2.notifyDataSetChanged();
        } else {
            // Show about screen by default
            mDrawerList2.performItemClick(mDrawerList2, NAV2_ABOUT,
                    mDrawerList2.getItemIdAtPosition(NAV2_ABOUT));
        }

        // Open drawer on first start
        new Thread() {
            @Override
            public void run() {
                SharedPreferences sp = getSharedPreferences("settings", 0);
                boolean isFirstStart = sp.getBoolean("firstStart", true);
                if (isFirstStart) {
                    mDrawerLayout.openDrawer(mDrawerView);
                    Editor e = sp.edit();
                    e.putBoolean("firstStart", false);
                    e.commit();
                }
            }
        }.start();
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);
        savedInstanceState.putInt("fragment", mFragment);
        savedInstanceState.putInt("title", mTitle);

        // Progress icon state
        boolean[] progressState = new boolean[mDrawerItems.size()];
        for (int i = 0; i < progressState.length; i++) {
            progressState[i] = mDrawerItems.get(i).isProgressShowing();
        }
        savedInstanceState.putBooleanArray("progressState", progressState);

        boolean[] progressState2 = new boolean[mDrawerItems2.size()];
        for (int i = 0; i < progressState2.length; i++) {
            progressState2[i] = mDrawerItems2.get(i).isProgressShowing();
        }
        savedInstanceState.putBooleanArray("progressState2", progressState2);
    }

    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);
        mDrawerToggle.syncState();

        updateTitle();
    }

    @Override
    public void onBackPressed() {
        boolean operationsRunning = false;

        for (int i = 0; i < mDrawerItems.size(); i++) {
            if (mDrawerItems.get(i).isProgressShowing()) {
                operationsRunning = true;
            }
        }

        for (int i = 0; i < mDrawerItems2.size(); i++) {
            if (mDrawerItems2.get(i).isProgressShowing()) {
                operationsRunning = true;
            }
        }

        if (!operationsRunning) {
            super.onBackPressed();
            return;
        }

        Toast.makeText(this, R.string.wait_until_finished, Toast.LENGTH_SHORT)
                .show();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        mDrawerToggle.onConfigurationChanged(newConfig);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (mDrawerToggle.onOptionsItemSelected(item)) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    private class DrawerItemClickListener implements
            ListView.OnItemClickListener {
        private final ListView mView;

        public DrawerItemClickListener(ListView view) {
            mView = view;
        }

        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position,
                long id) {
            selectItem(mView, position);
        }
    }

    private void updateTitle() {
        if (mDrawerLayout.isDrawerOpen(mDrawerView) || mTitle == 0) {
            getActionBar().setTitle(R.string.app_name);
        } else {
            getActionBar().setTitle(mTitle);
        }
    }

    private void selectItem(ListView view, int position) {
        if (view == mDrawerList) {
            mDrawerList2.clearChoices();
            mDrawerAdapter2.notifyDataSetChanged();

            switch (position) {
            case NAV_CHOOSE_ROM:
                mFragment = FRAGMENT_CHOOSE_ROM;
                showFragment();
                break;

            case NAV_SET_KERNEL:
                mFragment = FRAGMENT_SET_KERNEL;
                showFragment();
                break;

            case NAV_PATCH_FILE:
                mFragment = FRAGMENT_PATCH_FILE;
                showFragment();
                break;
            }
        } else if (view == mDrawerList2) {
            mDrawerList.clearChoices();
            mDrawerAdapter.notifyDataSetChanged();

            switch (position) {
            case NAV2_REBOOT:
                SwitcherUtils.reboot(this, getFragmentManager());
                break;

            case NAV2_ABOUT:
                mFragment = FRAGMENT_ABOUT;
                showFragment();
                break;
            }
        }

        mDrawerLayout.closeDrawer(mDrawerView);
    }

    private void showFragment() {
        FragmentManager fm = getFragmentManager();

        Fragment prevChooseRom = fm
                .findFragmentByTag(SwitcherListFragment.TAG_CHOOSE_ROM);
        Fragment prevSetKernel = fm
                .findFragmentByTag(SwitcherListFragment.TAG_SET_KERNEL);
        Fragment prevPatchFile = fm.findFragmentByTag(PatchFileFragment.TAG);
        Fragment prevAbout = fm.findFragmentByTag(AboutFragment.TAG);

        hideFragment(prevChooseRom);
        hideFragment(prevSetKernel);
        hideFragment(prevPatchFile);
        hideFragment(prevAbout);

        FragmentTransaction ft = fm.beginTransaction();

        switch (mFragment) {
        case FRAGMENT_CHOOSE_ROM:
            mTitle = R.string.title_choose_rom;
            updateTitle();

            if (prevChooseRom == null) {
                Fragment f = SwitcherListFragment
                        .newInstance(SwitcherListFragment.ACTION_CHOOSE_ROM);
                ft.add(R.id.content_frame, f,
                        SwitcherListFragment.TAG_CHOOSE_ROM);
            } else {
                ft.show(prevChooseRom);
            }

            ft.commit();

            break;

        case FRAGMENT_SET_KERNEL:
            mTitle = R.string.title_set_kernel;
            updateTitle();

            if (prevSetKernel == null) {
                Fragment f = SwitcherListFragment
                        .newInstance(SwitcherListFragment.ACTION_SET_KERNEL);
                ft.add(R.id.content_frame, f,
                        SwitcherListFragment.TAG_SET_KERNEL);
            } else {
                ft.show(prevSetKernel);
            }

            ft.commit();

            break;

        case FRAGMENT_PATCH_FILE:
            mTitle = R.string.title_patch_zip;
            updateTitle();

            if (prevPatchFile == null) {
                Fragment f;
                if (mAutomated) {
                    f = PatchFileFragment.newAutomatedInstance(mAutomatedData);
                } else {
                    f = PatchFileFragment.newInstance();
                }
                ft.add(R.id.content_frame, f, PatchFileFragment.TAG);
            } else {
                ft.show(prevPatchFile);
            }

            ft.commit();

            break;

        case FRAGMENT_ABOUT:
            mTitle = R.string.app_name;
            updateTitle();

            if (prevAbout == null) {
                Fragment f = AboutFragment.newInstance();
                ft.add(R.id.content_frame, f, AboutFragment.TAG);
            } else {
                ft.show(prevAbout);
            }

            ft.commit();

            break;
        }
    }

    private void hideFragment(Fragment f) {
        if (f != null) {
            FragmentManager fm = getFragmentManager();
            FragmentTransaction ft = fm.beginTransaction();
            ft.hide(f);
            ft.commit();
        }
    }

    public void lockNavigation() {
        mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED);
        getActionBar().setDisplayHomeAsUpEnabled(false);
        getActionBar().setHomeButtonEnabled(false);
    }

    public void unlockNavigation() {
        mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED);
        getActionBar().setDisplayHomeAsUpEnabled(true);
        getActionBar().setHomeButtonEnabled(true);
    }

    public void showProgress(int fragment, boolean show) {
        int index;

        switch (fragment) {
        case FRAGMENT_CHOOSE_ROM:
            index = NAV_CHOOSE_ROM;
            break;

        case FRAGMENT_SET_KERNEL:
            index = NAV_SET_KERNEL;
            break;

        case FRAGMENT_PATCH_FILE:
            index = NAV_PATCH_FILE;
            break;

        default:
            return;
        }

        NavigationDrawerItem item = mDrawerItems.get(index);
        item.setProgressShowing(show);
        mDrawerAdapter.notifyDataSetChanged();
    }
}
