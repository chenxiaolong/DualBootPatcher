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

import android.app.Fragment;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
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

import com.getbase.floatingactionbutton.FloatingActionButton;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.SnackbarUtils;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SwitchRomResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmChecksumIssueDialog
        .ConfirmChecksumIssueDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomCardAdapter.RomCardActionListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SetKernelNeededDialog
        .SetKernelNeededDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.KernelStatus;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask.TaskState;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomsStateTask
        .GetRomsStateTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask.SetKernelTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask.SwitchRomTaskListener;
import com.github.chenxiaolong.dualbootpatcher.views.SwipeRefreshLayoutWorkaround;

import java.util.ArrayList;
import java.util.Collections;

public class SwitcherListFragment extends Fragment implements
        RomCardActionListener, SetKernelNeededDialogListener, ConfirmChecksumIssueDialogListener,
        ServiceConnection {
    public static final String TAG = SwitcherListFragment.class.getSimpleName();

    private static final String EXTRA_TASK_ID_GET_ROMS_STATE = "task_id_get_roms_state";
    private static final String EXTRA_TASK_ID_SWITCH_ROM = "task_id_switch_rom";
    private static final String EXTRA_TASK_ID_SET_KERNEL = "task_id_set_kernel";
    private static final String EXTRA_TASK_IDS_TO_REMOVE = "task_ids_to_reomve";

    private static final String EXTRA_PERFORMING_ACTION = "performing_action";
    private static final String EXTRA_SELECTED_ROM = "selected_rom";
    private static final String EXTRA_ACTIVE_ROM_ID = "active_rom_id";
    private static final String EXTRA_SHOWED_SET_KERNEL_WARNING = "showed_set_kernel_warning";

    /** Fragment tag for progress dialog for switching ROMS */
    private static final String PROGRESS_DIALOG_SWITCH_ROM =
            SwitcherListFragment.class.getCanonicalName() + ".progress.switch_rom";
    /** Fragment tag for progress dialog for setting the kernel */
    private static final String PROGRESS_DIALOG_SET_KERNEL =
            SwitcherListFragment.class.getCanonicalName() + ".progress.set_kernel";
    /** Fragment tag for confirmation dialog saying that setting the kernel is needed */
    private static final String DIALOG_SET_KERNEL_NEEDED =
            SwitcherListFragment.class.getCanonicalName() + ".set_kernel_needed";

    /** Service task ID to get the state of installed ROMs */
    private int mTaskIdGetRomsState = -1;
    /** Service task ID for switching the ROM */
    private int mTaskIdSwitchRom = -1;
    /** Service task ID for setting the kernel */
    private int mTaskIdSetKernel = -1;

    /** Task IDs to remove */
    private ArrayList<Integer> mTaskIdsToRemove = new ArrayList<>();

    /** Switcher service */
    private SwitcherService mService;
    /** Callback for events from the service */
    private final SwitcherEventCallback mCallback = new SwitcherEventCallback();

    /** Handler for processing events from the service on the UI thread */
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    private static final int REQUEST_FLASH_ZIP = 2345;

    private boolean mPerformingAction;

    private CardView mErrorCardView;
    private RomCardAdapter mRomCardAdapter;
    private RecyclerView mCardListView;
    private FloatingActionButton mFabFlashZip;
    private SwipeRefreshLayoutWorkaround mSwipeRefresh;

    private ArrayList<RomInformation> mRoms;
    private RomInformation mCurrentRom;
    private RomInformation mSelectedRom;
    private String mActiveRomId;

    private boolean mShowedSetKernelWarning;

    public static SwitcherListFragment newInstance() {
        return new SwitcherListFragment();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (savedInstanceState != null) {
            mTaskIdGetRomsState = savedInstanceState.getInt(EXTRA_TASK_ID_GET_ROMS_STATE);
            mTaskIdSwitchRom = savedInstanceState.getInt(EXTRA_TASK_ID_SWITCH_ROM);
            mTaskIdSetKernel = savedInstanceState.getInt(EXTRA_TASK_ID_SET_KERNEL);
            mTaskIdsToRemove = savedInstanceState.getIntegerArrayList(EXTRA_TASK_IDS_TO_REMOVE);

            mPerformingAction = savedInstanceState.getBoolean(EXTRA_PERFORMING_ACTION);

            mSelectedRom = savedInstanceState.getParcelable(EXTRA_SELECTED_ROM);
            mActiveRomId = savedInstanceState.getString(EXTRA_ACTIVE_ROM_ID);

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
                reloadRomsState();
            }
        });

        mSwipeRefresh.setColorSchemeResources(R.color.swipe_refresh_color1,
                R.color.swipe_refresh_color2, R.color.swipe_refresh_color3,
                R.color.swipe_refresh_color4);

        initErrorCard();
        initCardList();
        setErrorVisibility(false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onStart() {
        super.onStart();

        // Start and bind to the service
        Intent intent = new Intent(getActivity(), SwitcherService.class);
        getActivity().bindService(intent, this, Context.BIND_AUTO_CREATE);
        getActivity().startService(intent);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onStop() {
        super.onStop();

        // If we connected to the service and registered the callback, now we unregister it
        if (mService != null) {
            mService.unregisterCallback(mCallback);
        }

        // Unbind from our service
        getActivity().unbindService(this);
        mService = null;

        // At this point, the mCallback will not get called anymore by the service. Now we just need
        // to remove all pending Runnables that were posted to mHandler.
        mHandler.removeCallbacksAndMessages(null);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        // Save a reference to the service so we can interact with it
        ThreadPoolServiceBinder binder = (ThreadPoolServiceBinder) service;
        mService = (SwitcherService) binder.getService();

        // Register callback
        mService.registerCallback(mCallback);

        // Remove old task IDs
        for (int taskId : mTaskIdsToRemove) {
            mService.removeCachedTask(taskId);
        }
        mTaskIdsToRemove.clear();

        if (mTaskIdGetRomsState < 0) {
            // Load ROMs state asynchronously
            setLoadingSpinnerVisibility(true);
            mTaskIdGetRomsState = mService.getRomsState();
        } else if (mService.getCachedTaskState(mTaskIdGetRomsState) == TaskState.FINISHED) {
            // If ROMs state has already been loaded, then get the results from the service
            RomInformation[] roms = mService.getResultRomsStateRomsList(mTaskIdGetRomsState);
            RomInformation currentRom = mService.getResultRomsStateCurrentRom(mTaskIdGetRomsState);
            String activeRomId = mService.getResultRomsStateActiveRomId(mTaskIdGetRomsState);
            KernelStatus kernelStatus = mService.getResultRomsStateKernelStatus(mTaskIdGetRomsState);

            onGotRomsState(roms, currentRom, activeRomId, kernelStatus);
        } else {
            // Still loading
            setLoadingSpinnerVisibility(true);
        }

        if (mTaskIdSwitchRom >= 0 &&
                mService.getCachedTaskState(mTaskIdSwitchRom) == TaskState.FINISHED) {
            String romId = mService.getResultSwitchRomRomId(mTaskIdSwitchRom);
            SwitchRomResult result = mService.getResultSwitchRomResult(mTaskIdSwitchRom);

            onSwitchedRom(romId, result);
        }

        if (mTaskIdSetKernel >= 0 &&
                mService.getCachedTaskState(mTaskIdSetKernel) == TaskState.FINISHED) {
            String romId = mService.getResultSetKernelRomId(mTaskIdSetKernel);
            SetKernelResult result = mService.getResultSetKernelResult(mTaskIdSetKernel);

            onSetKernel(romId, result);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onServiceDisconnected(ComponentName name) {
        mService = null;
    }

    /**
     * Reload ROMs state
     *
     * This will be behave as if the entire fragment has been restarted. The following actions will
     * be performed:
     * - The active ROM ID will be cleared
     * - Various UI elements will be reset to their loading state
     * - The cached task will be removed from the service
     * - The Service will reload the ROMs state
     */
    private void reloadRomsState() {
        mActiveRomId = null;
        setErrorVisibility(false);
        setRomListVisibility(false);
        setFabVisibility(false);
        setLoadingSpinnerVisibility(true);

        removeCachedTaskId(mTaskIdGetRomsState);
        // onActivityResult() gets called before onStart() (apparently), so we'll just set the task
        // ID to -1 and wait for the natural reload in onServiceConnected().
        if (mService != null) {
            mTaskIdGetRomsState = mService.getRomsState();
        } else {
            mTaskIdGetRomsState = -1;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_switcher, container, false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putInt(EXTRA_TASK_ID_GET_ROMS_STATE, mTaskIdGetRomsState);
        outState.putInt(EXTRA_TASK_ID_SWITCH_ROM, mTaskIdSwitchRom);
        outState.putInt(EXTRA_TASK_ID_SET_KERNEL, mTaskIdSetKernel);
        outState.putIntegerArrayList(EXTRA_TASK_IDS_TO_REMOVE, mTaskIdsToRemove);

        outState.putBoolean(EXTRA_PERFORMING_ACTION, mPerformingAction);
        if (mSelectedRom != null) {
            outState.putParcelable(EXTRA_SELECTED_ROM, mSelectedRom);
        }
        outState.putString(EXTRA_ACTIVE_ROM_ID, mActiveRomId);
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
                reloadRomsState();
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

        setRomListVisibility(false);
        setFabVisibility(false);
    }

    /**
     * Set visibility of loading progress spinner
     *
     * @param visible Visibility
     */
    private void setLoadingSpinnerVisibility(boolean visible) {
        mSwipeRefresh.setRefreshing(visible);
    }

    /**
     * Set visibility of the ROMs list
     *
     * @param visible Visibility
     */
    private void setRomListVisibility(boolean visible) {
        mCardListView.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    /**
     * Set visibility of the mbtool connection error message
     *
     * @param visible Visibility
     */
    private void setErrorVisibility(boolean visible) {
        mErrorCardView.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    /**
     * Set visibility of the FAB for in-app flashing
     *
     * @param visible Visibility
     */
    private void setFabVisibility(boolean visible) {
        mFabFlashZip.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    private void removeCachedTaskId(int taskId) {
        if (mService != null) {
            mService.removeCachedTask(taskId);
        } else {
            mTaskIdsToRemove.add(taskId);
        }
    }

    private void updateKeepScreenOnStatus() {
        // NOTE: May crash on some versions of Android due to a bug where getActivity() returns null
        // after onAttach() has been called, but before onDetach() has been called. It's similar to
        // this bug report, except it happens with android.app.Fragment:
        // https://code.google.com/p/android/issues/detail?id=67519
        if (isAdded()) {
            if (mPerformingAction) {
                // Keep screen on
                getActivity().getWindow().addFlags(
                        WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            } else {
                // Don't keep screen on
                getActivity().getWindow().clearFlags(
                        WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onActivityResult(int request, int result, Intent data) {
        switch (request) {
        case REQUEST_FLASH_ZIP:
            reloadRomsState();
            break;
        }

        super.onActivityResult(request, result, data);
    }

    private void onGotRomsState(RomInformation[] roms, RomInformation currentRom,
                                String activeRomId, KernelStatus kernelStatus) {
        mRoms.clear();

        if (roms != null && roms.length > 0) {
            Collections.addAll(mRoms, roms);
            setFabVisibility(true);
        } else {
            setErrorVisibility(true);
            setFabVisibility(false);
        }

        mCurrentRom = currentRom;

        // Only set mActiveRomId if it isn't already set (eg. restored from savedInstanceState).
        // reloadRomsState() will set mActiveRomId back to null so it is refreshed.
        if (mActiveRomId == null) {
            mActiveRomId = activeRomId;
        }

        mRomCardAdapter.setActiveRomId(mActiveRomId);
        mRomCardAdapter.notifyDataSetChanged();
        updateKeepScreenOnStatus();

        setLoadingSpinnerVisibility(false);
        setRomListVisibility(true);

        if (mCurrentRom != null && mActiveRomId != null
                && mCurrentRom.getId().equals(mActiveRomId)) {
            // Only show if the current boot image ROM ID matches the booted ROM. Otherwise, the
            // user can switch to another ROM, exit the app, and open it again to trigger the dialog
            //mEventCollector.postEvent(new ShowSetKernelNeededEvent(result.kernelStatus));
            if (!mShowedSetKernelWarning) {
                mShowedSetKernelWarning = true;

                if (kernelStatus == KernelStatus.DIFFERENT) {
                    SetKernelNeededDialog dialog = SetKernelNeededDialog.newInstance(
                            this, R.string.kernel_different_from_saved_desc);
                    dialog.show(getFragmentManager(), DIALOG_SET_KERNEL_NEEDED);
                } else if (kernelStatus == KernelStatus.UNSET) {
                    SetKernelNeededDialog dialog = SetKernelNeededDialog.newInstance(
                            this, R.string.kernel_not_set_desc);
                    dialog.show(getFragmentManager(), DIALOG_SET_KERNEL_NEEDED);
                }
            }
        }
    }

    private void onSwitchedRom(String romId, SwitchRomResult result) {
        // Remove cached task from service
        removeCachedTaskId(mTaskIdSwitchRom);
        mTaskIdSwitchRom = -1;

        mPerformingAction = false;
        updateKeepScreenOnStatus();

        GenericProgressDialog d = (GenericProgressDialog) getFragmentManager()
                .findFragmentByTag(PROGRESS_DIALOG_SWITCH_ROM);
        if (d != null) {
            d.dismiss();
        }

        switch (result) {
        case SUCCEEDED:
            createSnackbar(R.string.choose_rom_success, Snackbar.LENGTH_LONG).show();
            Log.d(TAG, "Prior cached boot partition ROM ID was: " + mActiveRomId);
            mActiveRomId = romId;
            Log.d(TAG, "Changing cached boot partition ROM ID to: " + mActiveRomId);
            mRomCardAdapter.setActiveRomId(mActiveRomId);
            mRomCardAdapter.notifyDataSetChanged();
            break;
        case FAILED:
            createSnackbar(R.string.choose_rom_failure, Snackbar.LENGTH_LONG).show();
            break;
        case CHECKSUM_INVALID:
            createSnackbar(R.string.choose_rom_checksums_invalid, Snackbar.LENGTH_LONG).show();
            showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_INVALID, romId);
            break;
        case CHECKSUM_NOT_FOUND:
            createSnackbar(R.string.choose_rom_checksums_missing, Snackbar.LENGTH_LONG).show();
            showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_MISSING, romId);
            break;
        case UNKNOWN_BOOT_PARTITION:
            showUnknownBootPartitionDialog();
            break;
        }
    }

    private void onSetKernel(String romId, SetKernelResult result) {
        // Remove cached task from service
        removeCachedTaskId(mTaskIdSetKernel);
        mTaskIdSetKernel = -1;

        mPerformingAction = false;
        updateKeepScreenOnStatus();

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

    /**
     * Wrapper function to show a {@link Snackbar}
     *
     * @param resId String resource
     * @param duration {@link Snackbar} duration
     *
     * @return {@link Snackbar} to display
     */
    private Snackbar createSnackbar(@StringRes int resId, int duration) {
        return SnackbarUtils.createSnackbar(getActivity(), mFabFlashZip, resId, duration);
    }

    private void showChecksumIssueDialog(int issue, String romId) {
        ConfirmChecksumIssueDialog d =
                ConfirmChecksumIssueDialog.newInstanceFromFragment(this, issue, romId);
        d.show(getFragmentManager(), ConfirmChecksumIssueDialog.TAG);
    }

    private void showUnknownBootPartitionDialog() {
        String codename = RomUtils.getDeviceCodename(getActivity());
        String message = String.format(getString(R.string.unknown_boot_partition), codename);

        GenericConfirmDialog gcd = GenericConfirmDialog.newInstance(null, message);
        gcd.show(getFragmentManager(), GenericConfirmDialog.TAG);
    }

    @Override
    public void onSelectedRom(RomInformation info) {
        mSelectedRom = info;

        switchRom(info.getId(), false);
    }

    @Override
    public void onSelectedRomMenu(RomInformation info) {
        mSelectedRom = info;

        Intent intent = new Intent(getActivity(), RomDetailActivity.class);
        intent.putExtra(RomDetailActivity.EXTRA_ROM_INFO, mSelectedRom);
        intent.putExtra(RomDetailActivity.EXTRA_BOOTED_ROM_INFO, mCurrentRom);
        intent.putExtra(RomDetailActivity.EXTRA_ACTIVE_ROM_ID, mActiveRomId);
        startActivity(intent);
    }

    private void switchRom(String romId, boolean forceChecksumsUpdate) {
        mPerformingAction = true;
        updateKeepScreenOnStatus();

        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.switching_rom, R.string.please_wait);
        d.show(getFragmentManager(), PROGRESS_DIALOG_SWITCH_ROM);

        mTaskIdSwitchRom = mService.switchRom(romId, forceChecksumsUpdate);
    }

    private void setKernel(RomInformation info) {
        mPerformingAction = true;
        updateKeepScreenOnStatus();

        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.setting_kernel, R.string.please_wait);
        d.show(getFragmentManager(), PROGRESS_DIALOG_SET_KERNEL);

        mTaskIdSetKernel = mService.setKernel(info.getId());
    }

    @Override
    public void onConfirmSetKernelNeeded() {
        setKernel(mCurrentRom);
    }

    @Override
    public void onConfirmChecksumIssue(String romId) {
        switchRom(romId, true);
    }

    private class SwitcherEventCallback implements GetRomsStateTaskListener, SwitchRomTaskListener,
            SetKernelTaskListener {
        @Override
        public void onGotRomsState(int taskId, final RomInformation[] roms,
                                   final RomInformation currentRom, final String activeRomId,
                                   final KernelStatus kernelStatus) {
            if (taskId == mTaskIdGetRomsState) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        SwitcherListFragment.this.onGotRomsState(
                                roms, currentRom, activeRomId, kernelStatus);
                    }
                });
            }
        }

        @Override
        public void onSwitchedRom(int taskId, final String romId, final SwitchRomResult result) {
            if (taskId == mTaskIdSwitchRom) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        SwitcherListFragment.this.onSwitchedRom(romId, result);
                    }
                });
            }
        }

        @Override
        public void onSetKernel(int taskId, final String romId, final SetKernelResult result) {
            if (taskId == mTaskIdSetKernel) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        SwitcherListFragment.this.onSetKernel(romId, result);
                    }
                });
            }
        }
    }
}
