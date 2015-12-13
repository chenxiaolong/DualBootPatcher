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
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.media.MediaScannerConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.design.widget.Snackbar;
import android.support.v4.content.ContextCompat;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ItemAnimator;
import android.support.v7.widget.SimpleItemAnimator;
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
import com.github.chenxiaolong.dualbootpatcher.patcher.PatchFileItemAdapter
        .PatchFileItemClickListener;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherOptionsDialog
        .PatcherOptionsDialogListener;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherService.PatcherEventListener;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback
        .OnItemMovedOrDismissedListener;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;

public class PatchFileFragment extends Fragment implements
        ServiceConnection, PatcherOptionsDialogListener, OnItemMovedOrDismissedListener, PatchFileItemClickListener {
    public static final String TAG = PatchFileFragment.class.getSimpleName();

    private static final String DIALOG_PATCHER_OPTIONS =
            PatchFileFragment.class.getCanonicalName() + ".patcher_options";

    private static final String EXTRA_SELECTED_FILE = "selected_file";

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
    /** Callback for events from the service */
    private PatcherEventCallback mCallback = new PatcherEventCallback();

    /** Handler for processing events from the service on the UI thread */
    private final Handler mHandler = new Handler(Looper.getMainLooper());

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

    /** Selected file */
    private String mSelectedFile;

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

        if (savedInstanceState != null) {
            mSelectedFile = savedInstanceState.getString(EXTRA_SELECTED_FILE);
        }

        // Initialize UI elements
        mRecycler = (RecyclerView) getActivity().findViewById(R.id.files_list);
        mFAB = (FloatingActionButton) getActivity().findViewById(R.id.fab);
        mProgressBar = (ProgressBar) getActivity().findViewById(R.id.loading);
        mAddZipMessage = (TextView) getActivity().findViewById(R.id.add_zip_message);

        mItemTouchCallback = new DragSwipeItemTouchCallback(this);
        ItemTouchHelper itemTouchHelper = new ItemTouchHelper(mItemTouchCallback);
        itemTouchHelper.attachToRecyclerView(mRecycler);

        // Disable change animation since we frequently update the progress, which makes the
        // animation very ugly
        ItemAnimator animator = mRecycler.getItemAnimator();
        if (animator instanceof SimpleItemAnimator) {
            ((SimpleItemAnimator) animator).setSupportsChangeAnimations(false);
        }

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
        mAdapter = new PatchFileItemAdapter(getActivity(), mItems, this);
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
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putString(EXTRA_SELECTED_FILE, mSelectedFile);
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

        // NOTE: May crash on some versions of Android due to a bug where getActivity() returns null
        // after onAttach() has been called, but before onDetach() has been called. It's similar to
        // this bug report, except it happens with android.app.Fragment:
        // https://code.google.com/p/android/issues/detail?id=67519
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
        mService = (PatcherService) binder.getService();

        // Register callback
        mService.registerCallback(mCallback);

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

        updateAddZipMessage();
        updateToolbarIcons();

        executeNeedsService(new Runnable() {
            @Override
            public void run() {
                mService.removePatchFileTask(item.taskId);
            }
        });
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onPatchFileItemClicked(PatchFileItem item) {
        showPatcherOptionsDialog(item.taskId);
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

    private void showPatcherOptionsDialog(int taskId) {
        String preselectedDeviceId = null;
        String preselectedRomId = null;

        if (taskId >= 0) {
            PatchFileItem item = mItems.get(mItemsMap.get(taskId));
            preselectedDeviceId = item.device.getId();
            preselectedRomId = item.romId;
        }

        PatcherOptionsDialog dialog = PatcherOptionsDialog.newInstanceFromFragment(
                this, taskId, preselectedDeviceId, preselectedRomId);
        dialog.show(getFragmentManager(), DIALOG_PATCHER_OPTIONS);
    }

    /**
     * Called after the user tapped the FAB and selected a file
     */
    private void onSelectedFile(String file) {
        mSelectedFile = file;

        // Ask for patcher options
        showPatcherOptionsDialog(-1);
    }

    @Override
    public void onConfirmedOptions(final int id, final Device device, final String romId) {
        if (id >= 0) {
            // Edit existing task
            executeNeedsService(new Runnable() {
                @Override
                public void run() {
                    int index = mItemsMap.get(id);
                    PatchFileItem item = mItems.get(index);
                    item.device = device;
                    item.romId = romId;
                    mAdapter.notifyItemChanged(index);

                    mService.setDevice(id, device);
                    mService.setRomId(id, romId);

                }
            });
            return;
        }

        // Do not allow two patching operations with the same target filename (i.e. same file and
        // ROM ID)
        for (PatchFileItem item : mItems) {
            if (item.path.equals(mSelectedFile) && item.romId.equals(romId)) {
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
                pf.path = mSelectedFile;
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

    private class PatcherEventCallback implements PatcherEventListener {
        @Override
        public void onPatcherInitialized() {
            // Make sure we don't initialize more than once. This event could be sent more than
            // once if, eg., mService.initializePatcher() is called and the device is rotated
            // before this event is received. Then mService.initializePatcher() will be called
            // again leading to a duplicate event.
            if (!mInitialized) {
                mInitialized = true;
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        PatchFileFragment.this.onPatcherInitialized();
                    }
                });
            }
        }

        @Override
        public void onPatcherUpdateDetails(final int taskId, final String details) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mItemsMap.containsKey(taskId)) {
                        int itemIndex = mItemsMap.get(taskId);
                        PatchFileItem item = mItems.get(itemIndex);
                        item.details = details;
                        mAdapter.notifyItemChanged(itemIndex);
                    }
                }
            });
        }

        @Override
        public void onPatcherUpdateProgress(final int taskId, final long bytes,
                                            final long maxBytes) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mItemsMap.containsKey(taskId)) {
                        int itemIndex = mItemsMap.get(taskId);
                        PatchFileItem item = mItems.get(itemIndex);
                        item.bytes = bytes;
                        item.maxBytes = maxBytes;
                        mAdapter.notifyItemChanged(itemIndex);
                    }
                }
            });
        }

        @Override
        public void onPatcherUpdateFilesProgress(final int taskId, final long files,
                                                 final long maxFiles) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mItemsMap.containsKey(taskId)) {
                        int itemIndex = mItemsMap.get(taskId);
                        PatchFileItem item = mItems.get(itemIndex);
                        item.files = files;
                        item.maxFiles = maxFiles;
                        mAdapter.notifyItemChanged(itemIndex);
                    }
                }
            });
        }

        @Override
        public void onPatcherStarted(final int taskId) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mItemsMap.containsKey(taskId)) {
                        int itemIndex = mItemsMap.get(taskId);
                        PatchFileItem item = mItems.get(itemIndex);
                        item.state = PatchFileState.IN_PROGRESS;
                        updateToolbarIcons();
                        updateModifiability();
                        updateScreenOnState();
                        mAdapter.notifyItemChanged(itemIndex);
                    }
                }
            });
        }

        @Override
        public void onPatcherFinished(final int taskId, final boolean cancelled, final boolean ret,
                                      final int errorCode, final String newPath) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mItemsMap.containsKey(taskId)) {
                        int itemIndex = mItemsMap.get(taskId);
                        PatchFileItem item = mItems.get(itemIndex);

                        if (cancelled) {
                            item.state = PatchFileState.CANCELLED;
                        } else {
                            item.state = PatchFileState.COMPLETED;
                        }
                        item.details = getString(R.string.details_done);
                        item.successful = ret;
                        item.errorCode = errorCode;
                        item.newPath = newPath;

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
                    }
                }
            });
        }
    }
}
