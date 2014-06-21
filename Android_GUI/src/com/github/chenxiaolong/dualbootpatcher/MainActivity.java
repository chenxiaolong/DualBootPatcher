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
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.app.ActionBarDrawerToggle;
import android.support.v4.widget.DrawerLayout;
import android.view.Gravity;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.patcher.PatchFileFragment;
import com.github.chenxiaolong.dualbootpatcher.settings.RomSettingsFragment;
import com.github.chenxiaolong.dualbootpatcher.settings.SettingsActivity;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherListFragment;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;

public class MainActivity extends Activity {
    private static final String PATCH_FILE = "com.github.chenxiaolong.dualbootpatcher.PATCH_FILE";

    private static final int[] RES_NAV_TITLES = new int[] {
            R.string.title_choose_rom, R.string.title_set_kernel,
            R.string.title_patch_zip, R.string.title_rom_settings,
            R.string.title_reboot, R.string.title_about, R.string.title_exit };

    private static final int[] RES_NAV_ICONS = new int[] { R.drawable.check,
            R.drawable.pin, R.drawable.split, R.drawable.settings,
            R.drawable.refresh, R.drawable.about, R.drawable.exit };

    private static final int NAV_SEPARATOR = -1;
    private static final int NAV_CHOOSE_ROM = 0;
    private static final int NAV_SET_KERNEL = 1;
    private static final int NAV_PATCH_FILE = 2;
    private static final int NAV_SETTINGS = 3;
    private static final int NAV_REBOOT = 4;
    private static final int NAV_ABOUT = 5;
    private static final int NAV_EXIT = 6;

    private SharedPreferences mPrefs;

    private DrawerLayout mDrawerLayout;
    private RelativeLayout mDrawerView;
    private ActionBarDrawerToggle mDrawerToggle;

    private final ArrayList<Integer> mDrawerItems = new ArrayList<Integer>();
    private View[] mDrawerItemViews;
    private boolean[] mDrawerItemsLoaded;
    private boolean[] mDrawerItemsProgress;
    private int mDrawerItemSelected;

    private int mTitle;
    private Handler mHandler;

    private boolean mAutomated;
    private Bundle mAutomatedData;

    public static final int FRAGMENT_CHOOSE_ROM = 1;
    public static final int FRAGMENT_SET_KERNEL = 2;
    public static final int FRAGMENT_PATCH_FILE = 3;
    public static final int FRAGMENT_ABOUT = 4;

    private int mFragment;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.drawer_layout);

        mHandler = new Handler();

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

        mDrawerItems.clear();
        mDrawerItems.add(NAV_CHOOSE_ROM);
        mDrawerItems.add(NAV_SET_KERNEL);
        mDrawerItems.add(NAV_PATCH_FILE);
        mDrawerItems.add(NAV_SETTINGS);
        mDrawerItems.add(NAV_SEPARATOR);
        mDrawerItems.add(NAV_REBOOT);
        mDrawerItems.add(NAV_ABOUT);
        mDrawerItems.add(NAV_EXIT);
        createNavigationViews();

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
            mDrawerItemsProgress = savedInstanceState
                    .getBooleanArray("progressState");

            mDrawerItemSelected = savedInstanceState.getInt("selectedItem");
        } else {
            // Show about screen by default
            mDrawerItemSelected = getItemForType(NAV_ABOUT);
        }

        onDrawerItemClicked(mDrawerItemSelected);

        refreshProgressBars();
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
        savedInstanceState.putBooleanArray("progressState",
                mDrawerItemsProgress);
        savedInstanceState.putInt("selectedItem", mDrawerItemSelected);
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
            int type = mDrawerItems.get(i);

            if (!isSeparator(type)) {
                if (mDrawerItemsProgress[i]) {
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

    private void updateTitle() {
        if (mDrawerLayout.isDrawerOpen(mDrawerView) || mTitle == 0) {
            getActionBar().setTitle(R.string.app_name);
        } else {
            getActionBar().setTitle(mTitle);
        }
    }

    private void createNavigationViews() {
        ViewGroup container = (ViewGroup) findViewById(R.id.drawer_list);
        // container.removeAllViews();

        mDrawerItemViews = new View[mDrawerItems.size()];
        mDrawerItemsLoaded = new boolean[mDrawerItems.size()];
        mDrawerItemsProgress = new boolean[mDrawerItems.size()];

        for (int i = 0; i < mDrawerItems.size(); i++) {
            mDrawerItemViews[i] = createDrawerItem(mDrawerItems.get(i),
                    container);
            container.addView(mDrawerItemViews[i]);
        }
    }

    private boolean isSeparator(int type) {
        return type == NAV_SEPARATOR;
    }

    private View createDrawerItem(final int type, ViewGroup container) {
        int layout;

        if (isSeparator(type)) {
            layout = R.layout.drawer_list_separator_item;
        } else {
            layout = R.layout.drawer_list_item;
        }

        View view = getLayoutInflater().inflate(layout, container, false);

        if (isSeparator(type)) {
            view.setClickable(false);
            view.setFocusable(false);
            return view;
        }

        final TextView textView = (TextView) view
                .findViewById(R.id.drawer_text);
        ImageView imageView = (ImageView) view.findViewById(R.id.drawer_icon);

        int titleId = RES_NAV_TITLES[type];
        int iconId = RES_NAV_ICONS[type];

        textView.setText(titleId);
        imageView.setImageResource(iconId);

        view.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                int item = getItemForType(type);
                onDrawerItemClicked(item);
            }
        });

        return view;
    }

    private void onDrawerItemClicked(final int item) {
        mDrawerLayout.closeDrawer(mDrawerView);
        mDrawerItemSelected = item;

        for (int i = 0; i < mDrawerItems.size(); i++) {
            int type = mDrawerItems.get(i);

            if (!isSeparator(type)) {
                showAsSelected(i, mDrawerItemSelected == i);
            }
        }

        // Add a small delay so the navigation drawer doesn't stutter
        if (mDrawerItemsLoaded[item]) {
            performDrawerItemSelection(item);
        } else {
            mDrawerItemsLoaded[item] = true;

            mHandler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    performDrawerItemSelection(item);
                }
            }, 200);
        }
    }

    private void showAsSelected(int item, boolean selected) {
        int type = mDrawerItems.get(item);

        switch (type) {
        case NAV_CHOOSE_ROM:
        case NAV_SET_KERNEL:
        case NAV_PATCH_FILE:
        case NAV_ABOUT:
            View view = mDrawerItemViews[item];
            TextView textView = (TextView) view.findViewById(R.id.drawer_text);
            textView.setTypeface(null, selected ? Typeface.BOLD
                    : Typeface.NORMAL);
            break;
        }
    }

    private void performDrawerItemSelection(int item) {
        int type = mDrawerItems.get(item);

        switch (type) {
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
            startActivity(new Intent(this, SettingsActivity.class));
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

    private int getItemForType(int type) {
        for (int i = 0; i < mDrawerItems.size(); i++) {
            int curType = mDrawerItems.get(i);

            if (type == curType) {
                return i;
            }
        }

        // We'll never hit this
        return -1;
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
        int type;

        switch (fragment) {
        case FRAGMENT_CHOOSE_ROM:
            type = NAV_CHOOSE_ROM;
            break;

        case FRAGMENT_SET_KERNEL:
            type = NAV_SET_KERNEL;
            break;

        case FRAGMENT_PATCH_FILE:
            type = NAV_PATCH_FILE;
            break;

        default:
            return;
        }

        int item = getItemForType(type);

        mDrawerItemsProgress[item] = show;

        View view = mDrawerItemViews[item];
        ProgressBar progressBar = (ProgressBar) view
                .findViewById(R.id.drawer_progress);

        progressBar.setVisibility(show ? View.VISIBLE : View.GONE);
    }

    private void refreshProgressBars() {
        for (int i = 0; i < mDrawerItemsProgress.length; i++) {
            int type = mDrawerItems.get(i);

            if (!isSeparator(type)) {
                View view = mDrawerItemViews[i];
                ProgressBar progressBar = (ProgressBar) view
                        .findViewById(R.id.drawer_progress);

                if (mDrawerItemsProgress[i]) {
                    progressBar.setVisibility(View.VISIBLE);
                } else {
                    progressBar.setVisibility(View.GONE);
                }
            }
        }
    }

    public void refreshOptionalItems() {
        boolean reboot = mPrefs.getBoolean("show_reboot", true);
        boolean exit = mPrefs.getBoolean("show_exit", false);
        showReboot(reboot);
        showExit(exit);
    }

    public void showReboot(boolean visible) {
        int item = getItemForType(NAV_REBOOT);
        View view = mDrawerItemViews[item];
        view.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    public void showExit(boolean visible) {
        int item = getItemForType(NAV_EXIT);
        View view = mDrawerItemViews[item];
        view.setVisibility(visible ? View.VISIBLE : View.GONE);
    }
}
