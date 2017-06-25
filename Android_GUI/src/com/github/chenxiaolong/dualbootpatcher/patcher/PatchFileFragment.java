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
import android.app.Fragment;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.design.widget.Snackbar;
import android.support.v13.app.FragmentCompat;
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

import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.FileUtils.UriMetadata;
import com.github.chenxiaolong.dualbootpatcher.MenuUtils;
import com.github.chenxiaolong.dualbootpatcher.PermissionUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.SnackbarUtils;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog
        .GenericYesNoDialogListener;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatchFileItemAdapter
        .PatchFileItemClickListener;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherOptionsDialog
        .PatcherOptionsDialogListener;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherService.PatcherEventListener;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback
        .OnItemMovedOrDismissedListener;
import com.github.clans.fab.FloatingActionButton;
import com.github.clans.fab.FloatingActionMenu;

import org.apache.commons.io.FilenameUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;

public class PatchFileFragment extends Fragment implements
        ServiceConnection, PatcherOptionsDialogListener, OnItemMovedOrDismissedListener,
        PatchFileItemClickListener, FragmentCompat.OnRequestPermissionsResultCallback,
        GenericYesNoDialogListener {
    public static final String FRAGMENT_TAG = PatchFileFragment.class.getCanonicalName();
    private static final String TAG = PatchFileFragment.class.getSimpleName();

    private static final String DIALOG_PATCHER_OPTIONS =
            PatchFileFragment.class.getCanonicalName() + ".patcher_options";
    private static final String YES_NO_DIALOG_PERMISSIONS =
            PatchFileFragment.class.getCanonicalName() + ".yes_no.permissions";
    private static final String CONFIRM_DIALOG_PERMISSIONS =
            PatchFileFragment.class.getCanonicalName() + ".confirm.permissions";
    private static final String PROGRESS_DIALOG_QUERYING_METADATA =
            PatchFileFragment.class.getCanonicalName() + ".progress.querying_metadata";

    private static final String EXTRA_SELECTED_PATCHER_ID = "selected_patcher_id";
    private static final String EXTRA_SELECTED_INPUT_URI = "selected_input_file";
    private static final String EXTRA_SELECTED_OUTPUT_URI = "selected_output_file";
    private static final String EXTRA_SELECTED_INPUT_FILE_NAME = "selected_input_file_name";
    private static final String EXTRA_SELECTED_INPUT_FILE_SIZE = "selected_input_file_size";
    private static final String EXTRA_SELECTED_TASK_ID = "selected_task_id";
    private static final String EXTRA_SELECTED_DEVICE = "selected_device";
    private static final String EXTRA_SELECTED_ROM_ID = "selected_rom_id";
    private static final String EXTRA_QUERYING_METADATA = "querying_metadata";

    /** Request code for choosing input file */
    private static final int ACTIVITY_REQUEST_INPUT_FILE = 1000;
    /** Request code for choosing output file */
    private static final int ACTIVITY_REQUEST_OUTPUT_FILE = 1001;
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
    private FloatingActionMenu mFAB;
    private FloatingActionButton mFABAddZip;
    private FloatingActionButton mFABAddOdin;
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

    /** Selected patcher ID */
    private String mSelectedPatcherId;
    /** Selected input file */
    private Uri mSelectedInputUri;
    /** Selected output file */
    private Uri mSelectedOutputUri;
    /** Selected input file's name */
    private String mSelectedInputFileName;
    /** Selected input file's size */
    private long mSelectedInputFileSize;
    /** Task ID of selected patcher item */
    private int mSelectedTaskId;
    /** Target device */
    private Device mSelectedDevice;
    /** Target ROM ID */
    private String mSelectedRomId;
    /** Whether we're querying the URI metadata */
    private boolean mQueryingMetadata;
    /** Task for querying the metadata of URIs */
    private GetUriMetadataTask mQueryMetadataTask;

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
            mSelectedPatcherId = savedInstanceState.getString(EXTRA_SELECTED_PATCHER_ID);
            mSelectedInputUri = savedInstanceState.getParcelable(EXTRA_SELECTED_INPUT_URI);
            mSelectedOutputUri = savedInstanceState.getParcelable(EXTRA_SELECTED_OUTPUT_URI);
            mSelectedInputFileName = savedInstanceState.getString(EXTRA_SELECTED_INPUT_FILE_NAME);
            mSelectedInputFileSize = savedInstanceState.getLong(EXTRA_SELECTED_INPUT_FILE_SIZE);
            mSelectedTaskId = savedInstanceState.getInt(EXTRA_SELECTED_TASK_ID);
            mSelectedDevice = savedInstanceState.getParcelable(EXTRA_SELECTED_DEVICE);
            mSelectedRomId = savedInstanceState.getString(EXTRA_SELECTED_ROM_ID);
            mQueryingMetadata = savedInstanceState.getBoolean(EXTRA_QUERYING_METADATA);
        }

        // Initialize UI elements
        mRecycler = (RecyclerView) getActivity().findViewById(R.id.files_list);
        mFAB = (FloatingActionMenu) getActivity().findViewById(R.id.fab);
        mFABAddZip = (FloatingActionButton) getActivity().findViewById(R.id.fab_add_flashable_zip);
        mFABAddOdin = (FloatingActionButton) getActivity().findViewById(R.id.fab_add_odin_image);
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
        mFABAddZip.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                startFileSelection(PatcherUtils.PATCHER_ID_MULTIBOOTPATCHER);
                mFAB.close(true);
            }
        });
        mFABAddOdin.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                startFileSelection(PatcherUtils.PATCHER_ID_ODINPATCHER);
                mFAB.close(true);
            }
        });

        // Set up adapter for the files list
        mAdapter = new PatchFileItemAdapter(getActivity(), mItems, this);
        mRecycler.setHasFixedSize(true);
        mRecycler.setAdapter(mAdapter);

        LinearLayoutManager llm = new LinearLayoutManager(getActivity());
        llm.setOrientation(LinearLayoutManager.VERTICAL);
        mRecycler.setLayoutManager(llm);

        // Hide FAB initially
        mFAB.hideMenuButton(false);

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

        outState.putString(EXTRA_SELECTED_PATCHER_ID, mSelectedPatcherId);
        outState.putParcelable(EXTRA_SELECTED_INPUT_URI, mSelectedInputUri);
        outState.putParcelable(EXTRA_SELECTED_OUTPUT_URI, mSelectedOutputUri);
        outState.putString(EXTRA_SELECTED_INPUT_FILE_NAME, mSelectedInputFileName);
        outState.putLong(EXTRA_SELECTED_INPUT_FILE_SIZE, mSelectedInputFileSize);
        outState.putInt(EXTRA_SELECTED_TASK_ID, mSelectedTaskId);
        outState.putParcelable(EXTRA_SELECTED_DEVICE, mSelectedDevice);
        outState.putString(EXTRA_SELECTED_ROM_ID, mSelectedRomId);
        outState.putBoolean(EXTRA_QUERYING_METADATA, mQueryingMetadata);
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
                    for (int i = 0; i < mItems.size(); i++) {
                        PatchFileItem item = mItems.get(i);
                        if (item.state == PatchFileState.QUEUED) {
                            item.state = PatchFileState.PENDING;
                            mService.startPatching(item.taskId);
                            mAdapter.notifyItemChanged(i);
                        }
                    }
                }
            });
            return true;
        case R.id.cancel_item:
            executeNeedsService(new Runnable() {
                @Override
                public void run() {
                    // Cancel the tasks in reverse order since there's a chance that the next task
                    // will start when the previous one is cancelled
                    for (int i = mItems.size() - 1; i >= 0; i--) {
                        PatchFileItem item = mItems.get(i);
                        if (item.state == PatchFileState.IN_PROGRESS
                                || item.state == PatchFileState.PENDING) {
                            mService.cancelPatching(item.taskId);
                            if (item.state == PatchFileState.PENDING) {
                                item.state = PatchFileState.QUEUED;
                                mAdapter.notifyItemChanged(i);
                            }
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

        // Cancel metadata query task
        cancelQueryUriMetadata();

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
        case ACTIVITY_REQUEST_INPUT_FILE:
            if (data != null && resultCode == Activity.RESULT_OK) {
                onSelectedInputUri(data.getData());
            }
            break;
        case ACTIVITY_REQUEST_OUTPUT_FILE:
            if (data != null && resultCode == Activity.RESULT_OK) {
                onSelectedOutputUri(data.getData());
            }
            break;
        default:
            super.onActivityResult(requestCode, resultCode, data);
            break;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        switch (requestCode) {
        case PERMISSIONS_REQUEST_STORAGE:
            if (PermissionUtils.verifyPermissions(grantResults)) {
                selectInputUri();
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
            mFAB.hideMenuButton(true);
            mAddZipMessage.setVisibility(View.GONE);
            mProgressBar.setVisibility(View.VISIBLE);
        } else {
            mRecycler.setVisibility(View.VISIBLE);
            mFAB.showMenuButton(true);
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
        onReady();
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
            item.inputUri = mService.getInputUri(taskId);
            item.outputUri = mService.getOutputUri(taskId);
            item.displayName = mService.getDisplayName(taskId);
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

        // Resume URI metadata query if it was interrupted
        if (mQueryingMetadata) {
            queryUriMetadata();
        }
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

    private void startFileSelection(String patcherId) {
        mSelectedPatcherId = patcherId;

        if (PermissionUtils.supportsRuntimePermissions()) {
            requestPermissions();
        } else {
            selectInputUri();
        }
    }

    private void requestPermissions() {
        FragmentCompat.requestPermissions(PatchFileFragment.this,
                PermissionUtils.STORAGE_PERMISSIONS, PERMISSIONS_REQUEST_STORAGE);
    }

    private void showPermissionsRationaleDialogYesNo() {
        GenericYesNoDialog dialog = (GenericYesNoDialog)
                getFragmentManager().findFragmentByTag(YES_NO_DIALOG_PERMISSIONS);
        if (dialog == null) {
            GenericYesNoDialog.Builder builder = new GenericYesNoDialog.Builder();
            builder.message(R.string.patcher_storage_permission_required);
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
            builder.message(R.string.patcher_storage_permission_required);
            builder.buttonText(R.string.ok);
            dialog = builder.build();
            dialog.show(getFragmentManager(), CONFIRM_DIALOG_PERMISSIONS);
        }
    }

    /**
     * Show activity for selecting an input file
     *
     * After the user selects a file, {@link #onSelectedInputUri(Uri)} will be called. If the user
     * cancels the selection, nothing will happen.
     *
     * @see {@link #onSelectedInputUri(Uri)}
     */
    private void selectInputUri() {
        Intent intent = FileUtils.getFileOpenIntent(getActivity(), "*/*");
        startActivityForResult(intent, ACTIVITY_REQUEST_INPUT_FILE);
    }

    /**
     * Show activity for selecting an output file
     *
     * After the user confirms the patcher options, this function will be called. Once the user
     * selects the output filename, {@link #onSelectedOutputUri(Uri)} will be called. If the user
     * leaves the activity without selecting a file, nothing will happen.
     *
     * The specified ROM ID is added to the suggested filename before the file extension. For
     * example, if the original input filename was "SuperDuperROM-1.0.zip" and the ROM ID is "dual",
     * then the suggested filename is "SuperDuperROM-1.0_dual.zip".
     *
     * @see {@link #onSelectedOutputUri(Uri)}
     */
    private void selectOutputFile() {
        String baseName;
        String extension;
        if (mSelectedPatcherId.equals(PatcherUtils.PATCHER_ID_ODINPATCHER)) {
            baseName = mSelectedInputFileName.replaceAll(
                    "(\\.tar\\.md5(\\.gz|\\.xz)?|\\.zip)$", "");
            extension = "zip";
        } else {
            baseName = FilenameUtils.getBaseName(mSelectedInputFileName);
            extension = FilenameUtils.getExtension(mSelectedInputFileName);
        }
        StringBuilder sb = new StringBuilder();
        if (baseName != null) {
            sb.append(baseName);
            sb.append('_');
        }
        sb.append(mSelectedRomId);
        if (extension != null) {
            sb.append('.');
            sb.append(extension);
        }
        String desiredName = sb.toString();
        Intent intent = FileUtils.getFileSaveIntent(getActivity(), "*/*", desiredName);
        startActivityForResult(intent, ACTIVITY_REQUEST_OUTPUT_FILE);
    }

    /**
     * Query the metadata for the input file
     *
     * After the user selects an input file, this function is called to start the task of retrieving
     * the file's name and size. Once the information has been retrieved,
     * {@link #onQueriedMetadata(UriMetadata)} is called.
     *
     * @see {@link #onQueriedMetadata(UriMetadata)}
     */
    private void queryUriMetadata() {
        if (mQueryMetadataTask != null) {
            throw new IllegalStateException("Already querying metadata!");
        }
        mQueryMetadataTask = new GetUriMetadataTask();
        mQueryMetadataTask.execute(mSelectedInputUri);

        // Show progress dialog. Dialog may already exist if a configuration change occurred during
        // the query (and thus, this function is called again in onReady()).
        GenericProgressDialog dialog = (GenericProgressDialog)
                getFragmentManager().findFragmentByTag(PROGRESS_DIALOG_QUERYING_METADATA);
        if (dialog == null) {
            GenericProgressDialog.Builder builder = new GenericProgressDialog.Builder();
            builder.message(R.string.please_wait);
            dialog = builder.build();
            dialog.show(getFragmentManager(), PROGRESS_DIALOG_QUERYING_METADATA);
        }
    }

    /**
     * Cancel task for querying the input URI metadata
     *
     * This function is a no-op if there is no such task.
     *
     * @see {@link #onStop()}
     */
    private void cancelQueryUriMetadata() {
        if (mQueryMetadataTask != null) {
            mQueryMetadataTask.cancel(true);
            mQueryMetadataTask = null;
        }
    }

    /**
     * Called after the user tapped the FAB and selected a file
     *
     * This function will call {@link #queryUriMetadata()} to start querying various metadata of the
     * input URI.
     *
     * @see {@link #queryUriMetadata()}
     *
     * @param uri Input file's URI
     */
    private void onSelectedInputUri(@NonNull Uri uri) {
        mSelectedInputUri = uri;

        queryUriMetadata();
    }

    /**
     * Called after the user selects the output file
     *
     * This function will call {@link #addOrEditItem()} to either add the new patcher item or edit
     * the selected item.
     *
     * @see {@link #addOrEditItem()}
     *
     * @param uri Output file's URI
     */
    private void onSelectedOutputUri(@NonNull Uri uri) {
        mSelectedOutputUri = uri;

        addOrEditItem();
    }

    /**
     * Called after the input URI's metadata has been retrieved
     *
     * This function will open the patcher options dialog.
     *
     * @param metadata URI metadata
     *
     * @see {@link #queryUriMetadata()}
     * @see {@link #showPatcherOptionsDialog(int)}
     */
    private void onQueriedMetadata(@NonNull UriMetadata metadata) {
        GenericProgressDialog dialog = (GenericProgressDialog)
                getFragmentManager().findFragmentByTag(PROGRESS_DIALOG_QUERYING_METADATA);
        if (dialog != null) {
            dialog.dismiss();
        }

        mSelectedInputFileName = metadata.displayName;
        mSelectedInputFileSize = metadata.size;

        // Open patcher options
        showPatcherOptionsDialog(-1);
    }

    /**
     * Called after the user confirms the patcher options
     *
     * This function will call {@link #selectOutputFile()} for choosing an output file
     *
     * @see {@link #selectOutputFile()}
     *
     * @param id Task ID (-1 if new item)
     * @param device Target device
     * @param romId Target ROM ID
     */
    @Override
    public void onConfirmedOptions(final int id, final Device device, final String romId) {
        mSelectedTaskId = id;
        mSelectedDevice = device;
        mSelectedRomId = romId;

        selectOutputFile();
    }

    @Override
    public void onConfirmYesNo(@Nullable String tag, boolean choice) {
        if (YES_NO_DIALOG_PERMISSIONS.equals(tag)) {
            if (choice) {
                requestPermissions();
            }
        }
    }

    private void addOrEditItem() {
        if (mSelectedTaskId >= 0) {
            // Edit existing task
            executeNeedsService(new Runnable() {
                @Override
                public void run() {
                    int index = mItemsMap.get(mSelectedTaskId);
                    PatchFileItem item = mItems.get(index);
                    item.device = mSelectedDevice;
                    item.romId = mSelectedRomId;
                    mAdapter.notifyItemChanged(index);

                    mService.setDevice(mSelectedTaskId, mSelectedDevice);
                    mService.setRomId(mSelectedTaskId, mSelectedRomId);
                }
            });
            return;
        }

        // Do not allow two patching operations with the same output file. This is not completely
        // foolproof since two URIs can refer to the same target path, but it's the best we can do.
        for (PatchFileItem item : mItems) {
            if (item.outputUri.equals(mSelectedOutputUri)) {
                SnackbarUtils.createSnackbar(getActivity(), mFAB,
                        R.string.patcher_cannot_add_same_item,
                        Snackbar.LENGTH_LONG).show();
                return;
            }
        }

        executeNeedsService(new Runnable() {
            @Override
            public void run() {
                final PatchFileItem pf = new PatchFileItem();
                pf.patcherId = mSelectedPatcherId;
                pf.device = mSelectedDevice;
                pf.inputUri = mSelectedInputUri;
                pf.outputUri = mSelectedOutputUri;
                pf.displayName = mSelectedInputFileName;
                pf.romId = mSelectedRomId;
                pf.state = PatchFileState.QUEUED;

                int taskId = mService.addPatchFileTask(pf.patcherId, pf.inputUri, pf.outputUri,
                        pf.displayName, pf.device, pf.romId);
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
        public void onPatcherFinished(final int taskId, final PatchFileState state,
                                      final boolean ret, final int errorCode) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    if (mItemsMap.containsKey(taskId)) {
                        int itemIndex = mItemsMap.get(taskId);
                        PatchFileItem item = mItems.get(itemIndex);

                        item.state = state;
                        item.details = getString(R.string.details_done);
                        item.successful = ret;
                        item.errorCode = errorCode;

                        updateToolbarIcons();
                        updateModifiability();
                        updateScreenOnState();

                        mAdapter.notifyItemChanged(itemIndex);

                        //returnResult(ret ? RESULT_PATCHING_SUCCEEDED : RESULT_PATCHING_FAILED,
                        //        "See " + LogUtils.getPath("patch-file.log") + " for details", newPath);
                    }
                }
            });
        }
    }

    /**
     * Task to query the display name, size, and MIME type of a list of openable URIs.
     */
    private class GetUriMetadataTask extends AsyncTask<Uri, Void, UriMetadata[]> {
        private ContentResolver mCR;

        @Override
        protected void onPreExecute() {
            mCR = getActivity().getContentResolver();
            mQueryingMetadata = true;
        }

        @Override
        protected UriMetadata[] doInBackground(Uri... params) {
            return FileUtils.queryUriMetadata(mCR, params);
        }

        @Override
        protected void onPostExecute(UriMetadata[] metadatas) {
            mCR = null;

            if (isAdded()) {
                mQueryingMetadata = false;
                onQueriedMetadata(metadatas[0]);
            }
        }
    }
}
