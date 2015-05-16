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

import android.app.Dialog;
import android.app.DialogFragment;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.res.Configuration;
import android.graphics.Point;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Display;
import android.view.Gravity;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.afollestad.materialdialogs.MaterialDialog;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherListFragment;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.github.chenxiaolong.multibootpatcher.appsharing.AppSharingSettingsActivity;
import com.github.chenxiaolong.multibootpatcher.freespace.FreeSpaceFragment;
import com.github.chenxiaolong.multibootpatcher.patcher.PatchFileFragment;
import com.github.chenxiaolong.multibootpatcher.settings.RomSettingsActivity;

import org.apache.commons.lang3.ArrayUtils;

import java.util.ArrayList;

public class MainActivity extends AppCompatActivity {
    private static final int[] RES_NAV_TITLES = new int[] {
            R.string.title_roms, R.string.title_patch_zip, R.string.title_free_space,
            R.string.title_rom_settings, R.string.title_app_sharing, R.string.title_reboot,
            R.string.title_about, R.string.title_exit };

    private static final int[] RES_NAV_ICONS = new int[] { R.drawable.check,
            R.drawable.split, R.drawable.storage, R.drawable.settings, R.drawable.dashboard,
            R.drawable.refresh, R.drawable.about, R.drawable.exit };

    private static final int NAV_SEPARATOR = -1;
    private static final int NAV_ROMS = 0;
    private static final int NAV_PATCH_FILE = 1;
    private static final int NAV_FREE_SPACE = 2;
    private static final int NAV_ROM_SETTINGS = 3;
    private static final int NAV_APP_SHARING = 4;
    private static final int NAV_REBOOT = 5;
    private static final int NAV_ABOUT = 6;
    private static final int NAV_EXIT = 7;

    private SharedPreferences mPrefs;

    private DrawerLayout mDrawerLayout;
    private ScrollView mDrawerView;
    private ActionBarDrawerToggle mDrawerToggle;

    private final ArrayList<Integer> mDrawerItems = new ArrayList<>();
    private View[] mDrawerItemViews;
    private boolean[] mDrawerItemsProgress;
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
            mTitle = savedInstanceState.getInt("title");
        }

        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        mDrawerLayout = (DrawerLayout) findViewById(R.id.drawer_layout);
        mDrawerToggle = new ActionBarDrawerToggle(this, mDrawerLayout,
                R.string.drawer_open, R.string.drawer_close) {
            @Override
            public void onDrawerClosed(View view) {
                super.onDrawerClosed(view);
                updateTitle();

                if (mPending != null) {
                    mHandler.post(mPending);
                    mPending = null;
                }
            }

            @Override
            public void onDrawerOpened(View drawerView) {
                super.onDrawerOpened(drawerView);
                updateTitle();
            }
        };

        mDrawerLayout.setDrawerListener(mDrawerToggle);
        mDrawerLayout.setDrawerShadow(R.drawable.drawer_shadow, Gravity.START);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setHomeButtonEnabled(true);

        mDrawerView = (ScrollView) findViewById(R.id.left_drawer);

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

        mDrawerItems.clear();
        mDrawerItems.add(NAV_ROMS);
        mDrawerItems.add(NAV_PATCH_FILE);
        mDrawerItems.add(NAV_FREE_SPACE);
        mDrawerItems.add(NAV_ROM_SETTINGS);
        mDrawerItems.add(NAV_APP_SHARING);
        mDrawerItems.add(NAV_SEPARATOR);
        mDrawerItems.add(NAV_REBOOT);
        mDrawerItems.add(NAV_ABOUT);
        mDrawerItems.add(NAV_EXIT);
        createNavigationViews();

        if (savedInstanceState != null) {
            mFragment = savedInstanceState.getInt("fragment");

            showFragment();

            // Progress icons
            mDrawerItemsProgress = savedInstanceState
                    .getBooleanArray("progressState");

            mDrawerItemSelected = savedInstanceState.getInt("selectedItem");
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
                navId = NAV_ABOUT;
                break;
            case INITIAL_SCREEN_ROMS:
                navId = NAV_ROMS;
                break;
            case INITIAL_SCREEN_PATCHER:
                navId = NAV_PATCH_FILE;
                break;
            default:
                throw new IllegalStateException("Invalid initial screen value");
            }

            // Show about screen by default
            mDrawerItemSelected = getItemForType(navId);
        }

        onDrawerItemClicked(mDrawerItemSelected, false);

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
        savedInstanceState.putInt("fragment", mFragment);
        savedInstanceState.putInt("title", mTitle);
        savedInstanceState.putBooleanArray("progressState", mDrawerItemsProgress);
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
            getSupportActionBar().setTitle(BuildConfig.APP_NAME_RESOURCE);
        } else {
            getSupportActionBar().setTitle(mTitle);
        }
    }

    private void createNavigationViews() {
        ViewGroup container = (ViewGroup) findViewById(R.id.drawer_list);
        // container.removeAllViews();

        mDrawerItemViews = new View[mDrawerItems.size()];
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

    private boolean isItemAFragment(int item) {
        int type = mDrawerItems.get(item);

        switch (type) {
        case NAV_ROMS:
        case NAV_PATCH_FILE:
        case NAV_FREE_SPACE:
        case NAV_ABOUT:
            return true;

        default:
            return false;
        }
    }

    private View createDrawerItem(final int type, ViewGroup container) {
        int layout;

        if (isSeparator(type)) {
            layout = R.layout.nav_separator_item;
        } else {
            layout = R.layout.nav_item;
        }

        View view = getLayoutInflater().inflate(layout, container, false);

        if (isSeparator(type)) {
            view.setClickable(false);
            view.setFocusable(false);
            return view;
        }

        final TextView textView = (TextView) view.findViewById(R.id.nav_item_text);
        final ImageView imageView = (ImageView) view.findViewById(R.id.nav_item_icon);

        int titleId = RES_NAV_TITLES[type];
        int iconId = RES_NAV_ICONS[type];

        textView.setText(titleId);
        imageView.setImageResource(iconId);

        view.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                int item = getItemForType(type);
                onDrawerItemClicked(item, true);
            }
        });

        return view;
    }

    private void onDrawerItemClicked(final int item, boolean userClicked) {
        if (mDrawerItemSelected == item && userClicked) {
            mDrawerLayout.closeDrawer(mDrawerView);
            return;
        }

        if (isItemAFragment(item)) {
            mDrawerItemSelected = item;
            // Animate if user clicked
            hideFragments(userClicked);
        }

        for (int i = 0; i < mDrawerItems.size(); i++) {
            int type = mDrawerItems.get(i);

            if (!isSeparator(type)) {
                showAsSelected(i, mDrawerItemSelected == i);
            }
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

    private void showAsSelected(int item, boolean selected) {
        int type = mDrawerItems.get(item);

        switch (type) {
        case NAV_ROMS:
        case NAV_PATCH_FILE:
        case NAV_FREE_SPACE:
        case NAV_ABOUT:
            View view = mDrawerItemViews[item];
            TextView textView = (TextView) view.findViewById(R.id.nav_item_text);
            textView.setTypeface(null, selected ? Typeface.BOLD : Typeface.NORMAL);
            break;
        }
    }

    private void performDrawerItemSelection(int item) {
        int type = mDrawerItems.get(item);

        switch (type) {
        case NAV_ROMS:
            mFragment = FRAGMENT_ROMS;
            showFragment();
            break;

        case NAV_PATCH_FILE:
            mFragment = FRAGMENT_PATCH_FILE;
            showFragment();
            break;

        case NAV_FREE_SPACE:
            mFragment = FRAGMENT_FREE_SPACE;
            showFragment();
            break;

        case NAV_ROM_SETTINGS: {
            Intent intent = new Intent(this, RomSettingsActivity.class);
            startActivity(intent);
            break;
        }

        case NAV_APP_SHARING: {
            Intent intent = new Intent(this, AppSharingSettingsActivity.class);
            startActivity(intent);
            break;
        }

        case NAV_REBOOT:
            RebootingDialog d = RebootingDialog.newInstance();
            d.show(getFragmentManager(), RebootingDialog.TAG);

            SwitcherUtils.reboot(this);
            break;

        case NAV_ABOUT:
            mFragment = FRAGMENT_ABOUT;
            showFragment();
            break;

        case NAV_EXIT:
            finish();
            break;

        default:
            throw new IllegalStateException("Invalid drawer item");
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

    public void lockNavigation() {
        mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_LOCKED_CLOSED);
        getSupportActionBar().setDisplayHomeAsUpEnabled(false);
        getSupportActionBar().setHomeButtonEnabled(false);
    }

    @SuppressWarnings("unused")
    public void unlockNavigation() {
        mDrawerLayout.setDrawerLockMode(DrawerLayout.LOCK_MODE_UNLOCKED);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        getSupportActionBar().setHomeButtonEnabled(true);
    }

    public void showProgress(int fragment, boolean show) {
        int type;

        switch (fragment) {
        case FRAGMENT_ROMS:
            type = NAV_ROMS;
            break;

        case FRAGMENT_PATCH_FILE:
            type = NAV_PATCH_FILE;
            break;

        case FRAGMENT_FREE_SPACE:
            type = NAV_FREE_SPACE;
            break;

        default:
            return;
        }

        int item = getItemForType(type);

        mDrawerItemsProgress[item] = show;

        View view = mDrawerItemViews[item];
        ProgressBar progressBar = (ProgressBar) view
                .findViewById(R.id.nav_item_progress);

        progressBar.setVisibility(show ? View.VISIBLE : View.GONE);
    }

    private void refreshProgressBars() {
        for (int i = 0; i < mDrawerItemsProgress.length; i++) {
            int type = mDrawerItems.get(i);

            if (!isSeparator(type)) {
                View view = mDrawerItemViews[i];
                ProgressBar progressBar = (ProgressBar) view
                        .findViewById(R.id.nav_item_progress);

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

    public static class RebootingDialog extends DialogFragment {
        private static final String TAG = RebootingDialog.class.getSimpleName();

        public static RebootingDialog newInstance() {
            RebootingDialog frag = new RebootingDialog();
            return frag;
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            Dialog dialog = new MaterialDialog.Builder(getActivity())
                    .content(R.string.please_wait)
                    .progress(true, 0)
                    .build();

            setCancelable(false);
            dialog.setCanceledOnTouchOutside(false);

            return dialog;
        }
    }
}
