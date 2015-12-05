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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.app.Activity;
import android.app.DialogFragment;
import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.media.MediaScannerConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.support.annotation.NonNull;
import android.support.design.widget.Snackbar;
import android.support.v4.content.ContextCompat;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.helper.ItemTouchHelper;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.getbase.floatingactionbutton.FloatingActionButton;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.MenuUtils;
import com.github.chenxiaolong.dualbootpatcher.PermissionUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.SnackbarUtils;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherOptionsDialog
        .PatcherOptionsDialogListener;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback
        .OnItemMovedOrDismissedListener;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;

public class PatchFileFragment extends Fragment implements
        ServiceConnection, PatcherOptionsDialogListener, OnItemMovedOrDismissedListener {
    public static final String TAG = PatchFileFragment.class.getSimpleName();

    /** Intent filter for messages we care about from the service */
    private static final IntentFilter SERVICE_INTENT_FILTER = new IntentFilter();

    /** Request code for file picker (used in {@link #onActivityResult(int, int, Intent)}) */
    private static final int ACTIVITY_REQUEST_FILE = 1000;
    /**
     * Request code for storage permissions request
     * (used in {@link #onRequestPermissionsResult(int, String[], int[])})
     */
    private static final int PERMISSIONS_REQUEST_STORAGE = 1;

    /** Whether we should show the progress bar (true by default for obvious reasons) */
    private boolean mShowingProgress = true;

    /** Main files list */
    private RecyclerView mRecycler;
    /** FAB */
    private FloatingActionButton mFAB;
    /** Loading progress spinner */
    private ProgressBar mProgressBar;
    /** Add zip message */
    private TextView mAddZipMessage;

    /** Check icon in the toolbar */
    private MenuItem mCheckItem;
    /** Cancel icon in the toolbar */
    private MenuItem mCancelItem;

    /** Adapter for the list of files to patch */
    private PatchFileItemAdapter mAdapter;

    /** Our patcher service */
    private PatcherService mService;
    /** Broadcast receiver for events from the service */
    private PatcherEventReceiver mReceiver = new PatcherEventReceiver();

    /** {@link Runnable}s to process once the service has been connected */
    private ArrayList<Runnable> mExecOnConnect = new ArrayList<>();

    /** Whether we're initialized */
    private boolean mInitialized;

    /** List of patcher items (pending, in progress, or complete) */
    private ArrayList<PatchFileItem> mItems = new ArrayList<>();
    /** Map task IDs to item indexes */
    private HashMap<Integer, Integer> mItemsMap = new HashMap<>();

    /** Item touch callback for dragging and swiping */
    DragSwipeItemTouchCallback mItemTouchCallback;

    static {
        // We only care about file patching events
        SERVICE_INTENT_FILTER.addAction(PatcherService.ACTION_PATCHER_INITIALIZED);
        SERVICE_INTENT_FILTER.addAction(PatcherService.ACTION_PATCHER_DETAILS_CHANGED);
        SERVICE_INTENT_FILTER.addAction(PatcherService.ACTION_PATCHER_PROGRESS_CHANGED);
        SERVICE_INTENT_FILTER.addAction(PatcherService.ACTION_PATCHER_FILES_PROGRESS_CHANGED);
        SERVICE_INTENT_FILTER.addAction(PatcherService.ACTION_PATCHER_STARTED);
        SERVICE_INTENT_FILTER.addAction(PatcherService.ACTION_PATCHER_FINISHED);
    }





    public static PatchFileFragment newInstance() {
        return new PatchFileFragment();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setHasOptionsMenu(true);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        // Initialize UI elements
        mRecycler = (RecyclerView) getActivity().findViewById(R.id.files_list);
        mFAB = (FloatingActionButton) getActivity().findViewById(R.id.fab);
        mProgressBar = (ProgressBar) getActivity().findViewById(R.id.loading);
        mAddZipMessage = (TextView) getActivity().findViewById(R.id.add_zip_message);

        mItemTouchCallback = new DragSwipeItemTouchCallback(this);
        ItemTouchHelper itemTouchHelper = new ItemTouchHelper(mItemTouchCallback);
        itemTouchHelper.attachToRecyclerView(mRecycler);

        // Set up listener for the FAB
        mFAB.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                // Show file chooser
                Intent intent = FileUtils.getFileChooserIntent(getActivity());
                if (intent == null) {
                    FileUtils.showMissingFileChooserDialog(getActivity(), getFragmentManager());
                } else {
                    startActivityForResult(intent, ACTIVITY_REQUEST_FILE);
                }
            }
        });

        // Set up adapter for the files list
        mAdapter = new PatchFileItemAdapter(getActivity(), mItems);
        mRecycler.setHasFixedSize(true);
        mRecycler.setAdapter(mAdapter);

        LinearLayoutManager llm = new LinearLayoutManager(getActivity());
        llm.setOrientation(LinearLayoutManager.VERTICAL);
        mRecycler.setLayoutManager(llm);

        // Show loading progress bar
        updateLoadingStatus();

        // Initialize the patcher once the service is connected
        executeNeedsService(new Runnable() {
            @Override
            public void run() {
                mService.initializePatcher();
            }
        });

        // NOTE: No further loading should be done here. All initialization should be done in
        // onPatcherLoaded(), which is called once the patcher's data files have been extracted and
        // loaded.
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_patcher, container, false);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {
        inflater.inflate(R.menu.actionbar_check_cancel, menu);

        int primary = ContextCompat.getColor(getActivity(), R.color.text_color_primary);
        MenuUtils.tintAllMenuIcons(menu, primary);

        mCheckItem = menu.findItem(R.id.check_item);
        mCancelItem = menu.findItem(R.id.cancel_item);

        updateToolbarIcons();

        super.onCreateOptionsMenu(menu, inflater);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case R.id.check_item:
            executeNeedsService(new Runnable() {
                @Override
                public void run() {
                    for (PatchFileItem item : mItems) {
                        if (item.state == PatchFileState.QUEUED) {
                            mService.startPatching(item.taskId);
                        }
                    }
                }
            });
            return true;
        case R.id.cancel_item:
            executeNeedsService(new Runnable() {
                @Override
                public void run() {
                    for (PatchFileItem item : mItems) {
                        if (item.state == PatchFileState.IN_PROGRESS) {
                            mService.cancelPatching(item.taskId);
                        }
                    }
                }
            });
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onStart() {
        super.onStart();

        // Bind to our service. We start the service so it doesn't get killed when all the clients
        // unbind from the service. The service will automatically stop once all clients have
        // unbinded and all tasks have completed.
        Intent intent = new Intent(getActivity(), PatcherService.class);
        getActivity().bindService(intent, this, Context.BIND_AUTO_CREATE);
        getActivity().startService(intent);

        // Register our broadcast receiver for our service
        LocalBroadcastManager.getInstance(getActivity()).registerReceiver(
                mReceiver, SERVICE_INTENT_FILTER);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onStop() {
        super.onStop();

        // Unbind from our service
        getActivity().unbindService(this);

        // Unregister the broadcast receiver
        LocalBroadcastManager.getInstance(getActivity()).unregisterReceiver(mReceiver);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onServiceConnected(ComponentName name, IBinder service) {
        // Save a reference to the service so we can interact with it
        ThreadPoolServiceBinder binder = (ThreadPoolServiceBinder) service;
        mService = (PatcherService) binder.getService();

        for (Runnable runnable : mExecOnConnect) {
            runnable.run();
        }
        mExecOnConnect.clear();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onServiceDisconnected(ComponentName componentName) {
        mService = null;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
        case ACTIVITY_REQUEST_FILE:
            if (data != null && resultCode == Activity.RESULT_OK) {
                final String file = FileUtils.getPathFromUri(getActivity(), data.getData());
                onSelectedFile(file);
            }
            break;
        }

        super.onActivityResult(requestCode, resultCode, data);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        switch (requestCode) {
        case PERMISSIONS_REQUEST_STORAGE: {
            for (int grantResult : grantResults) {
                if (grantResult != PackageManager.PERMISSION_GRANTED) {
                    Log.e(TAG, "Storage permissions were denied");
                    DialogFragment d = GenericConfirmDialog.newInstance(
                            0, R.string.patcher_storage_permission_required);
                    d.show(getFragmentManager(), "storage_permissions_denied");

                    // TODO: Re-request permissions
                    //PermissionUtils.clearRequestedCache();
                    return;
                }
            }

            onReady();
            break;
        }
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onItemMoved(int fromPosition, int toPosition) {
        // Update index map
        PatchFileItem fromItem = mItems.get(fromPosition);
        PatchFileItem toItem = mItems.get(toPosition);
        mItemsMap.put(fromItem.taskId, toPosition);
        mItemsMap.put(toItem.taskId, fromPosition);

        Collections.swap(mItems, fromPosition, toPosition);
        mAdapter.notifyItemMoved(fromPosition, toPosition);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onItemDismissed(int position) {
        final PatchFileItem item = mItems.get(position);
        mItemsMap.remove(item.taskId);

        mItems.remove(position);
        mAdapter.notifyItemRemoved(position);

        executeNeedsService(new Runnable() {
            @Override
            public void run() {
                mService.removePatchFileTask(item.taskId);
            }
        });
    }

    /**
     * Toggle main UI and progress bar visibility depending on {@link #mShowingProgress}
     */
    private void updateLoadingStatus() {
        if (mShowingProgress) {
            mRecycler.setVisibility(View.GONE);
            mFAB.setVisibility(View.GONE);
            mAddZipMessage.setVisibility(View.GONE);
            mProgressBar.setVisibility(View.VISIBLE);
        } else {
            mRecycler.setVisibility(View.VISIBLE);
            mFAB.setVisibility(View.VISIBLE);
            mAddZipMessage.setVisibility(View.VISIBLE);
            mProgressBar.setVisibility(View.GONE);
        }
    }

    /**
     * Called when the patcher data and libmbp have been initialized
     *
     * This method is guaranteed to be called only once during between onStart() and onStop()
     */
    private void onPatcherInitialized() {
        Log.d(TAG, "Patcher has been initialized");

        // Check if we have storage permissions
        if (PermissionUtils.hasPermissions(getActivity(), PermissionUtils.STORAGE_PERMISSIONS)) {
            // If we have permissions, then all initialization is complete
            onReady();
        } else {
            // Otherwise, request the permissions. onReady() will be called by
            // onRequestPermissionsResult() if permissions are granted
            PermissionUtils.requestPermissions(getActivity(), this,
                    PermissionUtils.STORAGE_PERMISSIONS, PERMISSIONS_REQUEST_STORAGE);
        }
    }

    /**
     * Called when everything has been initialized and we have the necessary permissions
     */
    private void onReady() {
        // Load patch file items from the service
        int[] taskIds = mService.getPatchFileTaskIds();
        Arrays.sort(taskIds);
        for (int taskId : taskIds) {
            PatchFileItem item = new PatchFileItem();
            item.taskId = taskId;
            item.patcherId = mService.getPatcherId(taskId);
            item.path = mService.getPath(taskId);
            item.device = mService.getDevice(taskId);
            item.romId = mService.getRomId(taskId);
            item.state = mService.getState(taskId);
            item.details = mService.getDetails(taskId);
            item.bytes = mService.getCurrentBytes(taskId);
            item.maxBytes = mService.getMaximumBytes(taskId);
            item.files = mService.getCurrentFiles(taskId);
            item.maxFiles = mService.getMaximumFiles(taskId);
            item.successful = mService.isSuccessful(taskId);
            item.errorCode = mService.getErrorCode(taskId);
            item.newPath = mService.getNewPath(taskId);

            mItems.add(item);
            mItemsMap.put(taskId, mItems.size() - 1);
        }
        mAdapter.notifyDataSetChanged();

        // We are now fully initialized. Hide the loading spinner
        mShowingProgress = false;
        updateLoadingStatus();

        // Hide add zip message if we've already added a zip
        updateAddZipMessage();

        updateToolbarIcons();
        updateModifiability();
        updateScreenOnState();
    }

    private void updateAddZipMessage() {
        mAddZipMessage.setVisibility(mItems.isEmpty() ? View.VISIBLE : View.GONE);
    }

    private void updateToolbarIcons() {
        if (mCheckItem != null && mCancelItem != null) {
            boolean checkVisible = false;
            boolean cancelVisible = false;
            for (PatchFileItem item : mItems) {
                if (item.state == PatchFileState.QUEUED) {
                    checkVisible = true;
                } else if (item.state == PatchFileState.IN_PROGRESS) {
                    checkVisible = false;
                    cancelVisible = true;
                    break;
                }
            }
            mCheckItem.setVisible(checkVisible);
            mCancelItem.setVisible(cancelVisible);
        }
    }

    private void updateModifiability() {
        boolean canModify = true;
        for (PatchFileItem item : mItems) {
            if (item.state == PatchFileState.PENDING || item.state == PatchFileState.IN_PROGRESS) {
                canModify = false;
                break;
            }
        }
        mItemTouchCallback.setLongPressDragEnabled(canModify);
        mItemTouchCallback.setItemViewSwipeEnabled(canModify);
    }

    private void updateScreenOnState() {
        boolean keepScreenOn = false;
        for (PatchFileItem item : mItems) {
            if (item.state == PatchFileState.PENDING || item.state == PatchFileState.IN_PROGRESS) {
                keepScreenOn = true;
                break;
            }
        }
        if (keepScreenOn) {
            getActivity().getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        } else {
            getActivity().getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }
    }

    /**
     * Called after the user tapped the FAB and selected a file
     */
    private void onSelectedFile(String file) {
        // Ask for patcher options
        PatcherOptionsDialog dialog = PatcherOptionsDialog.newInstanceFromFragment(this, file);
        dialog.show(getFragmentManager(), PatcherOptionsDialog.TAG);
    }

    @Override
    public void onConfirmedOptions(final String path, final Device device, final String romId) {
        // Do not allow two patching operations with the same target filename (i.e. same file and
        // ROM ID)
        for (PatchFileItem item : mItems) {
            if (item.path.equals(path) && item.romId.equals(romId)) {
                SnackbarUtils.createSnackbar(getActivity(), mFAB,
                        getString(R.string.patcher_cannot_add_same_item, romId),
                        Snackbar.LENGTH_LONG).show();
                return;
            }
        }

        executeNeedsService(new Runnable() {
            @Override
            public void run() {
                final PatchFileItem pf = new PatchFileItem();
                pf.patcherId = "MultiBootPatcher";
                pf.device = device;
                pf.path = path;
                pf.romId = romId;
                pf.state = PatchFileState.QUEUED;

                int taskId = mService.addPatchFileTask(pf.patcherId, pf.path, pf.device, pf.romId);
                pf.taskId = taskId;

                mItems.add(pf);
                mItemsMap.put(taskId, mItems.size() - 1);
                mAdapter.notifyItemInserted(mItems.size() - 1);
                updateAddZipMessage();
                updateToolbarIcons();
            }
        });
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

    private class PatcherEventReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();

            if (PatcherService.ACTION_PATCHER_INITIALIZED.equals(action)) {
                // Make sure we don't initialize more than once. This event could be sent more than
                // once if, eg., mService.initializePatcher() is called and the device is rotated
                // before this event is received. Then mService.initializePatcher() will be called
                // again leading to a duplicate event.
                if (!mInitialized) {
                    mInitialized = true;
                    onPatcherInitialized();
                }
                return;
            }

            // Status updates always send a task ID. If it does, get the corresponding item
            PatchFileItem item;
            int itemIndex;
            int taskId = intent.getIntExtra(PatcherService.RESULT_PATCHER_TASK_ID, -1);
            if (taskId != -1 && mItemsMap.containsKey(taskId)) {
                itemIndex = mItemsMap.get(taskId);
                item = mItems.get(itemIndex);
            } else {
                // Something went horribly wrong with the service
                return;
            }

            if (PatcherService.ACTION_PATCHER_DETAILS_CHANGED.equals(action)) {
                item.details = intent.getStringExtra(PatcherService.RESULT_PATCHER_DETAILS);
                mAdapter.notifyItemChanged(itemIndex);
            } else if (PatcherService.ACTION_PATCHER_PROGRESS_CHANGED.equals(action)) {
                item.bytes = intent.getLongExtra(PatcherService.RESULT_PATCHER_CURRENT_BYTES, 0);
                item.maxBytes = intent.getLongExtra(PatcherService.RESULT_PATCHER_MAX_BYTES, 0);
                mAdapter.notifyItemChanged(itemIndex);
            } else if (PatcherService.ACTION_PATCHER_FILES_PROGRESS_CHANGED.equals(action)) {
                item.files = intent.getLongExtra(PatcherService.RESULT_PATCHER_CURRENT_FILES, 0);
                item.maxFiles = intent.getLongExtra(PatcherService.RESULT_PATCHER_MAX_FILES, 0);
                mAdapter.notifyItemChanged(itemIndex);
            } else if (PatcherService.ACTION_PATCHER_STARTED.equals(action)) {
                item.state = PatchFileState.IN_PROGRESS;
                updateToolbarIcons();
                updateModifiability();
                updateScreenOnState();
                mAdapter.notifyItemChanged(itemIndex);
            } else if (PatcherService.ACTION_PATCHER_FINISHED.equals(action)) {
                boolean cancelled = intent.getBooleanExtra(
                        PatcherService.RESULT_PATCHER_CANCELLED, false);
                if (cancelled) {
                    item.state = PatchFileState.CANCELLED;
                } else {
                    item.state = PatchFileState.COMPLETED;
                }
                item.details = getString(R.string.details_done);
                item.successful = intent.getBooleanExtra(PatcherService.RESULT_PATCHER_SUCCESS,
                        false);
                item.errorCode = intent.getIntExtra(PatcherService.RESULT_PATCHER_ERROR_CODE, 0);
                item.newPath = intent.getStringExtra(PatcherService.RESULT_PATCHER_NEW_FILE);

                updateToolbarIcons();
                updateModifiability();
                updateScreenOnState();

                mAdapter.notifyItemChanged(itemIndex);

                // Update MTP cache
                if (item.successful) {
                    // TODO: Android has a bug that causes the context to leak!
                    // TODO: See https://github.com/square/leakcanary/issues/26
                    MediaScannerConnection.scanFile(getActivity().getApplicationContext(),
                            new String[] { item.newPath }, null, null);
                }

                //returnResult(ret ? RESULT_PATCHING_SUCCEEDED : RESULT_PATCHING_FAILED,
                //        "See " + LogUtils.getPath("patch-file.log") + " for details", newPath);
            } else {
                throw new IllegalStateException("Invalid action: " + action);
            }
        }
    }
}
