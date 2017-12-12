/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.design.widget.Snackbar;
import android.support.v13.app.FragmentCompat;
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

import com.github.chenxiaolong.dualbootpatcher.PermissionUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.SnackbarUtils;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
        .GenericConfirmDialogListener;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog
        .GenericYesNoDialogListener;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolErrorActivity;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SwitchRomResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmChecksumIssueDialog
        .ConfirmChecksumIssueDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomCardAdapter.RomCardActionListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SetKernelNeededDialog
        .SetKernelNeededDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.KernelStatus;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomsStateTask
        .GetRomsStateTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask.SetKernelTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask.SwitchRomTaskListener;
import com.github.chenxiaolong.dualbootpatcher.views.SwipeRefreshLayoutWorkaround;
import com.github.clans.fab.FloatingActionButton;

import java.util.ArrayList;
import java.util.Collections;

public class SwitcherListFragment extends Fragment implements
        RomCardActionListener, SetKernelNeededDialogListener, ConfirmChecksumIssueDialogListener,
        GenericYesNoDialogListener, GenericConfirmDialogListener, ServiceConnection {
    public static final String FRAGMENT_TAG = SwitcherListFragment.class.getCanonicalName();
    private static final String TAG = SwitcherListFragment.class.getSimpleName();

    private static final String EXTRA_TASK_ID_GET_ROMS_STATE = "task_id_get_roms_state";
    private static final String EXTRA_TASK_ID_SWITCH_ROM = "task_id_switch_rom";
    private static final String EXTRA_TASK_ID_SET_KERNEL = "task_id_set_kernel";
    private static final String EXTRA_TASK_IDS_TO_REMOVE = "task_ids_to_reomve";

    private static final String EXTRA_PERFORMING_ACTION = "performing_action";
    private static final String EXTRA_SELECTED_ROM = "selected_rom";
    private static final String EXTRA_ACTIVE_ROM_ID = "active_rom_id";
    private static final String EXTRA_SHOWED_SET_KERNEL_WARNING = "showed_set_kernel_warning";
    private static final String EXTRA_HAVE_PERMISSIONS_RESULT = "have_permissions_result";
    private static final String EXTRA_IS_LOADING = "is_loading";

    /** Fragment tag for progress dialog for switching ROMS */
    private static final String PROGRESS_DIALOG_SWITCH_ROM =
            SwitcherListFragment.class.getCanonicalName() + ".progress.switch_rom";
    /** Fragment tag for progress dialog for setting the kernel */
    private static final String PROGRESS_DIALOG_SET_KERNEL =
            SwitcherListFragment.class.getCanonicalName() + ".progress.set_kernel";
    /** Fragment tag for confirmation dialog saying that setting the kernel is needed */
    private static final String CONFIRM_DIALOG_SET_KERNEL_NEEDED =
            SwitcherListFragment.class.getCanonicalName() + ".confirm.set_kernel_needed";
    private static final String CONFIRM_DIALOG_CHECKSUM_ISSUE =
            SwitcherListFragment.class.getCanonicalName() + ".confirm.checksum_issue";
    private static final String CONFIRM_DIALOG_UNKNOWN_BOOT_PARTITION =
            SwitcherListFragment.class.getCanonicalName() + ".confirm.unknown_boot_partition";
    private static final String CONFIRM_DIALOG_PERMISSIONS =
            SwitcherListFragment.class.getCanonicalName() + ".confirm.permissions";
    private static final String YES_NO_DIALOG_PERMISSIONS =
            SwitcherListFragment.class.getCanonicalName() + ".yes_no.permissions";

    /**
     * Request code for storage permissions request
     * (used in {@link #onRequestPermissionsResult(int, String[], int[])})
     */
    private static final int PERMISSIONS_REQUEST_STORAGE = 1;

    private boolean mHavePermissionsResult = false;

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

    /** {@link Runnable}s to process once the service has been connected */
    private ArrayList<Runnable> mExecOnConnect = new ArrayList<>();

    private static final int REQUEST_FLASH_ZIP = 2345;
    private static final int REQUEST_ROM_DETAILS = 3456;
    private static final int REQUEST_MBTOOL_ERROR = 4567;

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

    private boolean mIsInitialStart;
    private boolean mIsLoading;

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
            mHavePermissionsResult = savedInstanceState.getBoolean(
                    EXTRA_HAVE_PERMISSIONS_RESULT);

            mIsLoading = savedInstanceState.getBoolean(EXTRA_IS_LOADING);
        }

        mFabFlashZip = (FloatingActionButton) getActivity()
                .findViewById(R.id.fab_flash_zip);
        mFabFlashZip.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(getActivity(), InAppFlashingActivity.class);
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

        mIsInitialStart = savedInstanceState == null;
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

        // Permissions
        if (PermissionUtils.supportsRuntimePermissions()) {
            if (mIsInitialStart) {
                requestPermissions();
            } else if (mHavePermissionsResult) {
                if (PermissionUtils.hasPermissions(
                        getActivity(), PermissionUtils.STORAGE_PERMISSIONS)) {
                    onPermissionsGranted();
                } else {
                    onPermissionsDenied();
                }
            }
        } else {
            startWhenServiceConnected();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onStop() {
        super.onStop();

        // If we connected to the service and registered the callback, now we unregister it
        if (mService != null) {
            if (mTaskIdGetRomsState >= 0) {
                mService.removeCallback(mTaskIdGetRomsState, mCallback);
            }
            if (mTaskIdSwitchRom >= 0) {
                mService.removeCallback(mTaskIdSwitchRom, mCallback);
            }
            if (mTaskIdSetKernel >= 0) {
                mService.removeCallback(mTaskIdSetKernel, mCallback);
            }
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

        for (Runnable runnable : mExecOnConnect) {
            runnable.run();
        }
        mExecOnConnect.clear();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onServiceDisconnected(ComponentName name) {
        mService = null;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        switch (requestCode) {
        case PERMISSIONS_REQUEST_STORAGE:
            if (PermissionUtils.verifyPermissions(grantResults)) {
                onPermissionsGranted();
            } else {
                if (PermissionUtils.shouldShowRationales(this, permissions)) {
                    showPermissionsRationaleDialogYesNo();
                } else {
                    showPermissionsRationaleDialogConfirm();
                }
            }
            break;
        default:
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
            break;
        }
    }

    private void requestPermissions() {
        FragmentCompat.requestPermissions(
                this, PermissionUtils.STORAGE_PERMISSIONS, PERMISSIONS_REQUEST_STORAGE);
    }

    private void onPermissionsGranted() {
        mHavePermissionsResult = true;
        startWhenServiceConnected();
    }

    private void onPermissionsDenied() {
        mHavePermissionsResult = true;
        startWhenServiceConnected();
    }

    private void startWhenServiceConnected() {
        executeNeedsService(new Runnable() {
            @Override
            public void run() {
                onReady();
            }
        });
    }

    private void showPermissionsRationaleDialogYesNo() {
        GenericYesNoDialog dialog = (GenericYesNoDialog)
                getFragmentManager().findFragmentByTag(YES_NO_DIALOG_PERMISSIONS);
        if (dialog == null) {
            GenericYesNoDialog.Builder builder = new GenericYesNoDialog.Builder();
            builder.message(R.string.switcher_storage_permission_required);
            builder.positive(R.string.try_again);
            builder.negative(R.string.cancel);
            dialog = builder.buildFromFragment(YES_NO_DIALOG_PERMISSIONS, this);
            dialog.show(getFragmentManager(), YES_NO_DIALOG_PERMISSIONS);
        }
    }

    private void showPermissionsRationaleDialogConfirm() {
        GenericConfirmDialog dialog = (GenericConfirmDialog)
                getFragmentManager().findFragmentByTag(CONFIRM_DIALOG_PERMISSIONS);
        if (dialog == null) {
            GenericConfirmDialog.Builder builder = new GenericConfirmDialog.Builder();
            builder.message(R.string.switcher_storage_permission_required);
            builder.buttonText(R.string.ok);
            dialog = builder.buildFromFragment(CONFIRM_DIALOG_PERMISSIONS, this);
            dialog.show(getFragmentManager(), CONFIRM_DIALOG_PERMISSIONS);
        }
    }

    @Override
    public void onConfirmYesNo(@Nullable String tag, boolean choice) {
        if (YES_NO_DIALOG_PERMISSIONS.equals(tag)) {
            if (choice) {
                requestPermissions();
            } else {
                onPermissionsDenied();
            }
        }
    }

    @Override
    public void onConfirmOk(@Nullable String tag) {
        if (CONFIRM_DIALOG_PERMISSIONS.equals(tag)) {
            onPermissionsDenied();
        }
    }

    private void onReady() {
        // Remove old task IDs
        for (int taskId : mTaskIdsToRemove) {
            mService.removeCachedTask(taskId);
        }
        mTaskIdsToRemove.clear();

        if (mTaskIdGetRomsState < 0) {
            setLoadingSpinnerVisibility(true);

            // Load ROMs state asynchronously
            mTaskIdGetRomsState = mService.getRomsState();
            mService.addCallback(mTaskIdGetRomsState, mCallback);
            mService.enqueueTaskId(mTaskIdGetRomsState);
        } else {
            setLoadingSpinnerVisibility(mIsLoading);

            mService.addCallback(mTaskIdGetRomsState, mCallback);
        }

        if (mTaskIdSwitchRom >= 0) {
            mService.addCallback(mTaskIdSwitchRom, mCallback);
        }

        if (mTaskIdSetKernel >= 0) {
            mService.addCallback(mTaskIdSetKernel, mCallback);
        }
    }

    /**
     * Execute a {@link Runnable} that requires the service to be connected
     *
     * NOTE: If the service is disconnect before this method is called and does not reconnect before
     * this fragment is destroyed, then the runnable will NOT be executed.
     *
     * @param runnable Runnable that requires access to the service
     */
    public void executeNeedsService(final Runnable runnable) {
        if (mService != null) {
            runnable.run();
        } else {
            mExecOnConnect.add(runnable);
        }
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
        setFabVisibility(false, true);
        setLoadingSpinnerVisibility(true);

        removeCachedTaskId(mTaskIdGetRomsState);
        // onActivityResult() gets called before onStart() (apparently), so we'll just set the task
        // ID to -1 and wait for the natural reload in onServiceConnected().
        if (mService != null) {
            mTaskIdGetRomsState = mService.getRomsState();
            mService.addCallback(mTaskIdGetRomsState, mCallback);
            mService.enqueueTaskId(mTaskIdGetRomsState);
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
        outState.putBoolean(EXTRA_HAVE_PERMISSIONS_RESULT, mHavePermissionsResult);

        outState.putBoolean(EXTRA_IS_LOADING, mIsLoading);
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
        setFabVisibility(false, false);
    }

    /**
     * Set visibility of loading progress spinner
     *
     * @param visible Visibility
     */
    private void setLoadingSpinnerVisibility(boolean visible) {
        mIsLoading = visible;
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
    private void setFabVisibility(boolean visible, boolean animate) {
        if (visible) {
            mFabFlashZip.show(animate);
        } else {
            mFabFlashZip.hide(animate);
        }
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
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
        case REQUEST_FLASH_ZIP:
            reloadRomsState();
            break;
        case REQUEST_ROM_DETAILS:
            if (data != null && resultCode == Activity.RESULT_OK) {
                boolean wipedRom = data.getBooleanExtra(
                        RomDetailActivity.EXTRA_RESULT_WIPED_ROM, false);
                if (wipedRom) {
                    reloadRomsState();
                }
            }
            break;
        case REQUEST_MBTOOL_ERROR:
            if (data != null && resultCode == Activity.RESULT_OK) {
                boolean canRetry = data.getBooleanExtra(
                        MbtoolErrorActivity.EXTRA_RESULT_CAN_RETRY, false);
                if (canRetry) {
                    reloadRomsState();
                }
            }
            break;
        default:
            super.onActivityResult(requestCode, resultCode, data);
            break;
        }
    }

    private void onGotRomsState(RomInformation[] roms, RomInformation currentRom,
                                String activeRomId, KernelStatus kernelStatus) {
        mRoms.clear();

        if (roms != null && roms.length > 0) {
            Collections.addAll(mRoms, roms);
            setFabVisibility(true, true);
        } else {
            setErrorVisibility(true);
            setFabVisibility(false, true);
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
                    dialog.show(getFragmentManager(), CONFIRM_DIALOG_SET_KERNEL_NEEDED);
                } else if (kernelStatus == KernelStatus.UNSET) {
                    SetKernelNeededDialog dialog = SetKernelNeededDialog.newInstance(
                            this, R.string.kernel_not_set_desc);
                    dialog.show(getFragmentManager(), CONFIRM_DIALOG_SET_KERNEL_NEEDED);
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
        d.show(getFragmentManager(), CONFIRM_DIALOG_CHECKSUM_ISSUE);
    }

    private void showUnknownBootPartitionDialog() {
        String codename = RomUtils.getDeviceCodename(getActivity());
        String message = String.format(getString(R.string.unknown_boot_partition), codename);

        GenericConfirmDialog.Builder builder = new GenericConfirmDialog.Builder();
        builder.message(message);
        builder.buttonText(R.string.ok);
        builder.build().show(getFragmentManager(), CONFIRM_DIALOG_UNKNOWN_BOOT_PARTITION);
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
        startActivityForResult(intent, REQUEST_ROM_DETAILS);
    }

    private void switchRom(String romId, boolean forceChecksumsUpdate) {
        mPerformingAction = true;
        updateKeepScreenOnStatus();

        GenericProgressDialog.Builder builder = new GenericProgressDialog.Builder();
        builder.title(R.string.switching_rom);
        builder.message(R.string.please_wait);
        builder.build().show(getFragmentManager(), PROGRESS_DIALOG_SWITCH_ROM);

        mTaskIdSwitchRom = mService.switchRom(romId, forceChecksumsUpdate);
        mService.addCallback(mTaskIdSwitchRom, mCallback);
        mService.enqueueTaskId(mTaskIdSwitchRom);
    }

    private void setKernel(RomInformation info) {
        mPerformingAction = true;
        updateKeepScreenOnStatus();

        GenericProgressDialog.Builder builder = new GenericProgressDialog.Builder();
        builder.title(R.string.setting_kernel);
        builder.message(R.string.please_wait);
        builder.build().show(getFragmentManager(), PROGRESS_DIALOG_SET_KERNEL);

        mTaskIdSetKernel = mService.setKernel(info.getId());
        mService.addCallback(mTaskIdSetKernel, mCallback);
        mService.enqueueTaskId(mTaskIdSetKernel);
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

        @Override
        public void onMbtoolConnectionFailed(int taskId, final Reason reason) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    Intent intent = new Intent(getActivity(), MbtoolErrorActivity.class);
                    intent.putExtra(MbtoolErrorActivity.EXTRA_REASON, reason);
                    startActivityForResult(intent, REQUEST_MBTOOL_ERROR);
                }
            });
        }
    }
}
