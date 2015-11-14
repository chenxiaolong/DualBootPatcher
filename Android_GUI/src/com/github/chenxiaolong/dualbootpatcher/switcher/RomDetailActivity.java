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
import android.app.FragmentManager;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.StringRes;
import android.support.design.widget.AppBarLayout;
import android.support.design.widget.CollapsingToolbarLayout;
import android.support.design.widget.CoordinatorLayout;
import android.support.design.widget.FloatingActionButton;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
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

import com.github.chenxiaolong.dualbootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
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
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.CreatedLauncherEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.SetKernelEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.SwitchedRomEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.WipedRomEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.WipeTargetsSelectionDialog
        .WipeTargetsSelectionDialogListener;
import com.squareup.picasso.Picasso;

import java.io.File;
import java.util.ArrayList;

import mbtool.daemon.v2.WipeTarget;

public class RomDetailActivity extends AppCompatActivity implements
        RomNameInputDialogListener,
        AddToHomeScreenOptionsDialogListener,
        CacheRomThumbnailTaskListener,
        RomDetailAdapterListener,
        ConfirmMismatchedSetKernelDialogListener,
        SetKernelConfirmDialogListener,
        WipeTargetsSelectionDialogListener,
        ConfirmChecksumIssueDialogListener,
        EventCollectorListener {
    private static final String TAG = RomDetailActivity.class.getSimpleName();

    private static final int MENU_EDIT_NAME = Menu.FIRST;
    private static final int MENU_CHANGE_IMAGE = Menu.FIRST + 1;
    private static final int MENU_RESET_IMAGE = Menu.FIRST + 2;

    private static final int INFO_SLOT = 1;
    private static final int INFO_VERSION = 2;
    private static final int INFO_BUILD = 3;
    private static final int INFO_MBTOOL_VERSION = 4;
    private static final int INFO_SYSTEM_SIZE = 5;
    private static final int INFO_CACHE_SIZE = 6;
    private static final int INFO_DATA_SIZE = 7;

    private static final int ACTION_SET_KERNEL = 1;
    private static final int ACTION_BACKUP = 2;
    private static final int ACTION_ADD_TO_HOME_SCREEN = 3;
    private static final int ACTION_WIPE_ROM = 4;

    private static final int PROGRESS_DIALOG_SWITCH_ROM = 1;
    private static final int PROGRESS_DIALOG_SET_KERNEL = 2;
    private static final int PROGRESS_DIALOG_WIPE_ROM = 3;

    private static final int REQUEST_IMAGE = 1234;

    public static final String EXTRA_ROM_INFO = "rom_info";
    public static final String EXTRA_BOOTED_ROM_INFO = "booted_rom_info";
    public static final String EXTRA_ACTIVE_ROM_ID = "active_rom_id";

    private CoordinatorLayout mCoordinator;

    private RomInformation mRomInfo;
    private RomInformation mBootedRomInfo;
    private String mActiveRomId;

    private ArrayList<Item> mItems = new ArrayList<>();
    private RomDetailAdapter mAdapter;

    private SwitcherEventCollector mEventCollector;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_rom_detail);

        FragmentManager fm = getFragmentManager();
        mEventCollector = (SwitcherEventCollector) fm.findFragmentByTag(SwitcherEventCollector.TAG);

        if (mEventCollector == null) {
            mEventCollector = new SwitcherEventCollector();
            fm.beginTransaction().add(mEventCollector, SwitcherEventCollector.TAG).commit();
        }

        Intent intent = getIntent();
        mRomInfo = intent.getParcelableExtra(EXTRA_ROM_INFO);
        mBootedRomInfo = intent.getParcelableExtra(EXTRA_BOOTED_ROM_INFO);
        mActiveRomId = intent.getStringExtra(EXTRA_ACTIVE_ROM_ID);

        final Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        mCoordinator = (CoordinatorLayout) findViewById(R.id.coordinator);

        FloatingActionButton fab = (FloatingActionButton) findViewById(R.id.fab);
        fab.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                switchRom(false);
            }
        });

        updateTitle();
        loadBackdrop();

        initializeRecyclerView();
        populateRomCardItem();
        populateInfoItems();
        populateActionItems();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mEventCollector.attachListener(TAG, this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mEventCollector.detachListener(TAG);
    }

    private void loadBackdrop() {
        // TODO: Need to implement mbtool backend for grabbing wallpaper
        final ImageView imageView = (ImageView) findViewById(R.id.backdrop);
        Picasso.with(this).load(R.drawable.material).into(imageView);
    }

    private void updateTitle() {
        final CollapsingToolbarLayout collapsingToolbar =
                (CollapsingToolbarLayout) findViewById(R.id.collapsing_toolbar);
        collapsingToolbar.setTitle(mRomInfo.getId());
        //collapsingToolbar.setTitle(mRomInfo.getName());

        // Hack to prevent the expanded title from showing
        AppBarLayout appBarLayout = (AppBarLayout) findViewById(R.id.appbar);
        appBarLayout.addOnOffsetChangedListener(new AppBarLayout.OnOffsetChangedListener() {
            boolean isShown = false;
            int scrollRange = -1;

            @Override
            public void onOffsetChanged(AppBarLayout appBarLayout, int verticalOffset) {
                if (scrollRange == -1) {
                    scrollRange = appBarLayout.getTotalScrollRange();
                }
                if (scrollRange + verticalOffset == 0) {
                    //collapsingToolbar.setTitle(mRomInfo.getName());
                    isShown = true;
                } else if (isShown) {
                    //collapsingToolbar.setTitle("");
                    isShown = false;
                }
            }
        });
    }

    private void initializeRecyclerView() {
        mAdapter = new RomDetailAdapter(mItems, this);

        RecyclerView rv = (RecyclerView) findViewById(R.id.main_rv);
        rv.setHasFixedSize(true);
        rv.setAdapter(mAdapter);
        rv.setLayoutManager(new LinearLayoutManager(this));
        rv.addItemDecoration(new DividerItemDecoration(this));
    }

    private void populateRomCardItem() {
        RomCardItem item = new RomCardItem(mRomInfo);
        mItems.add(item);
    }

    private void populateInfoItems() {
        mItems.add(new InfoItem(INFO_SLOT, "Slot", "Primary"));
        mItems.add(new InfoItem(INFO_VERSION, "Version", "5.0.2"));
        mItems.add(new InfoItem(INFO_BUILD, "Build", "LMY47X.N910T3UVU1DOG1"));
        mItems.add(new InfoItem(INFO_MBTOOL_VERSION, "mbtool version", "8.0.0.r1928.g2cb23c5"));
        mItems.add(new InfoItem(INFO_SYSTEM_SIZE, "System size", "2.7GB"));
        mItems.add(new InfoItem(INFO_CACHE_SIZE, "Cache size", "1.4GB"));
        mItems.add(new InfoItem(INFO_DATA_SIZE, "Data size", "1.2GB"));
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
        mEventCollector.createLauncher(info, reboot);
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

    @Override
    public void onEventReceived(BaseEvent bEvent) {
        if (bEvent instanceof CreatedLauncherEvent) {
            CreatedLauncherEvent event = (CreatedLauncherEvent) bEvent;

            createSnackbar(String.format(getString(R.string.successfully_created_launcher),
                    event.rom.getName()), Snackbar.LENGTH_LONG).show();
        } else if (bEvent instanceof SwitchedRomEvent) {
            SwitchedRomEvent event = (SwitchedRomEvent) bEvent;

            GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                    .findFragmentByTag(GenericProgressDialog.TAG + PROGRESS_DIALOG_SWITCH_ROM);
            if (d != null) {
                d.dismiss();
            }

            switch (event.result) {
            case SUCCEEDED:
                createSnackbar(R.string.choose_rom_success, Snackbar.LENGTH_LONG).show();
                Log.d(TAG, "Prior cached boot partition ROM ID was: " + mActiveRomId);
                mActiveRomId = event.kernelId;
                Log.d(TAG, "Changing cached boot partition ROM ID to: " + mActiveRomId);
                break;
            case FAILED:
                createSnackbar(R.string.choose_rom_failure, Snackbar.LENGTH_LONG).show();
                break;
            case CHECKSUM_INVALID:
                createSnackbar(R.string.choose_rom_checksums_invalid, Snackbar.LENGTH_LONG).show();
                showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_INVALID, event.kernelId);
                break;
            case CHECKSUM_NOT_FOUND:
                createSnackbar(R.string.choose_rom_checksums_missing, Snackbar.LENGTH_LONG).show();
                showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_MISSING, event.kernelId);
                break;
            case UNKNOWN_BOOT_PARTITION:
                showUnknownBootPartitionDialog();
                break;
            }
        } else if (bEvent instanceof SetKernelEvent) {
            SetKernelEvent event = (SetKernelEvent) bEvent;

            GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                    .findFragmentByTag(GenericProgressDialog.TAG + PROGRESS_DIALOG_SET_KERNEL);
            if (d != null) {
                d.dismiss();
            }

            switch (event.result) {
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
        } else if (bEvent instanceof WipedRomEvent) {
            WipedRomEvent event = (WipedRomEvent) bEvent;

            GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                    .findFragmentByTag(GenericProgressDialog.TAG + PROGRESS_DIALOG_WIPE_ROM);
            if (d != null) {
                d.dismiss();
            }

            if (event.succeeded == null || event.failed == null) {
                createSnackbar(R.string.wipe_rom_initiate_fail, Snackbar.LENGTH_LONG).show();
            } else {
                StringBuilder sb = new StringBuilder();

                if (event.succeeded.length > 0) {
                    sb.append(String.format(getString(R.string.wipe_rom_successfully_wiped),
                            targetsToString(event.succeeded)));
                    if (event.failed.length > 0) {
                        sb.append("\n");
                    }
                }
                if (event.failed.length > 0) {
                    sb.append(String.format(getString(R.string.wipe_rom_failed_to_wipe),
                            targetsToString(event.failed)));
                }

                createSnackbar(sb.toString(), Snackbar.LENGTH_LONG).show();
            }
        }
    }

    private void switchRom(boolean forceChecksumsUpdate) {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.switching_rom, R.string.please_wait);
        d.show(getFragmentManager(), GenericProgressDialog.TAG + PROGRESS_DIALOG_SWITCH_ROM);

        mEventCollector.chooseRom(mRomInfo.getId(), forceChecksumsUpdate);
    }

    private void setKernel() {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.setting_kernel, R.string.please_wait);
        d.show(getFragmentManager(), GenericProgressDialog.TAG + PROGRESS_DIALOG_SET_KERNEL);

        mEventCollector.setKernel(mRomInfo.getId());
    }

    private void wipeRom(short[] targets) {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.wiping_targets, R.string.please_wait);
        d.show(getFragmentManager(), GenericProgressDialog.TAG + PROGRESS_DIALOG_WIPE_ROM);

        mEventCollector.wipeRom(mRomInfo.getId(), targets);
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
        textView.setTextColor(getResources().getColor(R.color.text_color_primary));
    }

    private String targetsToString(short[] targets) {
        StringBuilder sb = new StringBuilder();

        for (int i = 0; i < targets.length; i++) {
            if (i != 0) {
                sb.append(", ");
            }

            if (targets[i] == WipeTarget.SYSTEM) {
                sb.append(getString(R.string.wipe_target_system));
            } else if (targets[i] == WipeTarget.CACHE) {
                sb.append(getString(R.string.wipe_target_cache));
            } else if (targets[i] == WipeTarget.DATA) {
                sb.append(getString(R.string.wipe_target_data));
            } else if (targets[i] == WipeTarget.DALVIK_CACHE) {
                sb.append(getString(R.string.wipe_target_dalvik_cache));
            } else if (targets[i] == WipeTarget.MULTIBOOT) {
                sb.append(getString(R.string.wipe_target_multiboot_files));
            }
        }

        return sb.toString();
    }
}