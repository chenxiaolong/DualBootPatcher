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

package com.github.chenxiaolong.dualbootpatcher;

import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.Configuration;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.IdRes;
import android.support.design.widget.NavigationView;
import android.support.design.widget.NavigationView.OnNavigationItemSelectedListener;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBar;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Display;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.appsharing.AppSharingSettingsActivity;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.freespace.FreeSpaceFragment;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatchFileFragment;
import com.github.chenxiaolong.dualbootpatcher.settings.RomSettingsActivity;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherListFragment;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.squareup.picasso.Picasso;

import org.apache.commons.lang3.ArrayUtils;

public class MainActivity extends AppCompatActivity implements OnNavigationItemSelectedListener {
    private static final String EXTRA_SELECTED_ITEM = "selected_item";
    private static final String EXTRA_TITLE = "title";
    private static final String EXTRA_FRAGMENT = "fragment";

    private static final String PREF_SHOW_REBOOT = "show_reboot";
    private static final String PREF_SHOW_EXIT = "show_exit";

    private SharedPreferences mPrefs;

    private DrawerLayout mDrawerLayout;
    private NavigationView mDrawerView;
    private ActionBarDrawerToggle mDrawerToggle;

    private int mDrawerItemSelected;

    private int mTitle;
    private Handler mHandler;
    private Runnable mPending;

    public static final int FRAGMENT_ROMS = 1;
    public static final int FRAGMENT_PATCH_FILE = 2;
    public static final int FRAGMENT_FREE_SPACE = 3;
    public static final int FRAGMENT_ABOUT = 4;

    private int mFragment;

    private static final String INITIAL_SCREEN_ABOUT = "ABOUT";
    private static final String INITIAL_SCREEN_ROMS = "ROMS";
    private static final String INITIAL_SCREEN_PATCHER = "PATCHER";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.drawer_layout);

        mHandler = new Handler();

        mPrefs = getSharedPreferences("settings", 0);

        if (savedInstanceState != null) {
            mTitle = savedInstanceState.getInt(EXTRA_TITLE);
        }

        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
        mDrawerToggle = new ActionBarDrawerToggle(this, mDrawerLayout,
                R.string.drawer_open, R.string.drawer_close) {
            @Override
            public void onDrawerClosed(View view) {
                super.onDrawerClosed(view);

                if (mPending != null) {
                    mHandler.post(mPending);
                    mPending = null;
                }
            }
        };

        mDrawerLayout.setDrawerListener(mDrawerToggle);
        mDrawerLayout.setDrawerShadow(R.drawable.drawer_shadow, GravityCompat.START);
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setHomeButtonEnabled(true);
        }

        mDrawerView = (NavigationView) findViewById(R.id.left_drawer);
        mDrawerView.setNavigationItemSelectedListener(this);

        // There's a weird performance issue when the drawer is first opened, no matter if we set
        // the background on the nav header RelativeLayout, set the image on an ImageView, or use
        // Picasso to do either asynchronously. By accident, I noticed that using Picasso's resize()
        // method with any dimensions (even the original) works around the performance issue. Maybe
        // something doesn't like PNGs exported from GIMP?
        View header = mDrawerView.inflateHeaderView(R.layout.nav_header);
        ImageView navImage = (ImageView) header.findViewById(R.id.nav_header_image);
        BitmapFactory.Options dimensions = new BitmapFactory.Options();
        dimensions.inJustDecodeBounds = true;
        BitmapFactory.decodeResource(getResources(), R.drawable.material, dimensions);
        Picasso.with(this).load(R.drawable.material)
                .resize(dimensions.outWidth, dimensions.outHeight).into(navImage);

        // Set nav drawer header text
        TextView appName = (TextView) header.findViewById(R.id.nav_header_app_name);
        appName.setText(BuildConfig.APP_NAME_RESOURCE);
        TextView appVersion = (TextView) header.findViewById(R.id.nav_header_app_version);
        appVersion.setText(String.format(getString(R.string.version), BuildConfig.VERSION_NAME));

        // Nav drawer width according to material design guidelines
        // http://www.google.com/design/spec/patterns/navigation-drawer.html
        // https://medium.com/sebs-top-tips/material-navigation-drawer-sizing-558aea1ad266
        final Display display = getWindowManager().getDefaultDisplay();
        final Point size = new Point();
        display.getSize(size);

        final ViewGroup.LayoutParams params = mDrawerView.getLayoutParams();
        int toolbarHeight = getResources().getDimensionPixelSize(
                R.dimen.abc_action_bar_default_height_material);
        params.width = Math.min(size.x - toolbarHeight, 6 * toolbarHeight);
        mDrawerView.setLayoutParams(params);

        if (savedInstanceState != null) {
            mFragment = savedInstanceState.getInt(EXTRA_FRAGMENT);

            showFragment();

            mDrawerItemSelected = savedInstanceState.getInt(EXTRA_SELECTED_ITEM);
        } else {
            String[] initialScreens = getResources().getStringArray(
                    R.array.initial_screen_entry_values);
            String initialScreen = mPrefs.getString("initial_screen", null);
            if (initialScreen == null || !ArrayUtils.contains(initialScreens, initialScreen)) {
                initialScreen = INITIAL_SCREEN_ABOUT;
                Editor e = mPrefs.edit();
                e.putString("initial_screen", initialScreen);
                e.apply();
            }

            int navId;
            switch (initialScreen) {
            case INITIAL_SCREEN_ABOUT:
                navId = R.id.nav_about;
                break;
            case INITIAL_SCREEN_ROMS:
                navId = R.id.nav_roms;
                break;
            case INITIAL_SCREEN_PATCHER:
                navId = R.id.nav_patch_zip;
                break;
            default:
                throw new IllegalStateException("Invalid initial screen value");
            }

            // Show about screen by default
            mDrawerItemSelected = navId;
        }

        onDrawerItemClicked(mDrawerItemSelected, false);

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
                    e.apply();
                }
            }
        }.start();
    }

    @Override
    public void onResume() {
        super.onResume();
        refreshOptionalItems();
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);
        savedInstanceState.putInt(EXTRA_FRAGMENT, mFragment);
        savedInstanceState.putInt(EXTRA_TITLE, mTitle);
        savedInstanceState.putInt(EXTRA_SELECTED_ITEM, mDrawerItemSelected);
    }

    @Override
    protected void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);
        mDrawerToggle.syncState();

        updateTitle();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        mDrawerToggle.onConfigurationChanged(newConfig);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        return mDrawerToggle.onOptionsItemSelected(item) || super.onOptionsItemSelected(item);
    }

    @Override
    public boolean onNavigationItemSelected(MenuItem menuItem) {
        onDrawerItemClicked(menuItem.getItemId(), true);
        return false;
    }

    private void updateTitle() {
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            if (mTitle == 0) {
                actionBar.setTitle(BuildConfig.APP_NAME_RESOURCE);
            } else {
                actionBar.setTitle(mTitle);
            }
        }
    }

    private boolean isItemAFragment(@IdRes int item) {
        switch (item) {
        case R.id.nav_roms:
        case R.id.nav_patch_zip:
        case R.id.nav_free_space:
        case R.id.nav_about:
            return true;

        default:
            return false;
        }
    }

    private void onDrawerItemClicked(@IdRes final int item, boolean userClicked) {
        if (mDrawerItemSelected == item && userClicked) {
            mDrawerLayout.closeDrawer(mDrawerView);
            return;
        }

        if (isItemAFragment(item)) {
            mDrawerItemSelected = item;
            // Animate if user clicked
            hideFragments(userClicked);
        }

        Menu menu = mDrawerView.getMenu();
        for (int i = 0; i < menu.size(); i++) {
            MenuItem menuItem = menu.getItem(i);
            menuItem.setChecked(menuItem.getItemId() == item);
        }

        if (mDrawerLayout.isDrawerOpen(mDrawerView)) {
            mPending = new Runnable() {
                @Override
                public void run() {
                    performDrawerItemSelection(item);
                }
            };
        } else {
            performDrawerItemSelection(item);
        }

        mDrawerLayout.closeDrawer(mDrawerView);
    }

    private void performDrawerItemSelection(@IdRes int item) {
        switch (item) {
        case R.id.nav_roms:
            mFragment = FRAGMENT_ROMS;
            showFragment();
            break;

        case R.id.nav_patch_zip:
            mFragment = FRAGMENT_PATCH_FILE;
            showFragment();
            break;

        case R.id.nav_free_space:
            mFragment = FRAGMENT_FREE_SPACE;
            showFragment();
            break;

        case R.id.nav_rom_settings: {
            Intent intent = new Intent(this, RomSettingsActivity.class);
            startActivity(intent);
            break;
        }

        case R.id.nav_app_sharing: {
            Intent intent = new Intent(this, AppSharingSettingsActivity.class);
            startActivity(intent);
            break;
        }

        case R.id.nav_reboot:
            GenericProgressDialog d = GenericProgressDialog.newInstance(0, R.string.please_wait);
            d.show(getFragmentManager(), GenericProgressDialog.TAG);

            SwitcherUtils.reboot(this);
            break;

        case R.id.nav_about:
            mFragment = FRAGMENT_ABOUT;
            showFragment();
            break;

        case R.id.nav_exit:
            finish();
            break;

        default:
            throw new IllegalStateException("Invalid drawer item");
        }
    }

    private void hideFragments(boolean animate) {
        FragmentManager fm = getFragmentManager();

        Fragment prevRoms = fm.findFragmentByTag(SwitcherListFragment.TAG);
        Fragment prevPatchFile = fm.findFragmentByTag(PatchFileFragment.TAG);
        Fragment prevFreeSpace = fm.findFragmentByTag(FreeSpaceFragment.TAG);
        Fragment prevAbout = fm.findFragmentByTag(AboutFragment.TAG);

        FragmentTransaction ft = fm.beginTransaction();

        if (animate) {
            ft.setCustomAnimations(0, R.animator.fragment_out);
        }

        if (prevRoms != null) {
            ft.hide(prevRoms);
        }
        if (prevPatchFile != null) {
            ft.hide(prevPatchFile);
        }
        if (prevFreeSpace != null) {
            ft.hide(prevFreeSpace);
        }
        if (prevAbout != null) {
            ft.hide(prevAbout);
        }

        ft.commit();
        fm.executePendingTransactions();
    }

    private void showFragment() {
        FragmentManager fm = getFragmentManager();

        Fragment prevRoms = fm.findFragmentByTag(SwitcherListFragment.TAG);
        Fragment prevPatchFile = fm.findFragmentByTag(PatchFileFragment.TAG);
        Fragment prevFreeSpace = fm.findFragmentByTag(FreeSpaceFragment.TAG);
        Fragment prevAbout = fm.findFragmentByTag(AboutFragment.TAG);

        FragmentTransaction ft = fm.beginTransaction();
        ft.setCustomAnimations(R.animator.fragment_in, 0);

        switch (mFragment) {
        case FRAGMENT_ROMS:
            mTitle = R.string.title_roms;
            updateTitle();

            if (prevRoms == null) {
                Fragment f = SwitcherListFragment.newInstance();
                ft.add(R.id.content_frame, f,
                        SwitcherListFragment.TAG);
            } else {
                ft.show(prevRoms);
            }

            break;

        case FRAGMENT_PATCH_FILE:
            mTitle = R.string.title_patch_zip;
            updateTitle();

            if (prevPatchFile == null) {
                Fragment f = PatchFileFragment.newInstance();
                ft.add(R.id.content_frame, f, PatchFileFragment.TAG);
            } else {
                ft.show(prevPatchFile);
            }

            break;

        case FRAGMENT_FREE_SPACE:
            mTitle = R.string.title_free_space;
            updateTitle();

            if (prevFreeSpace == null) {
                Fragment f = FreeSpaceFragment.newInstance();
                ft.add(R.id.content_frame, f, FreeSpaceFragment.TAG);
            } else {
                ft.show(prevFreeSpace);
            }

            break;

        case FRAGMENT_ABOUT:
            mTitle = BuildConfig.APP_NAME_RESOURCE;
            updateTitle();

            if (prevAbout == null) {
                Fragment f = AboutFragment.newInstance();
                ft.add(R.id.content_frame, f, AboutFragment.TAG);
            } else {
                ft.show(prevAbout);
            }

            break;

        default:
            throw new IllegalStateException("Invalid fragment ID");
        }

        ft.commit();
    }

    @SuppressWarnings("unused")
    public void lockNavigation() {
        mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED);
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(false);
            actionBar.setHomeButtonEnabled(false);
        }
    }

    @SuppressWarnings("unused")
    public void unlockNavigation() {
        mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED);
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setHomeButtonEnabled(true);
        }
    }

    public void refreshOptionalItems() {
        boolean reboot = mPrefs.getBoolean(PREF_SHOW_REBOOT, true);
        boolean exit = mPrefs.getBoolean(PREF_SHOW_EXIT, false);
        showReboot(reboot);
        showExit(exit);
    }

    public void showReboot(boolean visible) {
        Menu menu = mDrawerView.getMenu();
        MenuItem item = menu.findItem(R.id.nav_reboot);
        if (item != null) {
            item.setVisible(visible);
        }
    }

    public void showExit(boolean visible) {
        Menu menu = mDrawerView.getMenu();
        MenuItem item = menu.findItem(R.id.nav_exit);
        if (item != null) {
            item.setVisible(visible);
        }
    }
}
