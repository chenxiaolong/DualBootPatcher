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
import com.github.chenxiaolong.dualbootpatcher.settings.RomSettingsFragment;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherListFragment;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;

public class MainActivity extends Activity {
    private static final String PATCH_FILE = "com.github.chenxiaolong.dualbootpatcher.PATCH_FILE";

    private SharedPreferences mPrefs;

    private String[] mNavTitles;
    private DrawerLayout mDrawerLayout;
    private RelativeLayout mDrawerView;
    private ListView mDrawerList;
    private ArrayList<DrawerItem> mDrawerItems;
    private NavigationDrawerAdapter mDrawerAdapter;
    private ActionBarDrawerToggle mDrawerToggle;
    private int mTitle;

    private boolean mAutomated;
    private Bundle mAutomatedData;

    private static final int NAV_CHOOSE_ROM = 0;
    private static final int NAV_SET_KERNEL = 1;
    private static final int NAV_PATCH_FILE = 2;
    private static final int NAV_SETTINGS = 3;
    // Separator
    private static final int NAV_REBOOT = 5;
    private static final int NAV_ABOUT = 6;
    private static final int NAV_EXIT = 7;

    public static final int FRAGMENT_CHOOSE_ROM = 1;
    public static final int FRAGMENT_SET_KERNEL = 2;
    public static final int FRAGMENT_PATCH_FILE = 3;
    public static final int FRAGMENT_SETTINGS = 4;
    public static final int FRAGMENT_ABOUT = 5;

    private int mFragment;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.drawer_layout);

        mPrefs = getSharedPreferences("settings", 0);

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

        mDrawerItems = new ArrayList<DrawerItem>();

        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_CHOOSE_ROM],
                R.drawable.check));
        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_SET_KERNEL],
                R.drawable.pin));
        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_PATCH_FILE],
                R.drawable.split));
        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_SETTINGS],
                R.drawable.settings));
        mDrawerItems.add(new NavigationDrawerSeparatorItem());
        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_REBOOT],
                R.drawable.refresh));
        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_ABOUT],
                R.drawable.about));
        mDrawerItems.add(new NavigationDrawerItem(mNavTitles[NAV_EXIT],
                R.drawable.exit));

        mDrawerAdapter = new NavigationDrawerAdapter(this,
                R.layout.drawer_list_item, R.layout.drawer_list_item_hidden,
                mDrawerItems);
        mDrawerList.setAdapter(mDrawerAdapter);

        mDrawerList.setOnItemClickListener(new DrawerItemClickListener(
                mDrawerList));

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
                DrawerItem item = mDrawerItems.get(i);
                if (item instanceof NavigationDrawerItem) {
                    ((NavigationDrawerItem) item)
                            .setProgressShowing(progressState[i]);
                }
            }
            mDrawerAdapter.notifyDataSetChanged();
        } else {
            // Show about screen by default
            mDrawerList.performItemClick(mDrawerList, NAV_ABOUT,
                    mDrawerList.getItemIdAtPosition(NAV_ABOUT));
        }

        refreshOptionalItems();

        // Open drawer on first start
        new Thread() {
            @Override
            public void run() {
                boolean isFirstStart = mPrefs.getBoolean("first_start", true);
                if (isFirstStart) {
                    mDrawerLayout.openDrawer(mDrawerView);
                    Editor e = mPrefs.edit();
                    e.putBoolean("first_start", false);
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
            DrawerItem item = mDrawerItems.get(i);
            if (item instanceof NavigationDrawerItem) {
                progressState[i] = ((NavigationDrawerItem) item)
                        .isProgressShowing();
            }
        }
        savedInstanceState.putBooleanArray("progressState", progressState);
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
            DrawerItem item = mDrawerItems.get(i);
            if (item instanceof NavigationDrawerItem) {
                if (((NavigationDrawerItem) item).isProgressShowing()) {
                    operationsRunning = true;
                }
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

            case NAV_SETTINGS:
                mFragment = FRAGMENT_SETTINGS;
                showFragment();
                break;

            case NAV_REBOOT:
                SwitcherUtils.reboot(this, getFragmentManager());
                break;

            case NAV_ABOUT:
                mFragment = FRAGMENT_ABOUT;
                showFragment();
                break;

            case NAV_EXIT:
                finish();
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
        Fragment prevRomSettings = fm
                .findFragmentByTag(RomSettingsFragment.TAG);
        Fragment prevAbout = fm.findFragmentByTag(AboutFragment.TAG);

        hideFragment(prevChooseRom);
        hideFragment(prevSetKernel);
        hideFragment(prevPatchFile);
        hideFragment(prevRomSettings);
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

        case FRAGMENT_SETTINGS:
            mTitle = R.string.title_rom_settings;
            updateTitle();

            if (prevRomSettings == null) {
                Fragment f = RomSettingsFragment.newInstance();
                ft.add(R.id.content_frame, f, RomSettingsFragment.TAG);
            } else {
                ft.show(prevRomSettings);
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

        case FRAGMENT_SETTINGS:
            index = NAV_SETTINGS;
            break;

        default:
            return;
        }

        DrawerItem item = mDrawerItems.get(index);
        if (item instanceof NavigationDrawerItem) {
            ((NavigationDrawerItem) item).setProgressShowing(show);
            mDrawerAdapter.notifyDataSetChanged();
        }
    }

    public void refreshOptionalItems() {
        boolean reboot = mPrefs.getBoolean("show_reboot", true);
        boolean exit = mPrefs.getBoolean("show_exit", false);
        showReboot(reboot);
        showExit(exit);
    }

    public void showReboot(boolean visible) {
        NavigationDrawerItem rebootItem = (NavigationDrawerItem) mDrawerItems
                .get(NAV_REBOOT);
        rebootItem.setVisible(visible);
        mDrawerAdapter.notifyDataSetChanged();
    }

    public void showExit(boolean visible) {
        NavigationDrawerItem exitItem = (NavigationDrawerItem) mDrawerItems
                .get(NAV_EXIT);
        exitItem.setVisible(visible);
        mDrawerAdapter.notifyDataSetChanged();
    }
}
