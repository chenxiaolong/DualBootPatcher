/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.res.ColorStateList;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.support.annotation.StringRes;
import android.support.design.widget.CollapsingToolbarLayout;
import android.support.design.widget.CoordinatorLayout;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
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
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.Constants;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.CacheWallpaperResult;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.SnackbarUtils;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.picasso.PaletteGeneratorCallback;
import com.github.chenxiaolong.dualbootpatcher.picasso.PaletteGeneratorTransformation;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolErrorActivity;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SwitchRomResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.AddToHomeScreenOptionsDialog
        .AddToHomeScreenOptionsDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.BackupNameInputDialog.BackupNameInputDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.BackupRestoreTargetsSelectionDialog
        .BackupRestoreTargetsSelectionDialogListener;
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
import com.github.chenxiaolong.dualbootpatcher.switcher.UpdateRamdiskResultDialog
        .UpdateRamdiskResultDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.WipeTargetsSelectionDialog
        .WipeTargetsSelectionDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams.Action;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CacheWallpaperTask
        .CacheWallpaperTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateLauncherTask
        .CreateLauncherTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomDetailsTask
        .GetRomDetailsTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask.SetKernelTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask.SwitchRomTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.UpdateRamdiskTask
        .UpdateRamdiskTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.WipeRomTask.WipeRomTaskListener;
import com.squareup.picasso.Picasso;
import com.squareup.picasso.RequestCreator;

import java.io.File;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Locale;

import mbtool.daemon.v3.MbWipeTarget;

public class RomDetailActivity extends AppCompatActivity implements
        RomNameInputDialogListener,
        AddToHomeScreenOptionsDialogListener,
        CacheRomThumbnailTaskListener,
        RomDetailAdapterListener,
        ConfirmMismatchedSetKernelDialogListener,
        SetKernelConfirmDialogListener,
        UpdateRamdiskResultDialogListener,
        WipeTargetsSelectionDialogListener,
        ConfirmChecksumIssueDialogListener,
        BackupRestoreTargetsSelectionDialogListener,
        BackupNameInputDialogListener,
        ServiceConnection {
    private static final String TAG = RomDetailActivity.class.getSimpleName();

    private static final int MENU_EDIT_NAME = Menu.FIRST;
    private static final int MENU_CHANGE_IMAGE = Menu.FIRST + 1;
    private static final int MENU_RESET_IMAGE = Menu.FIRST + 2;

    private static final int INFO_SLOT = 1;
    private static final int INFO_VERSION = 2;
    private static final int INFO_BUILD = 3;
    private static final int INFO_MBTOOL_STATUS = 4;
    private static final int INFO_APPS_COUNTS = 5;
    private static final int INFO_SYSTEM_PATH = 6;
    private static final int INFO_CACHE_PATH = 7;
    private static final int INFO_DATA_PATH = 8;
    private static final int INFO_SYSTEM_SIZE = 9;
    private static final int INFO_CACHE_SIZE = 10;
    private static final int INFO_DATA_SIZE = 11;

    private static final int ACTION_UPDATE_RAMDISK = 1;
    private static final int ACTION_SET_KERNEL = 2;
    private static final int ACTION_BACKUP = 3;
    private static final int ACTION_ADD_TO_HOME_SCREEN = 4;
    private static final int ACTION_WIPE_ROM = 5;

    private static final String PROGRESS_DIALOG_SWITCH_ROM =
            RomDetailActivity.class.getCanonicalName() + ".progress.switch_rom";
    private static final String PROGRESS_DIALOG_SET_KERNEL =
            RomDetailActivity.class.getCanonicalName() + ".progress.set_kernel";
    private static final String PROGRESS_DIALOG_UPDATE_RAMDISK =
            RomDetailActivity.class.getCanonicalName() + ".progress.update_ramdisk";
    private static final String PROGRESS_DIALOG_WIPE_ROM =
            RomDetailActivity.class.getCanonicalName() + ".progress.wipe_rom";
    private static final String PROGRESS_DIALOG_REBOOT =
            RomDetailActivity.class.getCanonicalName() + ".progress.reboot";
    private static final String CONFIRM_DIALOG_UPDATED_RAMDISK =
            RomDetailActivity.class.getCanonicalName() + ".confirm.updated_ramdisk";
    private static final String CONFIRM_DIALOG_ADD_TO_HOME_SCREEN =
            RomDetailActivity.class.getCanonicalName() + ".confirm.add_to_home_screen";
    private static final String CONFIRM_DIALOG_ROM_NAME =
            RomDetailActivity.class.getCanonicalName() + ".confirm.rom_name";
    private static final String CONFIRM_DIALOG_SET_KERNEL =
            RomDetailActivity.class.getCanonicalName() + ".confirm.set_kernel";
    private static final String CONFIRM_DIALOG_MISMATCHED_KERNEL =
            RomDetailActivity.class.getCanonicalName() + ".confirm.mismatched_kernel";
    private static final String CONFIRM_DIALOG_BACKUP_RESTORE_TARGETS =
            RomDetailActivity.class.getCanonicalName() + ".confirm.backup_restore_targets";
    private static final String CONFIRM_DIALOG_WIPE_TARGETS =
            RomDetailActivity.class.getCanonicalName() + ".confirm.wipe_targets";
    private static final String CONFIRM_DIALOG_CHECKSUM_ISSUE =
            RomDetailActivity.class.getCanonicalName() + ".confirm.checksum_issue";
    private static final String CONFIRM_DIALOG_UNKNOWN_BOOT_PARTITION =
            RomDetailActivity.class.getCanonicalName() + ".confirm.unknown_boot_partition";
    private static final String INPUT_DIALOG_BACKUP_NAME =
            RomDetailActivity.class.getCanonicalName() + ".input.backup_name";

    private static final int REQUEST_IMAGE = 1234;
    private static final int REQUEST_MBTOOL_ERROR = 2345;

    // Argument extras
    public static final String EXTRA_ROM_INFO = "rom_info";
    public static final String EXTRA_BOOTED_ROM_INFO = "booted_rom_info";
    public static final String EXTRA_ACTIVE_ROM_ID = "active_rom_id";
    // Saved state extras
    private static final String EXTRA_STATE_TASK_ID_CACHE_WALLPAPER = "state.cache_wallpaper";
    private static final String EXTRA_STATE_TASK_ID_SWITCH_ROM = "state.switch_rom";
    private static final String EXTRA_STATE_TASK_ID_SET_KERNEL = "state.set_kernel";
    private static final String EXTRA_STATE_TASK_ID_UPDATE_RAMDISK = "state.update_ramdisk";
    private static final String EXTRA_STATE_TASK_ID_WIPE_ROM = "state.wipe_rom";
    private static final String EXTRA_STATE_TASK_ID_CREATE_LAUNCHER = "state.create_launcher";
    private static final String EXTRA_STATE_TASK_ID_GET_ROM_DETAILS = "state.get_rom_details";
    private static final String EXTRA_STATE_TASK_IDS_TO_REMOVE = "state.task_ids_to_reomve";
    private static final String EXTRA_STATE_RESULT_INTENT = "state.result_intent";
    private static final String EXTRA_STATE_BACKUP_TARGETS = "state.backup_targets";
    // Result extras
    public static final String EXTRA_RESULT_WIPED_ROM = "result.wiped_rom";

    private CoordinatorLayout mCoordinator;
    private CollapsingToolbarLayout mCollapsing;
    private ImageView mWallpaper;
    private FloatingActionButton mFab;
    private RecyclerView mRV;

    private RomInformation mRomInfo;
    private RomInformation mBootedRomInfo;
    private String mActiveRomId;

    private String[] mBackupTargets;

    private ArrayList<Item> mItems = new ArrayList<>();
    private RomDetailAdapter mAdapter;

    private static final int FORCE_RAMDISK_UPDATE_TAPS = 7;
    private int mUpdateRamdiskCountdown;
    private Toast mUpdateRamdiskToast;
    private int mUpdateRamdiskResId;

    private int mTaskIdCacheWallpaper = -1;
    private int mTaskIdSwitchRom = -1;
    private int mTaskIdSetKernel = -1;
    private int mTaskIdUpdateRamdisk = -1;
    private int mTaskIdWipeRom = -1;
    private int mTaskIdCreateLauncher = -1;
    private int mTaskIdGetRomDetails = -1;

    /** Task IDs to remove */
    private ArrayList<Integer> mTaskIdsToRemove = new ArrayList<>();

    /** Switcher service */
    private SwitcherService mService;
    /** Callback for events from the service */
    private final SwitcherEventCallback mCallback = new SwitcherEventCallback();

    /** Handler for processing events from the service on the UI thread */
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    /** Result intent */
    private Intent mResultIntent;

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
            mTaskIdUpdateRamdisk = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_UPDATE_RAMDISK);
            mTaskIdWipeRom = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_WIPE_ROM);
            mTaskIdCreateLauncher = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_CREATE_LAUNCHER);
            mTaskIdGetRomDetails = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_GET_ROM_DETAILS);
            mTaskIdsToRemove = savedInstanceState.getIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE);
            mResultIntent = savedInstanceState.getParcelable(EXTRA_STATE_RESULT_INTENT);
            setResult(RESULT_OK, mResultIntent);
            mBackupTargets = savedInstanceState.getStringArray(EXTRA_STATE_BACKUP_TARGETS);
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
        updateMbtoolStatus();

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
        outState.putInt(EXTRA_STATE_TASK_ID_UPDATE_RAMDISK, mTaskIdUpdateRamdisk);
        outState.putInt(EXTRA_STATE_TASK_ID_WIPE_ROM, mTaskIdWipeRom);
        outState.putInt(EXTRA_STATE_TASK_ID_CREATE_LAUNCHER, mTaskIdCreateLauncher);
        outState.putInt(EXTRA_STATE_TASK_ID_GET_ROM_DETAILS, mTaskIdGetRomDetails);
        outState.putIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE, mTaskIdsToRemove);
        outState.putParcelable(EXTRA_STATE_RESULT_INTENT, mResultIntent);
        if (mBackupTargets != null) {
            outState.putStringArray(EXTRA_STATE_BACKUP_TARGETS, mBackupTargets);
        }
    }

    @Override
    protected void onStart() {
        super.onStart();

        // Start and bind to the service
        Intent intent = new Intent(this, SwitcherService.class);
        bindService(intent, this, BIND_AUTO_CREATE);
        startService(intent);
    }

    @Override
    protected void onStop() {
        super.onStop();

        if (isFinishing()) {
            if (mTaskIdCacheWallpaper >= 0) {
                removeCachedTaskId(mTaskIdCacheWallpaper);
                mTaskIdCacheWallpaper = -1;
            }
            if (mTaskIdGetRomDetails >= 0) {
                removeCachedTaskId(mTaskIdGetRomDetails);
                mTaskIdGetRomDetails = -1;
            }
        }

        // If we connected to the service and registered the callback, now we unregister it
        if (mService != null) {
            if (mTaskIdCacheWallpaper >= 0) {
                mService.removeCallback(mTaskIdCacheWallpaper, mCallback);
            }
            if (mTaskIdSwitchRom >= 0) {
                mService.removeCallback(mTaskIdSwitchRom, mCallback);
            }
            if (mTaskIdSetKernel >= 0) {
                mService.removeCallback(mTaskIdSetKernel, mCallback);
            }
            if (mTaskIdUpdateRamdisk >= 0) {
                mService.removeCallback(mTaskIdUpdateRamdisk, mCallback);
            }
            if (mTaskIdWipeRom >= 0) {
                mService.removeCallback(mTaskIdWipeRom, mCallback);
            }
            if (mTaskIdCreateLauncher >= 0) {
                mService.removeCallback(mTaskIdCreateLauncher, mCallback);
            }
            if (mTaskIdGetRomDetails >= 0) {
                mService.removeCallback(mTaskIdGetRomDetails, mCallback);
            }
        }

        // Unbind from our service
        unbindService(this);
        mService = null;

        // At this point, the mCallback will not get called anymore by the service. Now we just need
        // to remove all pending Runnables that were posted to mHandler.
        mHandler.removeCallbacksAndMessages(null);
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
            mService.addCallback(mTaskIdCacheWallpaper, mCallback);
            mService.enqueueTaskId(mTaskIdCacheWallpaper);
        } else {
            mService.addCallback(mTaskIdCacheWallpaper, mCallback);
        }

        if (mTaskIdGetRomDetails < 0) {
            mTaskIdGetRomDetails = mService.getRomDetails(mRomInfo);
            mService.addCallback(mTaskIdGetRomDetails, mCallback);
            mService.enqueueTaskId(mTaskIdGetRomDetails);
        } else {
            mService.addCallback(mTaskIdGetRomDetails, mCallback);
        }

        if (mTaskIdCreateLauncher >= 0) {
            mService.addCallback(mTaskIdCreateLauncher, mCallback);
        }

        if (mTaskIdSwitchRom >= 0) {
            mService.addCallback(mTaskIdSwitchRom, mCallback);
        }

        if (mTaskIdSetKernel >= 0) {
            mService.addCallback(mTaskIdSetKernel, mCallback);
        }

        if (mTaskIdUpdateRamdisk >= 0) {
            mService.addCallback(mTaskIdUpdateRamdisk, mCallback);
        }

        if (mTaskIdWipeRom >= 0) {
            mService.addCallback(mTaskIdWipeRom, mCallback);
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

    private void updateMbtoolStatus() {
        // There is currently no way to determine the mbtool version of other ROMs
        if (mBootedRomInfo != null && mBootedRomInfo.getId().equals(mRomInfo.getId())) {
            Version minAppSharing = MbtoolUtils.getMinimumRequiredVersion(Feature.APP_SHARING);
            Version minDaemon = MbtoolUtils.getMinimumRequiredVersion(Feature.DAEMON);
            Version version = MbtoolUtils.getSystemMbtoolVersion(this);

            if (version.compareTo(minAppSharing) >= 0 && version.compareTo(minDaemon) >= 0) {
                mUpdateRamdiskCountdown = FORCE_RAMDISK_UPDATE_TAPS;
                mUpdateRamdiskResId = R.string.update_ramdisk_up_to_date_desc;
            } else {
                mUpdateRamdiskCountdown = -1;
                if (version.equals(Version.from("0.0.0"))) {
                    mUpdateRamdiskResId = R.string.update_ramdisk_missing_desc;
                } else {
                    mUpdateRamdiskResId = R.string.update_ramdisk_outdated_desc;
                }
            }
        } else {
            mUpdateRamdiskCountdown = -1;
            mUpdateRamdiskResId = R.string.unknown;
        }
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
        mItems.add(new InfoItem(INFO_MBTOOL_STATUS,
                getString(R.string.rom_details_info_mbtool_status),
                getString(mUpdateRamdiskResId)));
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
        mItems.add(new ActionItem(ACTION_UPDATE_RAMDISK,
                R.drawable.rom_details_action_update_ramdisk,
                getString(R.string.rom_menu_update_ramdisk)));
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
            if (data != null && result == RESULT_OK) {
                new CacheRomThumbnailTask(getApplicationContext(), mRomInfo, data.getData(), this).execute();
            }
            break;
        case REQUEST_MBTOOL_ERROR:
            if (data != null && result == RESULT_OK) {
                finish();
            }
            break;
        default:
            super.onActivityResult(request, result, data);
            break;
        }
    }

    private void onSelectedEditName() {
        RomNameInputDialog d = RomNameInputDialog.newInstanceFromActivity(mRomInfo);
        d.show(getFragmentManager(), CONFIRM_DIALOG_ROM_NAME);
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
        d.show(getFragmentManager(), CONFIRM_DIALOG_ADD_TO_HOME_SCREEN);
    }

    public void onSelectedUpdateRamdisk() {
        // Act like the Android settings app :)
        if (mUpdateRamdiskCountdown > 0) {
            if (mUpdateRamdiskCountdown == FORCE_RAMDISK_UPDATE_TAPS) {
                createSnackbar(mUpdateRamdiskResId, Snackbar.LENGTH_LONG).show();
            }

            mUpdateRamdiskCountdown--;
            if (mUpdateRamdiskCountdown == 0) {
                if (mUpdateRamdiskToast != null) {
                    mUpdateRamdiskToast.cancel();
                }
                mUpdateRamdiskToast = Toast.makeText(this, R.string.force_updating_ramdisk,
                        Toast.LENGTH_LONG);
                mUpdateRamdiskToast.show();

                updateRamdisk();

                mUpdateRamdiskCountdown = FORCE_RAMDISK_UPDATE_TAPS;
            } else if (mUpdateRamdiskCountdown > 0
                    && mUpdateRamdiskCountdown < (FORCE_RAMDISK_UPDATE_TAPS - 2)) {
                if (mUpdateRamdiskToast != null) {
                    mUpdateRamdiskToast.cancel();
                }
                mUpdateRamdiskToast = Toast.makeText(this, getResources().getQuantityString(
                        R.plurals.force_update_ramdisk_countdown,
                        mUpdateRamdiskCountdown, mUpdateRamdiskCountdown),
                        Toast.LENGTH_SHORT);
                mUpdateRamdiskToast.show();
            }
        } else if (mUpdateRamdiskCountdown < 0) {
            // Already enabled
            if (mUpdateRamdiskToast != null) {
                mUpdateRamdiskToast.cancel();
            }

            updateRamdisk();
        }
    }

    public void onSelectedSetKernel() {
        // Ask for confirmation
        if (mActiveRomId != null && !mActiveRomId.equals(mRomInfo.getId())) {
            ConfirmMismatchedSetKernelDialog d =
                    ConfirmMismatchedSetKernelDialog.newInstanceFromActivity(
                            mActiveRomId, mRomInfo.getId());
            d.show(getFragmentManager(), CONFIRM_DIALOG_MISMATCHED_KERNEL);
        } else {
            SetKernelConfirmDialog d = SetKernelConfirmDialog.newInstanceFromActivity(mRomInfo);
            d.show(getFragmentManager(), CONFIRM_DIALOG_SET_KERNEL);
        }
    }

    public void onSelectedBackupRom() {
        BackupRestoreTargetsSelectionDialog d =
                BackupRestoreTargetsSelectionDialog.newInstanceFromActivity(Action.BACKUP);
        d.show(getFragmentManager(), CONFIRM_DIALOG_BACKUP_RESTORE_TARGETS);
    }

    public void onSelectedWipeRom() {
        if (mBootedRomInfo != null && mBootedRomInfo.getId().equals(mRomInfo.getId())) {
            createSnackbar(R.string.wipe_rom_no_wipe_current_rom, Snackbar.LENGTH_LONG).show();
        } else {
            WipeTargetsSelectionDialog d = WipeTargetsSelectionDialog.newInstanceFromActivity();
            d.show(getFragmentManager(), CONFIRM_DIALOG_WIPE_TARGETS);
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
        mService.addCallback(mTaskIdCreateLauncher, mCallback);
        mService.enqueueTaskId(mTaskIdCreateLauncher);
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
    public void onConfirmUpdatedRamdisk(boolean reboot) {
        if (reboot) {
            GenericProgressDialog progressDialog = GenericProgressDialog.newInstance(
                    0, R.string.please_wait);
            progressDialog.show(getFragmentManager(), PROGRESS_DIALOG_REBOOT);

            new Thread() {
                @Override
                public void run() {
                    SwitcherUtils.reboot(getApplicationContext());
                }
            }.start();
        }
    }

    @Override
    public void onSelectedBackupRestoreTargets(String[] targets) {
        if (targets.length == 0) {
            createSnackbar(R.string.br_no_target_selected, Snackbar.LENGTH_LONG).show();
            return;
        }

        mBackupTargets = targets;

        DateFormat df = new SimpleDateFormat("yyyy.MM.dd-HH.mm.ss", Locale.US);
        String suggestedName = df.format(new Date()) + '_' + mRomInfo.getId();

        // Prompt for back up name
        BackupNameInputDialog d = BackupNameInputDialog.newInstanceFromActivity(suggestedName);
        d.show(getFragmentManager(), INPUT_DIALOG_BACKUP_NAME);
    }

    @Override
    public void onSelectedBackupName(String name) {
        SharedPreferences prefs = getSharedPreferences("settings", 0);

        String backupDir = prefs.getString(
                Constants.Preferences.BACKUP_DIRECTORY, Constants.Defaults.BACKUP_DIRECTORY);

        BackupRestoreParams params = new BackupRestoreParams(
                Action.BACKUP, mRomInfo.getId(), mBackupTargets, name, backupDir, false);
        MbtoolAction action = new MbtoolAction(params);

        // Start backup
        Intent intent = new Intent(this, MbtoolTaskOutputActivity.class);
        intent.putExtra(MbtoolTaskOutputFragment.PARAM_ACTIONS, new MbtoolAction[] { action });
        startActivity(intent);
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
        case ACTION_UPDATE_RAMDISK:
            onSelectedUpdateRamdisk();
            break;
        case ACTION_SET_KERNEL:
            onSelectedSetKernel();
            break;
        case ACTION_BACKUP:
            onSelectedBackupRom();
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
                .findFragmentByTag(PROGRESS_DIALOG_SWITCH_ROM);
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
                .findFragmentByTag(PROGRESS_DIALOG_SET_KERNEL);
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

    private void onUpdatedRamdisk(boolean success) {
        // Remove cached task from service
        removeCachedTaskId(mTaskIdUpdateRamdisk);
        mTaskIdUpdateRamdisk = -1;

        GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                .findFragmentByTag(PROGRESS_DIALOG_UPDATE_RAMDISK);
        if (d != null) {
            d.dismiss();
        }

        boolean isCurrentRom = mBootedRomInfo != null &&
                mBootedRomInfo.getId().equals(mRomInfo.getId());
        UpdateRamdiskResultDialog dialog =
                UpdateRamdiskResultDialog.newInstanceFromActivity(success, isCurrentRom);
        dialog.show(getFragmentManager(), CONFIRM_DIALOG_UPDATED_RAMDISK);
    }

    private void onWipedRom(short[] succeeded, short[] failed) {
        // Remove cached task from service
        removeCachedTaskId(mTaskIdWipeRom);
        mTaskIdWipeRom = -1;

        GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                .findFragmentByTag(PROGRESS_DIALOG_WIPE_ROM);
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

        // Alert parent activity
        if (mResultIntent == null) {
            mResultIntent = new Intent();
        }
        mResultIntent.putExtra(EXTRA_RESULT_WIPED_ROM, true);
        setResult(RESULT_OK, mResultIntent);
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
        d.show(getFragmentManager(), PROGRESS_DIALOG_SWITCH_ROM);

        mTaskIdSwitchRom = mService.switchRom(mRomInfo.getId(), forceChecksumsUpdate);
        mService.addCallback(mTaskIdSwitchRom, mCallback);
        mService.enqueueTaskId(mTaskIdSwitchRom);
    }

    private void setKernel() {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.setting_kernel, R.string.please_wait);
        d.show(getFragmentManager(), PROGRESS_DIALOG_SET_KERNEL);

        mTaskIdSetKernel = mService.setKernel(mRomInfo.getId());
        mService.addCallback(mTaskIdSetKernel, mCallback);
        mService.enqueueTaskId(mTaskIdSetKernel);
    }

    private void updateRamdisk() {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                0, R.string.please_wait);
        d.show(getFragmentManager(), PROGRESS_DIALOG_UPDATE_RAMDISK);

        mTaskIdUpdateRamdisk = mService.updateRamdisk(mRomInfo);
        mService.addCallback(mTaskIdUpdateRamdisk, mCallback);
        mService.enqueueTaskId(mTaskIdUpdateRamdisk);
    }

    private void wipeRom(short[] targets) {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.wiping_targets, R.string.please_wait);
        d.show(getFragmentManager(), PROGRESS_DIALOG_WIPE_ROM);

        mTaskIdWipeRom = mService.wipeRom(mRomInfo.getId(), targets);
        mService.addCallback(mTaskIdWipeRom, mCallback);
        mService.enqueueTaskId(mTaskIdWipeRom);
    }

    private void showChecksumIssueDialog(int issue, String romId) {
        ConfirmChecksumIssueDialog d =
                ConfirmChecksumIssueDialog.newInstanceFromActivity(issue, romId);
        d.show(getFragmentManager(), CONFIRM_DIALOG_CHECKSUM_ISSUE);
    }

    private void showUnknownBootPartitionDialog() {
        String codename = RomUtils.getDeviceCodename(this);
        String message = String.format(getString(R.string.unknown_boot_partition), codename);

        GenericConfirmDialog gcd = GenericConfirmDialog.newInstanceFromFragment(
                null, -1, null, message, null);
        gcd.show(getFragmentManager(), CONFIRM_DIALOG_UNKNOWN_BOOT_PARTITION);
    }

    private Snackbar createSnackbar(String text, int duration) {
        return SnackbarUtils.createSnackbar(this, mCoordinator, text, duration);
    }

    private Snackbar createSnackbar(@StringRes int resId, int duration) {
        return SnackbarUtils.createSnackbar(this, mCoordinator, resId, duration);
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

    private class SwitcherEventCallback implements CacheWallpaperTaskListener,
            SwitchRomTaskListener, SetKernelTaskListener, UpdateRamdiskTaskListener,
            WipeRomTaskListener, CreateLauncherTaskListener, GetRomDetailsTaskListener {
        @Override
        public void onCachedWallpaper(int taskId, RomInformation romInfo,
                                      final CacheWallpaperResult result) {
            if (taskId == mTaskIdCacheWallpaper) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        RomDetailActivity.this.onCachedWallpaper(result);
                    }
                });
            }
        }

        @Override
        public void onSwitchedRom(int taskId, String romId, final SwitchRomResult result) {
            if (taskId == mTaskIdSwitchRom) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        RomDetailActivity.this.onSwitchedRom(result);
                    }
                });
            }
        }

        @Override
        public void onSetKernel(int taskId, String romId, final SetKernelResult result) {
            if (taskId == mTaskIdSetKernel) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        RomDetailActivity.this.onSetKernel(result);
                    }
                });
            }
        }

        @Override
        public void onUpdatedRamdisk(int taskId, RomInformation romInfo, final boolean success) {
            if (taskId == mTaskIdUpdateRamdisk) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        RomDetailActivity.this.onUpdatedRamdisk(success);
                    }
                });
            }
        }

        @Override
        public void onWipedRom(int taskId, String romId, final short[] succeeded, final short[] failed) {
            if (taskId == mTaskIdWipeRom) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        RomDetailActivity.this.onWipedRom(succeeded, failed);
                    }
                });
            }
        }

        @Override
        public void onCreatedLauncher(int taskId, RomInformation romInfo) {
            if (taskId == mTaskIdCreateLauncher) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        RomDetailActivity.this.onCreatedLauncher();
                    }
                });
            }
        }

        @Override
        public void onRomDetailsGotSystemSize(int taskId, RomInformation romInfo,
                                              final boolean success, final long size) {
            if (taskId == mTaskIdGetRomDetails) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onHaveSystemSize(success, size);
                    }
                });
            }
        }

        @Override
        public void onRomDetailsGotCacheSize(int taskId, RomInformation romInfo,
                                             final boolean success, final long size) {
            if (taskId == mTaskIdGetRomDetails) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onHaveCacheSize(success, size);
                    }
                });
            }
        }

        @Override
        public void onRomDetailsGotDataSize(int taskId, RomInformation romInfo,
                                            final boolean success, final long size) {
            if (taskId == mTaskIdGetRomDetails) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onHaveDataSize(success, size);
                    }
                });
            }
        }

        @Override
        public void onRomDetailsGotPackagesCounts(int taskId, RomInformation romInfo,
                                                  final boolean success, final int systemPackages,
                                                  final int updatedPackages,
                                                  final int userPackages) {
            if (taskId == mTaskIdGetRomDetails) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onHavePackagesCounts(
                                success, systemPackages, updatedPackages, userPackages);
                    }
                });
            }
        }

        @Override
        public void onRomDetailsFinished(int taskId, RomInformation romInfo) {
            // Ignore
        }

        @Override
        public void onMbtoolConnectionFailed(int taskId, final Reason reason) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    Intent intent = new Intent(RomDetailActivity.this, MbtoolErrorActivity.class);
                    intent.putExtra(MbtoolErrorActivity.EXTRA_REASON, reason);
                    startActivityForResult(intent, REQUEST_MBTOOL_ERROR);
                }
            });
        }
    }
}