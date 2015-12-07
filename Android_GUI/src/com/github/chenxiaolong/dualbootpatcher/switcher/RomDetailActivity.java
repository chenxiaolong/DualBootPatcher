/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.res.ColorStateList;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.StringRes;
import android.support.design.widget.CollapsingToolbarLayout;
import android.support.design.widget.CoordinatorLayout;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v4.content.ContextCompat;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v4.graphics.ColorUtils;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.graphics.Palette;
import android.support.v7.graphics.Palette.Swatch;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.CacheWallpaperResult;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.picasso.PaletteGeneratorCallback;
import com.github.chenxiaolong.dualbootpatcher.picasso.PaletteGeneratorTransformation;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SwitchRomResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.AddToHomeScreenOptionsDialog
        .AddToHomeScreenOptionsDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.CacheRomThumbnailTask
        .CacheRomThumbnailTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmChecksumIssueDialog
        .ConfirmChecksumIssueDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmMismatchedSetKernelDialog
        .ConfirmMismatchedSetKernelDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.ActionItem;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.DividerItemDecoration;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.InfoItem;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.Item;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.RomCardItem;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.RomDetailAdapterListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomNameInputDialog
        .RomNameInputDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SetKernelConfirmDialog
        .SetKernelConfirmDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.WipeTargetsSelectionDialog
        .WipeTargetsSelectionDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask.TaskState;
import com.squareup.picasso.Picasso;
import com.squareup.picasso.RequestCreator;

import java.io.File;
import java.util.ArrayList;

import mbtool.daemon.v3.MbWipeTarget;

public class RomDetailActivity extends AppCompatActivity implements
        RomNameInputDialogListener,
        AddToHomeScreenOptionsDialogListener,
        CacheRomThumbnailTaskListener,
        RomDetailAdapterListener,
        ConfirmMismatchedSetKernelDialogListener,
        SetKernelConfirmDialogListener,
        WipeTargetsSelectionDialogListener,
        ConfirmChecksumIssueDialogListener, ServiceConnection {
    private static final String TAG = RomDetailActivity.class.getSimpleName();

    private static final int MENU_EDIT_NAME = Menu.FIRST;
    private static final int MENU_CHANGE_IMAGE = Menu.FIRST + 1;
    private static final int MENU_RESET_IMAGE = Menu.FIRST + 2;

    private static final int INFO_SLOT = 1;
    private static final int INFO_VERSION = 2;
    private static final int INFO_BUILD = 3;
    private static final int INFO_MBTOOL_VERSION = 4;
    private static final int INFO_APPS_COUNTS = 5;
    private static final int INFO_SYSTEM_PATH = 6;
    private static final int INFO_CACHE_PATH = 7;
    private static final int INFO_DATA_PATH = 8;
    private static final int INFO_SYSTEM_SIZE = 9;
    private static final int INFO_CACHE_SIZE = 10;
    private static final int INFO_DATA_SIZE = 11;

    private static final int ACTION_SET_KERNEL = 1;
    private static final int ACTION_BACKUP = 2;
    private static final int ACTION_ADD_TO_HOME_SCREEN = 3;
    private static final int ACTION_WIPE_ROM = 4;

    private static final int PROGRESS_DIALOG_SWITCH_ROM = 1;
    private static final int PROGRESS_DIALOG_SET_KERNEL = 2;
    private static final int PROGRESS_DIALOG_WIPE_ROM = 3;

    private static final int REQUEST_IMAGE = 1234;

    // Argument extras
    public static final String EXTRA_ROM_INFO = "rom_info";
    public static final String EXTRA_BOOTED_ROM_INFO = "booted_rom_info";
    public static final String EXTRA_ACTIVE_ROM_ID = "active_rom_id";
    // Saved state extras
    private static final String EXTRA_STATE_TASK_ID_CACHE_WALLPAPER = "state.cache_wallpaper";
    private static final String EXTRA_STATE_TASK_ID_SWITCH_ROM = "state.switch_rom";
    private static final String EXTRA_STATE_TASK_ID_SET_KERNEL = "state.set_kernel";
    private static final String EXTRA_STATE_TASK_ID_WIPE_ROM = "state.wipe_rom";
    private static final String EXTRA_STATE_TASK_ID_CREATE_LAUNCHER = "state.create_launcher";
    private static final String EXTRA_STATE_TASK_ID_GET_ROM_DETAILS = "state.get_rom_details";
    private static final String EXTRA_STATE_TASK_IDS_TO_REMOVE = "state.task_ids_to_reomve";

    /** Intent filter for messages we care about from the service */
    private static final IntentFilter SERVICE_INTENT_FILTER = new IntentFilter();

    private CoordinatorLayout mCoordinator;
    private CollapsingToolbarLayout mCollapsing;
    private ImageView mWallpaper;
    private FloatingActionButton mFab;
    private RecyclerView mRV;

    private RomInformation mRomInfo;
    private RomInformation mBootedRomInfo;
    private String mActiveRomId;

    private ArrayList<Item> mItems = new ArrayList<>();
    private RomDetailAdapter mAdapter;

    private int mTaskIdCacheWallpaper = -1;
    private int mTaskIdSwitchRom = -1;
    private int mTaskIdSetKernel = -1;
    private int mTaskIdWipeRom = -1;
    private int mTaskIdCreateLauncher = -1;
    private int mTaskIdGetRomDetails = -1;

    /** Task IDs to remove */
    private ArrayList<Integer> mTaskIdsToRemove = new ArrayList<>();

    /** Switcher service */
    private SwitcherService mService;
    /** Broadcast receiver for events from the service */
    private SwitcherEventReceiver mReceiver = new SwitcherEventReceiver();

    static {
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_CACHED_WALLPAPER);
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_SWITCHED_ROM);
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_SET_KERNEL);
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_WIPED_ROM);
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_CREATED_LAUNCHER);
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_ROM_DETAILS_GOT_SYSTEM_SIZE);
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_ROM_DETAILS_GOT_CACHE_SIZE);
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_ROM_DETAILS_GOT_DATA_SIZE);
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_ROM_DETAILS_GOT_PACKAGES_COUNTS);
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_ROM_DETAILS_FINISHED);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_rom_detail);

        Intent intent = getIntent();
        mRomInfo = intent.getParcelableExtra(EXTRA_ROM_INFO);
        mBootedRomInfo = intent.getParcelableExtra(EXTRA_BOOTED_ROM_INFO);
        mActiveRomId = intent.getStringExtra(EXTRA_ACTIVE_ROM_ID);

        if (savedInstanceState != null) {
            mTaskIdCacheWallpaper = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_CACHE_WALLPAPER);
            mTaskIdSwitchRom = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_SWITCH_ROM);
            mTaskIdSetKernel = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_SET_KERNEL);
            mTaskIdWipeRom = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_WIPE_ROM);
            mTaskIdCreateLauncher = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_CREATE_LAUNCHER);
            mTaskIdGetRomDetails = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_GET_ROM_DETAILS);
            mTaskIdsToRemove = savedInstanceState.getIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE);
        }

        final Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mCoordinator = (CoordinatorLayout) findViewById(R.id.coordinator);
        mCollapsing = (CollapsingToolbarLayout) findViewById(R.id.collapsing_toolbar);
        mWallpaper = (ImageView) findViewById(R.id.wallpaper);
        mFab = (FloatingActionButton) findViewById(R.id.fab);
        mRV = (RecyclerView) findViewById(R.id.main_rv);

        mFab.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                switchRom(false);
            }
        });

        updateTitle();

        initializeRecyclerView();
        populateRomCardItem();
        populateInfoItems();
        populateActionItems();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt(EXTRA_STATE_TASK_ID_CACHE_WALLPAPER, mTaskIdCacheWallpaper);
        outState.putInt(EXTRA_STATE_TASK_ID_SWITCH_ROM, mTaskIdSwitchRom);
        outState.putInt(EXTRA_STATE_TASK_ID_SET_KERNEL, mTaskIdSetKernel);
        outState.putInt(EXTRA_STATE_TASK_ID_WIPE_ROM, mTaskIdWipeRom);
        outState.putInt(EXTRA_STATE_TASK_ID_CREATE_LAUNCHER, mTaskIdCreateLauncher);
        outState.putInt(EXTRA_STATE_TASK_ID_GET_ROM_DETAILS, mTaskIdGetRomDetails);
        outState.putIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE, mTaskIdsToRemove);
    }

    @Override
    protected void onStart() {
        super.onStart();

        // Start and bind to the service
        Intent intent = new Intent(this, SwitcherService.class);
        bindService(intent, this, BIND_AUTO_CREATE);
        startService(intent);

        // Register our broadcast receiver for our service
        LocalBroadcastManager.getInstance(this).registerReceiver(
                mReceiver, SERVICE_INTENT_FILTER);
    }

    @Override
    protected void onStop() {
        super.onStop();

        if (isFinishing()) {
            if (mTaskIdCacheWallpaper >= 0) {
                removeCachedTaskId(mTaskIdCacheWallpaper);
            }
            if (mTaskIdGetRomDetails >= 0) {
                removeCachedTaskId(mTaskIdGetRomDetails);
            }
        }

        // Unbind from our service
        unbindService(this);
        mService = null;

        // Unregister the broadcast receiver
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mReceiver);
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        // Save a reference to the service so we can interact with it
        ThreadPoolServiceBinder binder = (ThreadPoolServiceBinder) service;
        mService = (SwitcherService) binder.getService();

        // Remove old task IDs
        for (int taskId : mTaskIdsToRemove) {
            mService.removeCachedTask(taskId);
        }
        mTaskIdsToRemove.clear();

        if (mTaskIdCacheWallpaper < 0) {
            mTaskIdCacheWallpaper = mService.cacheWallpaper(mRomInfo);
        } else if (mService.getCachedTaskState(mTaskIdCacheWallpaper) == TaskState.FINISHED) {
            CacheWallpaperResult result =
                    mService.getResultCachedWallpaperResult(mTaskIdCacheWallpaper);
            onCachedWallpaper(result);
        }

        if (mTaskIdGetRomDetails < 0) {
            mTaskIdGetRomDetails = mService.getRomDetails(mRomInfo);
        } else {
            if (mService.hasResultRomDetailsSystemSize(mTaskIdGetRomDetails)) {
                boolean success = mService.getResultRomDetailsSystemSizeSuccess(mTaskIdGetRomDetails);
                long size = mService.getResultRomDetailsSystemSize(mTaskIdGetRomDetails);
                onHaveSystemSize(success, size);
            }
            if (mService.hasResultRomDetailsCacheSize(mTaskIdGetRomDetails)) {
                boolean success = mService.getResultRomDetailsCacheSizeSuccess(mTaskIdGetRomDetails);
                long size = mService.getResultRomDetailsCacheSize(mTaskIdGetRomDetails);
                onHaveCacheSize(success, size);
            }
            if (mService.hasResultRomDetailsDataSize(mTaskIdGetRomDetails)) {
                boolean success = mService.getResultRomDetailsDataSizeSuccess(mTaskIdGetRomDetails);
                long size = mService.getResultRomDetailsDataSize(mTaskIdGetRomDetails);
                onHaveDataSize(success, size);
            }
            if (mService.hasResultRomDetailsPackagesCounts(mTaskIdGetRomDetails)) {
                boolean success = mService.getResultRomDetailsPackagesCountsSuccess(mTaskIdGetRomDetails);
                int systemPackages = mService.getResultRomDetailsSystemPackages(mTaskIdGetRomDetails);
                int updatedPackages = mService.getResultRomDetailsUpdatedPackages(mTaskIdGetRomDetails);
                int userPackages = mService.getResultRomDetailsUserPackages(mTaskIdGetRomDetails);
                onHavePackagesCounts(success, systemPackages, updatedPackages, userPackages);
            }
        }

        if (mTaskIdCreateLauncher >= 0 &&
                mService.getCachedTaskState(mTaskIdCreateLauncher) == TaskState.FINISHED) {
            onCreatedLauncher();
        }

        if (mTaskIdSwitchRom >= 0 &&
                mService.getCachedTaskState(mTaskIdSwitchRom) == TaskState.FINISHED) {
            SwitchRomResult result = mService.getResultSwitchedRomResult(mTaskIdSwitchRom);
            onSwitchedRom(result);
        }

        if (mTaskIdSetKernel >= 0 &&
                mService.getCachedTaskState(mTaskIdSetKernel) == TaskState.FINISHED) {
            SetKernelResult result = mService.getResultSetKernelResult(mTaskIdSetKernel);
            onSetKernel(result);
        }

        if (mTaskIdWipeRom >= 0 &&
                mService.getCachedTaskState(mTaskIdWipeRom) == TaskState.FINISHED) {
            short[] succeeded = mService.getResultWipedRomTargetsSucceeded(mTaskIdWipeRom);
            short[] failed = mService.getResultWipedRomTargetsFailed(mTaskIdWipeRom);
            onWipedRom(succeeded, failed);
        }
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        mService = null;
    }

    private void removeCachedTaskId(int taskId) {
        if (mService != null) {
            mService.removeCachedTask(taskId);
        } else {
            mTaskIdsToRemove.add(taskId);
        }
    }

    private void updateTitle() {
        mCollapsing.setTitle(mRomInfo.getId());
    }

    private void initializeRecyclerView() {
        mAdapter = new RomDetailAdapter(mItems, this);

        mRV.setHasFixedSize(true);
        mRV.setAdapter(mAdapter);
        mRV.setLayoutManager(new LinearLayoutManager(this));
        mRV.addItemDecoration(new DividerItemDecoration(this));
    }

    private void populateRomCardItem() {
        RomCardItem item = new RomCardItem(mRomInfo);
        mItems.add(item);
    }

    private void populateInfoItems() {
        mItems.add(new InfoItem(INFO_SLOT,
                getString(R.string.rom_details_info_slot),
                mRomInfo.getId()));
        mItems.add(new InfoItem(INFO_VERSION,
                getString(R.string.rom_details_info_version),
                mRomInfo.getVersion()));
        mItems.add(new InfoItem(INFO_BUILD,
                getString(R.string.rom_details_info_build),
                mRomInfo.getBuild()));
        //mItems.add(new InfoItem(INFO_MBTOOL_VERSION,
        //        getString(R.string.rom_details_info_mbtool_version),
        //        "8.0.0.r1928.g2cb23c5"));
        mItems.add(new InfoItem(INFO_APPS_COUNTS,
                getString(R.string.rom_details_info_apps_counts),
                getString(R.string.rom_details_info_calculating)));
        mItems.add(new InfoItem(INFO_SYSTEM_PATH,
                getString(R.string.rom_details_info_system_path),
                mRomInfo.getSystemPath()));
        mItems.add(new InfoItem(INFO_CACHE_PATH,
                getString(R.string.rom_details_info_cache_path),
                mRomInfo.getCachePath()));
        mItems.add(new InfoItem(INFO_DATA_PATH,
                getString(R.string.rom_details_info_data_path),
                mRomInfo.getDataPath()));
        mItems.add(new InfoItem(INFO_SYSTEM_SIZE,
                getString(R.string.rom_details_info_system_size),
                getString(R.string.rom_details_info_calculating)));
        mItems.add(new InfoItem(INFO_CACHE_SIZE,
                getString(R.string.rom_details_info_cache_size),
                getString(R.string.rom_details_info_calculating)));
        mItems.add(new InfoItem(INFO_DATA_SIZE,
                getString(R.string.rom_details_info_data_size),
                getString(R.string.rom_details_info_calculating)));
    }

    private void populateActionItems() {
        mItems.add(new ActionItem(ACTION_SET_KERNEL,
                R.drawable.rom_details_action_set_kernel,
                getString(R.string.rom_menu_set_kernel)));
        mItems.add(new ActionItem(ACTION_BACKUP,
                R.drawable.rom_details_action_backup,
                getString(R.string.rom_menu_backup_rom)));
        mItems.add(new ActionItem(ACTION_ADD_TO_HOME_SCREEN,
                R.drawable.rom_details_action_home,
                getString(R.string.rom_menu_add_to_home_screen)));
        mItems.add(new ActionItem(ACTION_WIPE_ROM,
                R.drawable.rom_details_action_wipe,
                getString(R.string.rom_menu_wipe_rom)));
    }

    private void setInfoItem(int id, String newValue) {
        for (int i = 0; i < mItems.size(); i++) {
            Item item = mItems.get(i);
            if (item instanceof InfoItem) {
                InfoItem infoItem = (InfoItem) item;
                if (infoItem.id == id) {
                    infoItem.value = newValue;
                }
                mAdapter.notifyItemChanged(i);
            }
        }
    }

    private void reloadThumbnail() {
        for (int i = 0; i < mItems.size(); i++) {
            Item item = mItems.get(i);
            if (item instanceof RomCardItem) {
                mAdapter.notifyItemChanged(i);
            }
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.add(0, MENU_EDIT_NAME, Menu.NONE, R.string.rom_menu_edit_name);
        menu.add(0, MENU_CHANGE_IMAGE, Menu.NONE, R.string.rom_menu_change_image);
        menu.add(0, MENU_RESET_IMAGE, Menu.NONE, R.string.rom_menu_reset_image);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case android.R.id.home:
            finish();
            return true;
        case MENU_EDIT_NAME:
            onSelectedEditName();
            return true;
        case MENU_CHANGE_IMAGE:
            onSelectedChangeImage();
            return true;
        case MENU_RESET_IMAGE:
            onSelectedResetImage();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onActivityResult(int request, int result, Intent data) {
        switch (request) {
        case REQUEST_IMAGE:
            if (data != null && result == Activity.RESULT_OK) {
                new CacheRomThumbnailTask(getApplicationContext(), mRomInfo, data.getData(), this).execute();
            }
            break;
        }

        super.onActivityResult(request, result, data);
    }

    private void onSelectedEditName() {
        RomNameInputDialog d = RomNameInputDialog.newInstanceFromActivity(mRomInfo);
        d.show(getFragmentManager(), RomNameInputDialog.TAG);
    }

    private void onSelectedChangeImage() {
        Intent intent = new Intent();
        intent.setType("image/*");
        intent.setAction(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        startActivityForResult(intent, REQUEST_IMAGE);
    }

    private void onSelectedResetImage() {
        File f = new File(mRomInfo.getThumbnailPath());
        if (f.isFile()) {
            f.delete();
        }

        reloadThumbnail();
    }

    public void onSelectedAddToHomeScreen() {
        AddToHomeScreenOptionsDialog d =
                AddToHomeScreenOptionsDialog.newInstanceFromActivity(mRomInfo);
        d.show(getFragmentManager(), AddToHomeScreenOptionsDialog.TAG);
    }

    public void onSelectedSetKernel() {
        // Ask for confirmation
        if (mActiveRomId != null && !mActiveRomId.equals(mRomInfo.getId())) {
            ConfirmMismatchedSetKernelDialog d =
                    ConfirmMismatchedSetKernelDialog.newInstanceFromActivity(
                            mActiveRomId, mRomInfo.getId());
            d.show(getFragmentManager(), ConfirmMismatchedSetKernelDialog.TAG);
        } else {
            SetKernelConfirmDialog d = SetKernelConfirmDialog.newInstanceFromActivity(mRomInfo);
            d.show(getFragmentManager(), SetKernelConfirmDialog.TAG);
        }
    }

    public void onSelectedWipeRom() {
        if (mBootedRomInfo != null && mBootedRomInfo.getId().equals(mRomInfo.getId())) {
            createSnackbar(R.string.wipe_rom_no_wipe_current_rom, Snackbar.LENGTH_LONG).show();
        } else {
            WipeTargetsSelectionDialog d = WipeTargetsSelectionDialog.newInstanceFromActivity();
            d.show(getFragmentManager(), WipeTargetsSelectionDialog.TAG);
        }
    }

    @Override
    public void onRomNameChanged(String newName) {
        mRomInfo.setName(newName);

        new Thread() {
            @Override
            public void run() {
                RomUtils.saveConfig(mRomInfo);
            }
        }.start();

        updateTitle();
    }

    @Override
    public void onConfirmAddToHomeScreenOptions(RomInformation info, boolean reboot) {
        mTaskIdCreateLauncher = mService.createLauncher(info, reboot);
    }

    @Override
    public void onConfirmMismatchedSetKernel() {
        setKernel();
    }

    @Override
    public void onConfirmChecksumIssue(String romId) {
        switchRom(true);
    }

    @Override
    public void onConfirmSetKernel() {
        setKernel();
    }

    @Override
    public void onSelectedWipeTargets(short[] targets) {
        if (targets.length == 0) {
            createSnackbar(R.string.wipe_rom_none_selected, Snackbar.LENGTH_LONG).show();
            return;
        }

        wipeRom(targets);
    }

    @Override
    public void onRomThumbnailCached(RomInformation info) {
        reloadThumbnail();
    }

    @Override
    public void onActionItemSelected(ActionItem item) {
        switch (item.id) {
        case ACTION_SET_KERNEL:
            onSelectedSetKernel();
            break;
        case ACTION_BACKUP:
            createSnackbar("WIP :)", Snackbar.LENGTH_LONG).show();
            break;
        case ACTION_ADD_TO_HOME_SCREEN:
            onSelectedAddToHomeScreen();
            break;
        case ACTION_WIPE_ROM:
            onSelectedWipeRom();
            break;
        default:
            throw new IllegalStateException("Invalid action ID: " + item.id);
        }
    }

    private void onCachedWallpaper(CacheWallpaperResult result) {
        File wallpaperFile = new File(mRomInfo.getWallpaperPath());
        RequestCreator creator;

        if (result == CacheWallpaperResult.USES_LIVE_WALLPAPER) {
            Toast.makeText(this, "Cannot preview live wallpaper", Toast.LENGTH_SHORT).show();
        }

        if (result == CacheWallpaperResult.UP_TO_DATE || result == CacheWallpaperResult.UPDATED) {
            creator = Picasso.with(this).load(wallpaperFile).error(R.drawable.material);
        } else {
            creator = Picasso.with(this).load(R.drawable.material);
        }

        creator
                .transform(PaletteGeneratorTransformation.getInstance())
                .into(mWallpaper, new PaletteGeneratorCallback(mWallpaper) {
                    @Override
                    public void onObtainedPalette(Palette palette) {
                        if (palette == null) {
                            return;
                        }

                        Swatch swatch = palette.getMutedSwatch();
                        if (swatch == null) {
                            swatch = palette.getDarkVibrantSwatch();
                        }
                        if (swatch != null) {
                            // Status bar should be slightly darker
                            float[] hsl = swatch.getHsl();
                            hsl[2] -= 0.05;
                            if (hsl[2] < 0) {
                                hsl[2] = 0;
                            }

                            mCollapsing.setContentScrimColor(swatch.getRgb());
                            mCollapsing.setStatusBarScrimColor(ColorUtils.HSLToColor(hsl));
                            mFab.setBackgroundTintList(ColorStateList.valueOf(swatch.getRgb()));
                        }
                        //Swatch lightVibrant = palette.getLightVibrantSwatch();
                        //if (lightVibrant != null) {
                        //    collapsingToolbar.setCollapsedTitleTextColor(lightVibrant.getRgb());
                        //    collapsingToolbar.setExpandedTitleColor(lightVibrant.getRgb());
                        //}
                    }
                });
    }

    private void onSwitchedRom(SwitchRomResult result) {
        // Remove cached task from service
        removeCachedTaskId(mTaskIdSwitchRom);
        mTaskIdSwitchRom = -1;

        GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                .findFragmentByTag(GenericProgressDialog.TAG + PROGRESS_DIALOG_SWITCH_ROM);
        if (d != null) {
            d.dismiss();
        }

        switch (result) {
        case SUCCEEDED:
            createSnackbar(R.string.choose_rom_success, Snackbar.LENGTH_LONG).show();
            Log.d(TAG, "Prior cached boot partition ROM ID was: " + mActiveRomId);
            mActiveRomId = mRomInfo.getId();
            Log.d(TAG, "Changing cached boot partition ROM ID to: " + mActiveRomId);
            break;
        case FAILED:
            createSnackbar(R.string.choose_rom_failure, Snackbar.LENGTH_LONG).show();
            break;
        case CHECKSUM_INVALID:
            createSnackbar(R.string.choose_rom_checksums_invalid, Snackbar.LENGTH_LONG).show();
            showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_INVALID, mRomInfo.getId());
            break;
        case CHECKSUM_NOT_FOUND:
            createSnackbar(R.string.choose_rom_checksums_missing, Snackbar.LENGTH_LONG).show();
            showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_MISSING, mRomInfo.getId());
            break;
        case UNKNOWN_BOOT_PARTITION:
            showUnknownBootPartitionDialog();
            break;
        }
    }

    private void onSetKernel(SetKernelResult result) {
        // Remove cached task from service
        removeCachedTaskId(mTaskIdSetKernel);
        mTaskIdSetKernel = -1;

        GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                .findFragmentByTag(GenericProgressDialog.TAG + PROGRESS_DIALOG_SET_KERNEL);
        if (d != null) {
            d.dismiss();
        }

        switch (result) {
        case SUCCEEDED:
            createSnackbar(R.string.set_kernel_success, Snackbar.LENGTH_LONG).show();
            break;
        case FAILED:
            createSnackbar(R.string.set_kernel_failure, Snackbar.LENGTH_LONG).show();
            break;
        case UNKNOWN_BOOT_PARTITION:
            showUnknownBootPartitionDialog();
            break;
        }
    }

    private void onWipedRom(short[] succeeded, short[] failed) {
        // Remove cached task from service
        removeCachedTaskId(mTaskIdWipeRom);
        mTaskIdWipeRom = -1;

        GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                .findFragmentByTag(GenericProgressDialog.TAG + PROGRESS_DIALOG_WIPE_ROM);
        if (d != null) {
            d.dismiss();
        }

        if (succeeded == null || failed == null) {
            createSnackbar(R.string.wipe_rom_initiate_fail, Snackbar.LENGTH_LONG).show();
        } else {
            StringBuilder sb = new StringBuilder();

            if (succeeded.length > 0) {
                sb.append(String.format(getString(R.string.wipe_rom_successfully_wiped),
                        targetsToString(succeeded)));
                if (failed.length > 0) {
                    sb.append("\n");
                }
            }
            if (failed.length > 0) {
                sb.append(String.format(getString(R.string.wipe_rom_failed_to_wipe),
                        targetsToString(failed)));
            }

            createSnackbar(sb.toString(), Snackbar.LENGTH_LONG).show();
        }
    }

    private void onCreatedLauncher() {
        // Remove cached task from service
        removeCachedTaskId(mTaskIdCreateLauncher);
        mTaskIdCreateLauncher = -1;

        createSnackbar(String.format(getString(R.string.successfully_created_launcher),
                mRomInfo.getName()), Snackbar.LENGTH_LONG).show();
    }

    private void onHaveSystemSize(boolean success, long size) {
        if (success) {
            setInfoItem(INFO_SYSTEM_SIZE, FileUtils.toHumanReadableSize(this, size, 2));
        } else {
            setInfoItem(INFO_SYSTEM_SIZE, getString(R.string.unknown));
        }
    }

    private void onHaveCacheSize(boolean success, long size) {
        if (success) {
            setInfoItem(INFO_CACHE_SIZE, FileUtils.toHumanReadableSize(this, size, 2));
        } else {
            setInfoItem(INFO_CACHE_SIZE, getString(R.string.unknown));
        }
    }

    private void onHaveDataSize(boolean success, long size) {
        if (success) {
            setInfoItem(INFO_DATA_SIZE, FileUtils.toHumanReadableSize(this, size, 2));
        } else {
            setInfoItem(INFO_DATA_SIZE, getString(R.string.unknown));
        }
    }

    private void onHavePackagesCounts(boolean success, int systemPackages, int updatedPackages,
                                      int userPackages) {
        if (success) {
            setInfoItem(INFO_APPS_COUNTS, getString(R.string.rom_details_info_apps_counts_value,
                    systemPackages, updatedPackages, userPackages));
        } else {
            setInfoItem(INFO_APPS_COUNTS, getString(R.string.unknown));
        }
    }

    private void switchRom(boolean forceChecksumsUpdate) {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.switching_rom, R.string.please_wait);
        d.show(getFragmentManager(), GenericProgressDialog.TAG + PROGRESS_DIALOG_SWITCH_ROM);

        mTaskIdSwitchRom = mService.switchRom(mRomInfo.getId(), forceChecksumsUpdate);
    }

    private void setKernel() {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.setting_kernel, R.string.please_wait);
        d.show(getFragmentManager(), GenericProgressDialog.TAG + PROGRESS_DIALOG_SET_KERNEL);

        mTaskIdSetKernel = mService.setKernel(mRomInfo.getId());
    }

    private void wipeRom(short[] targets) {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.wiping_targets, R.string.please_wait);
        d.show(getFragmentManager(), GenericProgressDialog.TAG + PROGRESS_DIALOG_WIPE_ROM);

        mTaskIdWipeRom = mService.wipeRom(mRomInfo.getId(), targets);
    }

    private void showChecksumIssueDialog(int issue, String romId) {
        ConfirmChecksumIssueDialog d =
                ConfirmChecksumIssueDialog.newInstanceFromActivity(issue, romId);
        d.show(getFragmentManager(), ConfirmChecksumIssueDialog.TAG);
    }

    private void showUnknownBootPartitionDialog() {
        String codename = RomUtils.getDeviceCodename(this);
        String message = String.format(getString(R.string.unknown_boot_partition), codename);

        GenericConfirmDialog gcd = GenericConfirmDialog.newInstance(null, message);
        gcd.show(getFragmentManager(), GenericConfirmDialog.TAG);
    }

    private Snackbar createSnackbar(String text, int duration) {
        Snackbar snackbar = Snackbar.make(mCoordinator, text, duration);
        snackbarSetTextColor(snackbar);
        return snackbar;
    }

    private Snackbar createSnackbar(@StringRes int resId, int duration) {
        Snackbar snackbar = Snackbar.make(mCoordinator, resId, duration);
        snackbarSetTextColor(snackbar);
        return snackbar;
    }

    private void snackbarSetTextColor(Snackbar snackbar) {
        View view = snackbar.getView();
        TextView textView = (TextView) view.findViewById(android.support.design.R.id.snackbar_text);
        textView.setTextColor(ContextCompat.getColor(this, R.color.text_color_primary));
    }

    private String targetsToString(short[] targets) {
        StringBuilder sb = new StringBuilder();

        for (int i = 0; i < targets.length; i++) {
            if (i != 0) {
                sb.append(", ");
            }

            if (targets[i] == MbWipeTarget.SYSTEM) {
                sb.append(getString(R.string.wipe_target_system));
            } else if (targets[i] == MbWipeTarget.CACHE) {
                sb.append(getString(R.string.wipe_target_cache));
            } else if (targets[i] == MbWipeTarget.DATA) {
                sb.append(getString(R.string.wipe_target_data));
            } else if (targets[i] == MbWipeTarget.DALVIK_CACHE) {
                sb.append(getString(R.string.wipe_target_dalvik_cache));
            } else if (targets[i] == MbWipeTarget.MULTIBOOT) {
                sb.append(getString(R.string.wipe_target_multiboot_files));
            }
        }

        return sb.toString();
    }

    private class SwitcherEventReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();

            // If we're not yet connected to the service, don't do anything. The state will be
            // restored upon connection.
            if (mService == null) {
                return;
            }

            if (!SERVICE_INTENT_FILTER.hasAction(action)) {
                return;
            }

            int taskId = intent.getIntExtra(SwitcherService.RESULT_TASK_ID, -1);

            if (SwitcherService.ACTION_CACHED_WALLPAPER.equals(action)
                    && taskId == mTaskIdCacheWallpaper) {
                CacheWallpaperResult result = (CacheWallpaperResult) intent.getSerializableExtra(
                        SwitcherService.RESULT_CACHED_WALLPAPER_RESULT);
                onCachedWallpaper(result);
            } else if (SwitcherService.ACTION_SWITCHED_ROM.equals(action)
                    && taskId == mTaskIdSwitchRom) {
                SwitchRomResult result = (SwitchRomResult) intent.getSerializableExtra(
                        SwitcherService.RESULT_SWITCHED_ROM_RESULT);
                onSwitchedRom(result);
            } else if (SwitcherService.ACTION_SET_KERNEL.equals(action)
                    && taskId == mTaskIdSetKernel) {
                SetKernelResult result = (SetKernelResult) intent.getSerializableExtra(
                        SwitcherService.RESULT_SET_KERNEL_RESULT);
                onSetKernel(result);
            } else if (SwitcherService.ACTION_WIPED_ROM.equals(action)
                    && taskId == mTaskIdWipeRom) {
                short[] succeeded = intent.getShortArrayExtra(
                        SwitcherService.RESULT_WIPED_ROM_SUCCEEDED);
                short[] failed = intent.getShortArrayExtra(
                        SwitcherService.RESULT_WIPED_ROM_FAILED);
                onWipedRom(succeeded, failed);
            } else if (SwitcherService.ACTION_CREATED_LAUNCHER.equals(action)
                    && taskId == mTaskIdCreateLauncher) {
                onCreatedLauncher();
            } else if (SwitcherService.ACTION_ROM_DETAILS_GOT_SYSTEM_SIZE.equals(action)
                    && taskId == mTaskIdGetRomDetails) {
                boolean success = intent.getBooleanExtra(
                        SwitcherService.RESULT_ROM_DETAILS_SUCCESS, false);
                long size = intent.getLongExtra(
                        SwitcherService.RESULT_ROM_DETAILS_SIZE, -1);
                onHaveSystemSize(success, size);
            } else if (SwitcherService.ACTION_ROM_DETAILS_GOT_CACHE_SIZE.equals(action)
                    && taskId == mTaskIdGetRomDetails) {
                boolean success = intent.getBooleanExtra(
                        SwitcherService.RESULT_ROM_DETAILS_SUCCESS, false);
                long size = intent.getLongExtra(
                        SwitcherService.RESULT_ROM_DETAILS_SIZE, -1);
                onHaveCacheSize(success, size);
            } else if (SwitcherService.ACTION_ROM_DETAILS_GOT_DATA_SIZE.equals(action)
                    && taskId == mTaskIdGetRomDetails) {
                boolean success = intent.getBooleanExtra(
                        SwitcherService.RESULT_ROM_DETAILS_SUCCESS, false);
                long size = intent.getLongExtra(
                        SwitcherService.RESULT_ROM_DETAILS_SIZE, -1);
                onHaveDataSize(success, size);
            } else if (SwitcherService.ACTION_ROM_DETAILS_GOT_PACKAGES_COUNTS.equals(action)
                    && taskId == mTaskIdGetRomDetails) {
                boolean success = intent.getBooleanExtra(
                        SwitcherService.RESULT_ROM_DETAILS_SUCCESS, false);
                int systemPackages = intent.getIntExtra(
                        SwitcherService.RESULT_ROM_DETAILS_PACKAGES_COUNT_SYSTEM, -1);
                int updatedPackages = intent.getIntExtra(
                        SwitcherService.RESULT_ROM_DETAILS_PACKAGES_COUNT_UPDATED, -1);
                int userPackages = intent.getIntExtra(
                        SwitcherService.RESULT_ROM_DETAILS_PACKAGES_COUNT_USER, -1);
                onHavePackagesCounts(success, systemPackages, updatedPackages, userPackages);
            } else if (SwitcherService.ACTION_ROM_DETAILS_FINISHED.equals(action)
                    && taskId == mTaskIdGetRomDetails) {
                // Ignore
            }
        }
    }
}