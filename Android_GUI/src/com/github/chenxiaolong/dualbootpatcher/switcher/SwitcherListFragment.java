/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.LoaderManager;
import android.content.AsyncTaskLoader;
import android.content.Context;
import android.content.Intent;
import android.content.Loader;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.StringRes;
import android.support.design.widget.Snackbar;
import android.support.v4.widget.SwipeRefreshLayout.OnRefreshListener;
import android.support.v7.widget.CardView;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.TextView;

import com.getbase.floatingactionbutton.FloatingActionButton;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.BootImage;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.dualbootpatcher.switcher.AddToHomeScreenOptionsDialog
        .AddToHomeScreenOptionsDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmChecksumIssueDialog
        .ConfirmChecksumIssueDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmMismatchedSetKernelDialog
        .ConfirmMismatchedSetKernelDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.ExperimentalInAppWipeDialog
        .ExperimentalInAppWipeDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomCardAdapter.RomCardActionListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomNameInputDialog
        .RomNameInputDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SetKernelConfirmDialog
        .SetKernelConfirmDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SetKernelNeededDialog
        .SetKernelNeededDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.CreatedLauncherEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.SetKernelEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.SwitchedRomEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.WipedRomEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherListFragment.LoaderResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.WipeTargetsSelectionDialog
        .WipeTargetsSelectionDialogListener;
import com.github.chenxiaolong.dualbootpatcher.views.SwipeRefreshLayoutWorkaround;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;

import mbtool.daemon.v2.WipeTarget;

public class SwitcherListFragment extends Fragment implements
        EventCollectorListener, RomCardActionListener,
        ExperimentalInAppWipeDialogListener,
        SetKernelConfirmDialogListener,
        SetKernelNeededDialogListener,
        WipeTargetsSelectionDialogListener,
        RomNameInputDialogListener,
        ConfirmMismatchedSetKernelDialogListener,
        ConfirmChecksumIssueDialogListener,
        AddToHomeScreenOptionsDialogListener,
        LoaderManager.LoaderCallbacks<LoaderResult> {
    public static final String TAG = SwitcherListFragment.class.getSimpleName();

    private static final String EXTRA_PERFORMING_ACTION = "performingAction";
    private static final String EXTRA_SELECTED_ROM = "selectedRom";
    private static final String EXTRA_SHOWED_SET_KERNEL_WARNING = "showedSetKernelWarning";

    private static final int PROGRESS_DIALOG_SWITCH_ROM = 1;
    private static final int PROGRESS_DIALOG_SET_KERNEL = 2;
    private static final int PROGRESS_DIALOG_WIPE_ROM = 3;

    private static final int REQUEST_IMAGE = 1234;
    private static final int REQUEST_FLASH_ZIP = 2345;

    private boolean mPerformingAction;

    private SwitcherEventCollector mEventCollector;

    private CardView mErrorCardView;
    private RomCardAdapter mRomCardAdapter;
    private RecyclerView mCardListView;
    private FloatingActionButton mFabFlashZip;
    private SwipeRefreshLayoutWorkaround mSwipeRefresh;

    private ArrayList<RomInformation> mRoms;
    private RomInformation mCurrentRom;
    private RomInformation mSelectedRom;
    private String mCurrentBootRomId;

    private boolean mShowedSetKernelWarning;

    public static SwitcherListFragment newInstance() {
        return new SwitcherListFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        FragmentManager fm = getFragmentManager();
        mEventCollector = (SwitcherEventCollector) fm.findFragmentByTag(SwitcherEventCollector.TAG);

        if (mEventCollector == null) {
            mEventCollector = new SwitcherEventCollector();
            fm.beginTransaction().add(mEventCollector, SwitcherEventCollector.TAG).commit();
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (savedInstanceState != null) {
            mPerformingAction = savedInstanceState.getBoolean(EXTRA_PERFORMING_ACTION);

            mSelectedRom = savedInstanceState.getParcelable(EXTRA_SELECTED_ROM);

            mShowedSetKernelWarning = savedInstanceState.getBoolean(
                    EXTRA_SHOWED_SET_KERNEL_WARNING);
        }

        mFabFlashZip = (FloatingActionButton) getActivity()
                .findViewById(R.id.fab_flash_zip);
        mFabFlashZip.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(getActivity(), ZipFlashingActivity.class);
                startActivityForResult(intent, REQUEST_FLASH_ZIP);
            }
        });

        mSwipeRefresh = (SwipeRefreshLayoutWorkaround)
                getActivity().findViewById(R.id.swiperefreshroms);
        mSwipeRefresh.setOnRefreshListener(new OnRefreshListener() {
            @Override
            public void onRefresh() {
                reloadFragment();
            }
        });

        mSwipeRefresh.setColorSchemeResources(R.color.swipe_refresh_color1,
                R.color.swipe_refresh_color2, R.color.swipe_refresh_color3,
                R.color.swipe_refresh_color4);

        initErrorCard();
        initCardList();
        refreshErrorVisibility(false);

        // Show progress bar on initial load, not on rotation
        refreshProgressVisibility(savedInstanceState == null);

        getActivity().getLoaderManager().initLoader(0, null, this);
    }

    private void reloadFragment() {
        refreshErrorVisibility(false);
        refreshRomListVisibility(false);
        refreshFabVisibility(false);
        refreshProgressVisibility(true);
        getActivity().getLoaderManager().restartLoader(0, null, this);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_switcher, container, false);
    }

    @Override
    public void onResume() {
        super.onResume();

        mEventCollector.attachListener(TAG, this);
    }

    @Override
    public void onPause() {
        super.onPause();

        mEventCollector.detachListener(TAG);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean(EXTRA_PERFORMING_ACTION, mPerformingAction);
        if (mSelectedRom != null) {
            outState.putParcelable(EXTRA_SELECTED_ROM, mSelectedRom);
        }
        outState.putBoolean(EXTRA_SHOWED_SET_KERNEL_WARNING, mShowedSetKernelWarning);
    }

    /**
     * Create error card on fragment startup
     */
    private void initErrorCard() {
        mErrorCardView = (CardView) getActivity().findViewById(R.id.card_error);
        mErrorCardView.setClickable(true);
        mErrorCardView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                reloadFragment();
            }
        });
    }

    /**
     * Create CardListView on fragment startup
     */
    private void initCardList() {
        mRoms = new ArrayList<>();
        mRomCardAdapter = new RomCardAdapter(getActivity(), mRoms, this);

        mCardListView = (RecyclerView) getActivity().findViewById(R.id.card_list);
        mCardListView.setHasFixedSize(true);
        mCardListView.setAdapter(mRomCardAdapter);

        LinearLayoutManager llm = new LinearLayoutManager(getActivity());
        llm.setOrientation(LinearLayoutManager.VERTICAL);
        mCardListView.setLayoutManager(llm);

        refreshRomListVisibility(false);
        refreshFabVisibility(false);
    }

    private void refreshProgressVisibility(boolean visible) {
        mSwipeRefresh.setRefreshing(visible);
    }

    private void refreshRomListVisibility(boolean visible) {
        mCardListView.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    private void refreshErrorVisibility(boolean visible) {
        mErrorCardView.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    private void refreshFabVisibility(boolean visible) {
        mFabFlashZip.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    private void updateCardUI() {
        if (mPerformingAction) {
            // Keep screen on
            getActivity().getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        } else {
            // Don't keep screen on
            getActivity().getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
    }

    @Override
    public void onActivityResult(int request, int result, Intent data) {
        switch (request) {
        case REQUEST_IMAGE:
            if (data != null && result == Activity.RESULT_OK) {
                new ResizeAndCacheImageTask(getActivity().getApplicationContext(),
                        data.getData()).execute();
            }
            break;
        case REQUEST_FLASH_ZIP:
            reloadFragment();
            break;
        }

        super.onActivityResult(request, result, data);
    }

    @Override
    public void onEventReceived(BaseEvent bEvent) {
        if (bEvent instanceof SwitchedRomEvent) {
            SwitchedRomEvent event = (SwitchedRomEvent) bEvent;
            mPerformingAction = false;
            updateCardUI();

            GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                    .findFragmentByTag(GenericProgressDialog.TAG + PROGRESS_DIALOG_SWITCH_ROM);
            if (d != null) {
                d.dismiss();
            }

            switch (event.result) {
            case SUCCEEDED:
                createSnackbar(R.string.choose_rom_success, Snackbar.LENGTH_LONG).show();
                Log.d(TAG, "Prior cached boot partition ROM ID was: " + mCurrentBootRomId);
                mCurrentBootRomId = event.kernelId;
                Log.d(TAG, "Changing cached boot partition ROM ID to: " + mCurrentBootRomId);
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
            mPerformingAction = false;
            updateCardUI();

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
        } else if (bEvent instanceof CreatedLauncherEvent) {
            CreatedLauncherEvent event = (CreatedLauncherEvent) bEvent;

            createSnackbar(String.format(getString(R.string.successfully_created_launcher),
                    event.rom.getName()), Snackbar.LENGTH_LONG).show();
        } else if (bEvent instanceof WipedRomEvent) {
            WipedRomEvent event = (WipedRomEvent) bEvent;
            mPerformingAction = false;
            updateCardUI();

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

                // We don't want deleted ROMs to still show up
                reloadFragment();
            }
        } else if (bEvent instanceof ShowSetKernelNeededEvent) {
            ShowSetKernelNeededEvent event = (ShowSetKernelNeededEvent) bEvent;

            if (!mShowedSetKernelWarning) {
                mShowedSetKernelWarning = true;

                if (event.kernelStatus == CurrentKernelStatus.DIFFERENT) {
                    SetKernelNeededDialog dialog = SetKernelNeededDialog.newInstance(
                            this, R.string.kernel_different_from_saved_desc);
                    dialog.show(getFragmentManager(), SetKernelNeededDialog.TAG);
                } else if (event.kernelStatus == CurrentKernelStatus.UNSET) {
                    SetKernelNeededDialog dialog = SetKernelNeededDialog.newInstance(
                            this, R.string.kernel_not_set_desc);
                    dialog.show(getFragmentManager(), SetKernelNeededDialog.TAG);
                }
            }
        }
    }

    private Snackbar createSnackbar(String text, int duration) {
        Snackbar snackbar = Snackbar.make(mFabFlashZip, text, duration);
        snackbarSetTextColor(snackbar);
        return snackbar;
    }

    private Snackbar createSnackbar(@StringRes int resId, int duration) {
        Snackbar snackbar = Snackbar.make(mFabFlashZip, resId, duration);
        snackbarSetTextColor(snackbar);
        return snackbar;
    }

    private void snackbarSetTextColor(Snackbar snackbar) {
        View view = snackbar.getView();
        TextView textView = (TextView) view.findViewById(android.support.design.R.id.snackbar_text);
        textView.setTextColor(getResources().getColor(R.color.text_color_primary));
    }

    private void showChecksumIssueDialog(int issue, String romId) {
        ConfirmChecksumIssueDialog d =
                ConfirmChecksumIssueDialog.newInstance(this, issue, romId);
        d.show(getFragmentManager(), ConfirmChecksumIssueDialog.TAG);
    }

    private void showUnknownBootPartitionDialog() {
        String codename = RomUtils.getDeviceCodename(getActivity());
        String message = String.format(getString(R.string.unknown_boot_partition), codename);

        GenericConfirmDialog gcd = GenericConfirmDialog.newInstance(null, message);
        gcd.show(getFragmentManager(), GenericConfirmDialog.TAG);
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

    @Override
    public void onSelectedRom(RomInformation info) {
        mSelectedRom = info;

        chooseRom(info.getId(), false);
    }

    @Override
    public void onSelectedSetKernel(RomInformation info) {
        mSelectedRom = info;

        // Ask for confirmation
        if (mCurrentBootRomId != null && !mCurrentBootRomId.equals(mSelectedRom.getId())) {
            ConfirmMismatchedSetKernelDialog d = ConfirmMismatchedSetKernelDialog.newInstance(
                    this, mCurrentBootRomId, mSelectedRom.getId());
            d.show(getFragmentManager(), ConfirmMismatchedSetKernelDialog.TAG);
        } else {
            SetKernelConfirmDialog d = SetKernelConfirmDialog.newInstance(this, mSelectedRom);
            d.show(getFragmentManager(), SetKernelConfirmDialog.TAG);
        }
    }

    private void chooseRom(String romId, boolean forceChecksumsUpdate) {
        mPerformingAction = true;
        updateCardUI();

        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.switching_rom, R.string.please_wait);
        d.show(getFragmentManager(), GenericProgressDialog.TAG + PROGRESS_DIALOG_SWITCH_ROM);

        mEventCollector.chooseRom(romId, forceChecksumsUpdate);
    }

    private void setKernel(RomInformation info) {
        mPerformingAction = true;
        updateCardUI();

        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.setting_kernel, R.string.please_wait);
        d.show(getFragmentManager(), GenericProgressDialog.TAG + PROGRESS_DIALOG_SET_KERNEL);

        mEventCollector.setKernel(info.getId());
    }

    @Override
    public void onSelectedAddToHomeScreen(RomInformation info) {
        mSelectedRom = info;

        AddToHomeScreenOptionsDialog d = AddToHomeScreenOptionsDialog.newInstance(this, info);
        d.show(getFragmentManager(), AddToHomeScreenOptionsDialog.TAG);
    }

    @Override
    public void onConfirmAddToHomeScreenOptions(RomInformation info, boolean reboot) {
        mEventCollector.createLauncher(info, reboot);
    }

    @Override
    public void onSelectedEditName(RomInformation info) {
        mSelectedRom = info;

        RomNameInputDialog d = RomNameInputDialog.newInstance(this, mSelectedRom);
        d.show(getFragmentManager(), RomNameInputDialog.TAG);
    }

    @Override
    public void onSelectedChangeImage(RomInformation info) {
        mSelectedRom = info;

        Intent intent = new Intent();
        intent.setType("image/*");
        intent.setAction(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        startActivityForResult(intent, REQUEST_IMAGE);
    }

    @Override
    public void onSelectedResetImage(RomInformation info) {
        mSelectedRom = info;

        File f = new File(info.getThumbnailPath());
        if (f.isFile()) {
            f.delete();
        }

        mRomCardAdapter.notifyDataSetChanged();
    }

    @Override
    public void onSelectedWipeRom(RomInformation info) {
        mSelectedRom = info;

        if (mCurrentRom != null && mCurrentRom.getId().equals(info.getId())) {
            createSnackbar(R.string.wipe_rom_no_wipe_current_rom, Snackbar.LENGTH_LONG).show();
        } else {
            ExperimentalInAppWipeDialog d = ExperimentalInAppWipeDialog.newInstance(this);
            d.show(getFragmentManager(), ExperimentalInAppWipeDialog.TAG);
        }
    }

    private void wipeRom(RomInformation info, short[] targets) {
        mPerformingAction = true;
        updateCardUI();

        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.wiping_targets, R.string.please_wait);
        d.show(getFragmentManager(), GenericProgressDialog.TAG + PROGRESS_DIALOG_WIPE_ROM);

        mEventCollector.wipeRom(info.getId(), targets);
    }

    @Override
    public Loader<LoaderResult> onCreateLoader(int id, Bundle args) {
        return new RomsLoader(getActivity());
    }

    @Override
    public void onLoadFinished(Loader<LoaderResult> loader, LoaderResult result) {
        mRoms.clear();

        if (result.roms != null) {
            Collections.addAll(mRoms, result.roms);
            refreshFabVisibility(true);
        } else {
            refreshErrorVisibility(true);
            refreshFabVisibility(false);
        }

        mCurrentRom = result.currentRom;
        mCurrentBootRomId = result.currentBootRomId;

        mRomCardAdapter.notifyDataSetChanged();
        updateCardUI();

        refreshProgressVisibility(false);
        refreshRomListVisibility(true);

        // Show a dialog if the kernel is different or unset. We can't directly show the
        // DialogFragment here because this method might be called after the state has already been
        // saved (leading to an IllegalStateException). Instead we'll just use EventCollector to
        // tell SwitcherListFragment that we want to show a dialog. If the fragment is still valid,
        // it will be shown immediately. Otherwise, it'll be shown when the fragment is recreated.
        if (mCurrentRom != null && mCurrentBootRomId != null
                && mCurrentRom.getId().equals(mCurrentBootRomId)) {
            // Only show if the current boot image ROM ID matches the booted ROM. Otherwise, the
            // user can switch to another ROM, exit the app, and open it again to trigger the dialog
            mEventCollector.postEvent(new ShowSetKernelNeededEvent(result.kernelStatus));
        }
    }

    @Override
    public void onLoaderReset(Loader<LoaderResult> loader) {
    }

    @Override
    public void onConfirmInAppRomWipeWarning() {
        WipeTargetsSelectionDialog d = WipeTargetsSelectionDialog.newInstance(this);
        d.show(getFragmentManager(), WipeTargetsSelectionDialog.TAG);
    }

    @Override
    public void onConfirmSetKernel() {
        setKernel(mSelectedRom);
    }

    @Override
    public void onConfirmSetKernelNeeded() {
        setKernel(mCurrentRom);
    }

    @Override
    public void onSelectedWipeTargets(short[] targets) {
        if (targets.length == 0) {
            createSnackbar(R.string.wipe_rom_none_selected, Snackbar.LENGTH_LONG).show();
            return;
        }

        wipeRom(mSelectedRom, targets);
    }

    @Override
    public void onRomNameChanged(String newName) {
        mSelectedRom.setName(newName);

        new Thread() {
            @Override
            public void run() {
                RomUtils.saveConfig(mSelectedRom);
            }
        }.start();

        mRomCardAdapter.notifyDataSetChanged();
    }

    @Override
    public void onConfirmMismatchedSetKernel() {
        setKernel(mCurrentRom);
    }

    @Override
    public void onConfirmChecksumIssue(String romId) {
        chooseRom(romId, true);
    }

    public class ShowSetKernelNeededEvent extends BaseEvent {
        CurrentKernelStatus kernelStatus;

        public ShowSetKernelNeededEvent(CurrentKernelStatus kernelStatus) {
            this.kernelStatus = kernelStatus;
        }
    }

    protected class ResizeAndCacheImageTask extends AsyncTask<Void, Void, Void> {
        private final Context mContext;
        private final Uri mUri;

        public ResizeAndCacheImageTask(Context context, Uri uri) {
            mContext = context;
            mUri = uri;
        }

        private Bitmap getThumbnail(Uri uri) {
            try {
                InputStream input = mContext.getContentResolver().openInputStream(uri);
                Bitmap bitmap = BitmapFactory.decodeStream(input);
                input.close();

                if (bitmap == null) {
                    return null;
                }

                return ThumbnailUtils.extractThumbnail(bitmap, 500, 500);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }

            return null;
        }

        @Override
        protected Void doInBackground(Void... params) {
            Bitmap thumbnail = getThumbnail(mUri);

            if (thumbnail == null) {
                return null;
            }

            // Write the image to a temporary file. If the user selects it,
            // the move it to the appropriate location.
            File f = new File(mSelectedRom.getThumbnailPath());
            f.getParentFile().mkdirs();

            FileOutputStream out = null;

            try {
                out = new FileOutputStream(f);
                thumbnail.compress(Bitmap.CompressFormat.WEBP, 100, out);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } finally {
                IOUtils.closeQuietly(out);
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            if (getActivity() == null) {
                return;
            }

            mRomCardAdapter.notifyDataSetChanged();
        }
    }

    private static class RomsLoader extends AsyncTaskLoader<LoaderResult> {
        private LoaderResult mResult;

        public RomsLoader(Context context) {
            super(context);
            onContentChanged();
        }

        @Override
        protected void onStartLoading() {
            if (mResult != null) {
                deliverResult(mResult);
            } else if (takeContentChanged()) {
                forceLoad();
            }
        }

        @Override
        public LoaderResult loadInBackground() {
            mResult = new LoaderResult();
            mResult.roms = RomUtils.getRoms(getContext());
            mResult.currentRom = RomUtils.getCurrentRom(getContext());
            long start = System.currentTimeMillis();
            obtainBootPartitionInfo(mResult.currentRom);
            long end = System.currentTimeMillis();
            Log.d(TAG, "It took " + (end - start) + " milliseconds to complete boot image checks");
            Log.d(TAG, "Current boot partition ROM ID: " + mResult.currentBootRomId);
            Log.d(TAG, "Kernel status: " + mResult.kernelStatus.name());
            return mResult;
        }

        private void obtainBootPartitionInfo(RomInformation currentRom) {
            // Create temporary copy of the boot partition
            String bootPartition = SwitcherUtils.getBootPartition(getContext());
            if (bootPartition == null) {
                Log.e(TAG, "Failed to determine boot partition");
                // Bail out. Both currentBootRomId and kernelStatus require access to the boot image
                // on the boot partition
                mResult.currentBootRomId = null;
                mResult.kernelStatus = CurrentKernelStatus.UNKNOWN;
                return;
            }

            String tmpImage = getContext().getCacheDir() + File.separator + "boot.img";
            File tmpImageFile = new File(tmpImage);

            MbtoolSocket socket = MbtoolSocket.getInstance();

            try {
                // Copy boot partition to the temporary file
                if (!socket.copy(getContext(), bootPartition, tmpImage) ||
                        !socket.chmod(getContext(), tmpImage, 0644)) {
                    Log.e(TAG, "Failed to copy boot partition to temporary file");
                    mResult.currentBootRomId = null;
                    mResult.kernelStatus = CurrentKernelStatus.UNKNOWN;
                    return;
                }

                mResult.currentBootRomId = SwitcherUtils.getBootImageRomId(
                        getContext(), tmpImageFile.getAbsolutePath());
                mResult.kernelStatus = getKernelStatus(currentRom, tmpImageFile);
            } catch (IOException e) {
                Log.e(TAG, "mbtool communication error", e);
                mResult.currentBootRomId = null;
                mResult.kernelStatus = CurrentKernelStatus.UNKNOWN;
            } finally {
                tmpImageFile.delete();
            }
        }

        private CurrentKernelStatus getKernelStatus(RomInformation currentRom, File tmpImageFile) {
            if (currentRom == null) {
                return CurrentKernelStatus.UNKNOWN;
            }

            String savedImage = RomUtils.getBootImagePath(currentRom.getId());
            File savedImageFile = new File(savedImage);

            if (!savedImageFile.isFile()) {
                return CurrentKernelStatus.UNSET;
            }

            // Create temporary copy of the boot partition
            String bootPartition = SwitcherUtils.getBootPartition(getContext());
            if (bootPartition == null) {
                Log.e(TAG, "Failed to determine boot partition when finding kernel status");
                return CurrentKernelStatus.UNKNOWN;
            }

            BootImage biSaved = new BootImage();
            BootImage biRunning = new BootImage();

            try {
                if (!biSaved.load(savedImage)) {
                    Log.e(TAG, "libmbp error code: " + biSaved.getError());
                    return CurrentKernelStatus.UNKNOWN;
                }
                if (!biRunning.load(tmpImageFile.getAbsolutePath())) {
                    Log.e(TAG, "libmbp error code: " + biRunning.getError());
                    return CurrentKernelStatus.UNKNOWN;
                }

                return biSaved.equals(biRunning)
                        ? CurrentKernelStatus.SET
                        : CurrentKernelStatus.DIFFERENT;
            } finally {
                biSaved.destroy();
                biRunning.destroy();
            }
        }
    }

    protected enum CurrentKernelStatus {
        UNSET,
        DIFFERENT,
        SET,
        UNKNOWN
    }

    protected static class LoaderResult {
        /** List of installed ROMs */
        RomInformation[] roms;
        /** Currently booted ROM */
        RomInformation currentRom;
        /** ROM ID of boot image currently on the boot partition */
        String currentBootRomId;
        /** Current kernel status (set, unset, unknown) */
        CurrentKernelStatus kernelStatus;
    }
}
