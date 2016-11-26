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
import android.app.LoaderManager.LoaderCallbacks;
import android.content.AsyncTaskLoader;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.Loader;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.support.annotation.NonNull;
import android.support.v7.widget.CardView;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.support.v7.widget.helper.ItemTouchHelper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.Constants;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.FileUtils.UriMetadata;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog.FirstUseDialogListener;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericSingleChoiceDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericSingleChoiceDialog
        .GenericSingleChoiceDialogListener;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.switcher.BackupRestoreTargetsSelectionDialog
        .BackupRestoreTargetsSelectionDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.ChangeInstallLocationDialog
        .ChangeInstallLocationDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.InAppFlashingFragment.LoaderResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.NamedSlotIdInputDialog
        .NamedSlotIdInputDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomIdSelectionDialog
        .RomIdSelectionDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomIdSelectionDialog.RomIdType;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams.Action;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction.Type;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.RomInstallerParams;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.VerifyZipTask.VerifyZipTaskListener;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback
        .OnItemMovedOrDismissedListener;
import com.github.clans.fab.FloatingActionButton;
import com.github.clans.fab.FloatingActionMenu;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class InAppFlashingFragment extends Fragment implements FirstUseDialogListener,
        RomIdSelectionDialogListener, NamedSlotIdInputDialogListener,
        ChangeInstallLocationDialogListener, GenericSingleChoiceDialogListener,
        BackupRestoreTargetsSelectionDialogListener, LoaderCallbacks<LoaderResult>,
        ServiceConnection, OnItemMovedOrDismissedListener {
    private static final int PERFORM_ACTIONS = 1234;

    private static final String EXTRA_PENDING_ACTIONS = "pending_actions";
    private static final String EXTRA_SELECTED_URI = "selected_uri";
    private static final String EXTRA_SELECTED_URI_FILE_NAME = "selected_uri_file_name";
    private static final String EXTRA_SELECTED_BACKUP_DIR = "selected_backup_dir";
    private static final String EXTRA_SELECTED_BACKUP_NAME = "selected_backup_name";
    private static final String EXTRA_SELECTED_BACKUP_TARGETS = "selected_backup_targets";
    private static final String EXTRA_SELECTED_ROM_ID = "selected_rom_id";
    private static final String EXTRA_ZIP_ROM_ID = "zip_rom_id";
    private static final String EXTRA_ADD_TYPE = "add_type";
    private static final String EXTRA_TASK_ID_VERIFY_ZIP = "task_id_verify_zip";
    private static final String EXTRA_QUERYING_METADATA = "querying_metadata";

    private static final String PREF_SHOW_FIRST_USE_DIALOG = "zip_flashing_first_use_show_dialog";

    /** Request code for file picker (used in {@link #onActivityResult(int, int, Intent)}) */
    private static final int ACTIVITY_REQUEST_FILE = 1000;

    private static final String PROGRESS_DIALOG_VERIFY_ZIP =
            InAppFlashingFragment.class.getCanonicalName() + ".progress.verify_zip";
    private static final String PROGRESS_DIALOG_QUERYING_METADATA =
            InAppFlashingFragment.class.getCanonicalName() + ".progress.querying_metadata";
    private static final String CONFIRM_DIALOG_FIRST_USE =
            InAppFlashingFragment.class.getCanonicalName() + ".confirm.first_use";
    private static final String CONFIRM_DIALOG_INSTALL_LOCATION =
            InAppFlashingFragment.class.getCanonicalName() + ".confirm.install_location";
    private static final String CONFIRM_DIALOG_NAMED_SLOT_ID =
            InAppFlashingFragment.class.getCanonicalName() + ".confirm.named_slot_id";
    private static final String CONFIRM_DIALOG_ROM_ID =
            InAppFlashingFragment.class.getCanonicalName() + ".confirm.rom_id";
    private static final String CONFIRM_DIALOG_ERROR =
            InAppFlashingFragment.class.getCanonicalName() + ".confirm.error";
    private static final String CONFIRM_DIALOG_SELECT_BACKUP =
            InAppFlashingFragment.class.getCanonicalName() + ".confirm.select_backup";
    private static final String CONFIRM_DIALOG_SELECT_TARGETS =
            InAppFlashingFragment.class.getCanonicalName() + ".confirm.select_targets";

    private OnReadyStateChangedListener mActivityCallback;

    private SharedPreferences mPrefs;

    private Uri mSelectedUri;
    private String mSelectedUriFileName;
    private String mSelectedBackupDir;
    private String mSelectedBackupName;
    private String[] mSelectedBackupTargets;
    private String mSelectedRomId;
    private String mCurrentRomId;
    private String mZipRomId;
    private Type mAddType;

    private ProgressBar mProgressBar;

    private ArrayList<MbtoolAction> mPendingActions = new ArrayList<>();
    private PendingActionCardAdapter mAdapter;

    private ArrayList<RomInformation> mBuiltinRoms = new ArrayList<>();

    private boolean mVerifyZipOnServiceConnected;

    private int mTaskIdVerifyZip = -1;

    /** Task IDs to remove */
    private ArrayList<Integer> mTaskIdsToRemove = new ArrayList<>();

    /** Switcher service */
    private SwitcherService mService;
    /** Callback for events from the service */
    private final SwitcherEventCallback mCallback = new SwitcherEventCallback();

    /** Handler for processing events from the service on the UI thread */
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    /** Whether we're querying the URI metadata */
    private boolean mQueryingMetadata;
    /** Task for querying the metadata of URIs */
    private GetUriMetadataTask mQueryMetadataTask;

    public interface OnReadyStateChangedListener {
        void onReady(boolean ready);

        void onFinished();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_in_app_flashing, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (savedInstanceState != null) {
            ArrayList<MbtoolAction> savedActions =
                    savedInstanceState.getParcelableArrayList(EXTRA_PENDING_ACTIONS);
            mPendingActions.addAll(savedActions);
        }

        mProgressBar = (ProgressBar) getActivity().findViewById(R.id.card_list_loading);
        RecyclerView cardListView = (RecyclerView) getActivity().findViewById(R.id.card_list);

        mAdapter = new PendingActionCardAdapter(getActivity(), mPendingActions);
        cardListView.setHasFixedSize(true);
        cardListView.setAdapter(mAdapter);

        DragSwipeItemTouchCallback itemTouchCallback = new DragSwipeItemTouchCallback(this);
        ItemTouchHelper itemTouchHelper = new ItemTouchHelper(itemTouchCallback);
        itemTouchHelper.attachToRecyclerView(cardListView);

        LinearLayoutManager llm = new LinearLayoutManager(getActivity());
        llm.setOrientation(LinearLayoutManager.VERTICAL);
        cardListView.setLayoutManager(llm);

        final FloatingActionMenu fabMenu =
                (FloatingActionMenu) getActivity().findViewById(R.id.fab_add_item_menu);
        FloatingActionButton fabAddPatchedFile =
                (FloatingActionButton) getActivity().findViewById(R.id.fab_add_patched_file);
        FloatingActionButton fabAddBackup =
                (FloatingActionButton) getActivity().findViewById(R.id.fab_add_backup);

        fabAddPatchedFile.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                addPatchedFile();
                fabMenu.close(true);
            }
        });
        fabAddBackup.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                addBackup();
                fabMenu.close(true);
            }
        });

        if (savedInstanceState != null) {
            mSelectedUri = savedInstanceState.getParcelable(EXTRA_SELECTED_URI);
            mSelectedUriFileName = savedInstanceState.getString(EXTRA_SELECTED_URI_FILE_NAME);
            mSelectedBackupDir = savedInstanceState.getString(EXTRA_SELECTED_BACKUP_DIR);
            mSelectedBackupName = savedInstanceState.getString(EXTRA_SELECTED_BACKUP_NAME);
            mSelectedBackupTargets = savedInstanceState.getStringArray(EXTRA_SELECTED_BACKUP_TARGETS);
            mSelectedRomId = savedInstanceState.getString(EXTRA_SELECTED_ROM_ID);
            mZipRomId = savedInstanceState.getString(EXTRA_ZIP_ROM_ID);
            mAddType = (Type) savedInstanceState.getSerializable(EXTRA_ADD_TYPE);
            mTaskIdVerifyZip = savedInstanceState.getInt(EXTRA_TASK_ID_VERIFY_ZIP);
            mQueryingMetadata = savedInstanceState.getBoolean(EXTRA_QUERYING_METADATA);
        }

        try {
            mActivityCallback = (OnReadyStateChangedListener) getActivity();
        } catch (ClassCastException e) {
            throw new ClassCastException(getActivity().toString()
                    + " must implement OnReadyStateChangedListener");
        }

        mActivityCallback.onReady(!mPendingActions.isEmpty());

        mPrefs = getActivity().getSharedPreferences("settings", 0);

        if (savedInstanceState == null) {
            boolean shouldShow = mPrefs.getBoolean(PREF_SHOW_FIRST_USE_DIALOG, true);
            if (shouldShow) {
                FirstUseDialog d = FirstUseDialog.newInstance(
                        this, R.string.in_app_flashing_title,
                        R.string.in_app_flashing_dialog_first_use);
                d.show(getFragmentManager(), CONFIRM_DIALOG_FIRST_USE);
            }
        }

        getActivity().getLoaderManager().initLoader(0, null, this);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putParcelableArrayList(EXTRA_PENDING_ACTIONS, mPendingActions);

        outState.putParcelable(EXTRA_SELECTED_URI, mSelectedUri);
        outState.putString(EXTRA_SELECTED_URI_FILE_NAME, mSelectedUriFileName);
        outState.putString(EXTRA_SELECTED_BACKUP_DIR, mSelectedBackupDir);
        outState.putString(EXTRA_SELECTED_BACKUP_NAME, mSelectedBackupName);
        outState.putStringArray(EXTRA_SELECTED_BACKUP_TARGETS, mSelectedBackupTargets);
        outState.putString(EXTRA_SELECTED_ROM_ID, mSelectedRomId);
        outState.putString(EXTRA_ZIP_ROM_ID, mZipRomId);
        outState.putSerializable(EXTRA_ADD_TYPE, mAddType);
        outState.putInt(EXTRA_TASK_ID_VERIFY_ZIP, mTaskIdVerifyZip);
        outState.putBoolean(EXTRA_QUERYING_METADATA, mQueryingMetadata);
    }

    @Override
    public void onStart() {
        super.onStart();

        // Start and bind to the service
        Intent intent = new Intent(getActivity(), SwitcherService.class);
        getActivity().bindService(intent, this, Context.BIND_AUTO_CREATE);
        getActivity().startService(intent);
    }

    @Override
    public void onStop() {
        super.onStop();

        // Cancel metadata query task
        cancelQueryUriMetadata();

        // If we connected to the service and registered the callback, now we unregister it
        if (mService != null) {
            if (mTaskIdVerifyZip >= 0) {
                mService.removeCallback(mTaskIdVerifyZip, mCallback);
            }
        }

        // Unbind from our service
        getActivity().unbindService(this);
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

        if (mTaskIdVerifyZip >= 0) {
            mService.addCallback(mTaskIdVerifyZip, mCallback);
        }

        if (mVerifyZipOnServiceConnected) {
            mVerifyZipOnServiceConnected = false;
            verifyZip();
        }

        if (mQueryingMetadata) {
            queryUriMetadata();
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

    @Override
    public Loader<LoaderResult> onCreateLoader(int id, Bundle args) {
        return new BuiltinRomsLoader(getActivity());
    }

    @Override
    public void onLoadFinished(Loader<LoaderResult> loader, LoaderResult result) {
        mBuiltinRoms.clear();

        if (result.builtinRoms != null) {
            Collections.addAll(mBuiltinRoms, result.builtinRoms);
        }

        if (result.currentRom != null) {
            mCurrentRomId = result.currentRom.getId();
        }

        mProgressBar.setVisibility(View.GONE);
    }

    @Override
    public void onLoaderReset(Loader<LoaderResult> loader) {
    }

    private static String[] getDirectories(String path) {
        final ArrayList<String> filenames = new ArrayList<>();
        File[] files = new File(path).listFiles();

        if (files == null) {
            return null;
        }

        for (File file : files) {
            if (file.isDirectory()) {
                filenames.add(file.getName());
            }
        }

        return filenames.toArray(new String[filenames.size()]);
    }

    private void addPatchedFile() {
        mAddType = Type.ROM_INSTALLER;

        // Show file chooser
        Intent intent = FileUtils.getFileOpenIntent(getActivity(), "*/*");
        startActivityForResult(intent, ACTIVITY_REQUEST_FILE);
    }

    private void addBackup() {
        mSelectedBackupDir = mPrefs.getString(
                Constants.Preferences.BACKUP_DIRECTORY, Constants.Defaults.BACKUP_DIRECTORY);
        String[] backupNames = getDirectories(mSelectedBackupDir);

        if (backupNames == null || backupNames.length == 0) {
            Toast.makeText(getActivity(), R.string.in_app_flashing_no_backups_available,
                    Toast.LENGTH_LONG).show();
        } else {
            mAddType = Type.BACKUP_RESTORE;

            GenericSingleChoiceDialog d = GenericSingleChoiceDialog.newInstanceFromFragment(
                    this, 0, null, getString(R.string.in_app_flashing_select_backup_dialog_desc),
                    getString(R.string.ok), getString(R.string.cancel), backupNames);
            d.show(getFragmentManager(), CONFIRM_DIALOG_SELECT_BACKUP);
        }
    }

    private void onVerifiedZip(String romId, VerificationResult result) {
        removeCachedTaskId(mTaskIdVerifyZip);
        mTaskIdVerifyZip = -1;

        GenericProgressDialog dialog = (GenericProgressDialog) getFragmentManager()
                .findFragmentByTag(PROGRESS_DIALOG_VERIFY_ZIP);
        if (dialog != null) {
            dialog.dismiss();
        }

        mZipRomId = romId;

        if (result == VerificationResult.NO_ERROR) {
            queryUriMetadata();
        } else {
            String error;

            switch (result) {
            case ERROR_ZIP_NOT_FOUND:
                error = getString(R.string.in_app_flashing_error_zip_not_found);
                break;

            case ERROR_ZIP_READ_FAIL:
                error = getString(R.string.in_app_flashing_error_zip_read_fail);
                break;

            case ERROR_NOT_MULTIBOOT:
                error = getString(R.string.in_app_flashing_error_not_multiboot);
                break;

            case ERROR_VERSION_TOO_OLD:
                error = String.format(
                        getString(R.string.in_app_flashing_error_version_too_old),
                        MbtoolUtils.getMinimumRequiredVersion(Feature.IN_APP_INSTALLATION));
                break;

            default:
                throw new IllegalStateException("Invalid verification result ID");
            }

            GenericConfirmDialog d = GenericConfirmDialog.newInstanceFromFragment(
                    null, -1, null, error, null);
            d.show(getFragmentManager(), CONFIRM_DIALOG_ERROR);
        }
    }

    /**
     * Called after the input URI's metadata has been retrieved
     *
     * @param metadata URI metadata
     *
     * @see #queryUriMetadata()
     */
    private void onQueriedMetadata(@NonNull UriMetadata metadata) {
        GenericProgressDialog dialog = (GenericProgressDialog)
                getFragmentManager().findFragmentByTag(PROGRESS_DIALOG_QUERYING_METADATA);
        if (dialog != null) {
            dialog.dismiss();
        }

        mSelectedUriFileName = metadata.displayName;

        if (mZipRomId != null) {
            ChangeInstallLocationDialog cild =
                    ChangeInstallLocationDialog.newInstance(this, mZipRomId);
            cild.show(getFragmentManager(), CONFIRM_DIALOG_INSTALL_LOCATION);
        } else {
            showRomIdSelectionDialog();
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
        case PERFORM_ACTIONS:
            mActivityCallback.onFinished();
            break;
        case ACTIVITY_REQUEST_FILE:
            if (data != null && resultCode == Activity.RESULT_OK) {
                mSelectedUri = data.getData();

                GenericProgressDialog d = GenericProgressDialog.newInstance(
                        R.string.in_app_flashing_dialog_verifying_zip, R.string.please_wait);
                d.show(getFragmentManager(), PROGRESS_DIALOG_VERIFY_ZIP);

                if (mService != null) {
                    verifyZip();
                } else {
                    mVerifyZipOnServiceConnected = true;
                }
            }
            break;
        }

        super.onActivityResult(requestCode, resultCode, data);
    }

    private void verifyZip() {
        mTaskIdVerifyZip = mService.verifyZip(mSelectedUri);
        mService.addCallback(mTaskIdVerifyZip, mCallback);
        mService.enqueueTaskId(mTaskIdVerifyZip);
    }

    /**
     * Query the metadata for the input file
     *
     * After the user selects an input file, this function is called to start the task of retrieving
     * the file's name and size. Once the information has been retrieved,
     * {@link #onQueriedMetadata(UriMetadata)} is called.
     *
     * @see #onQueriedMetadata(UriMetadata)
     */
    private void queryUriMetadata() {
        if (mQueryMetadataTask != null) {
            throw new IllegalStateException("Already querying metadata!");
        }
        mQueryMetadataTask = new GetUriMetadataTask();
        mQueryMetadataTask.execute(mSelectedUri);

        // Show progress dialog. Dialog may already exist if a configuration change occurred during
        // the query (and thus, this function is called again in onReady()).
        GenericProgressDialog dialog = (GenericProgressDialog)
                getFragmentManager().findFragmentByTag(PROGRESS_DIALOG_QUERYING_METADATA);
        if (dialog == null) {
            dialog = GenericProgressDialog.newInstance(0, R.string.please_wait);
            dialog.show(getFragmentManager(), PROGRESS_DIALOG_QUERYING_METADATA);
        }
    }

    /**
     * Cancel task for querying the input URI metadata
     *
     * This function is a no-op if there is no such task.
     *
     * @see #onStop()
     */
    private void cancelQueryUriMetadata() {
        if (mQueryMetadataTask != null) {
            mQueryMetadataTask.cancel(true);
            mQueryMetadataTask = null;
        }
    }

    @Override
    public void onConfirmFirstUse(boolean dontShowAgain) {
        Editor e = mPrefs.edit();
        e.putBoolean(PREF_SHOW_FIRST_USE_DIALOG, !dontShowAgain);
        e.apply();
    }

    @Override
    public void onConfirmSingleChoice(int id, int index, String text) {
        mSelectedBackupName = text;

        // Adk for restore targets
        BackupRestoreTargetsSelectionDialog d =
                BackupRestoreTargetsSelectionDialog.newInstanceFromFragment(this, Action.RESTORE);
        d.show(getFragmentManager(), CONFIRM_DIALOG_SELECT_TARGETS);
    }

    @Override
    public void onSelectedBackupRestoreTargets(String[] targets) {
        mSelectedBackupTargets = targets;

        // Ask for ROM ID
        showRomIdSelectionDialog();
    }

    @Override
    public void onSelectedRomId(RomIdType type, String romId) {
        switch (type) {
        case BUILT_IN_ROM_ID:
            mSelectedRomId = romId;
            onHaveRomId();
            break;
        case NAMED_DATA_SLOT: {
            NamedSlotIdInputDialog d = NamedSlotIdInputDialog.newInstance(
                    this, NamedSlotIdInputDialog.DATA_SLOT);
            d.show(getFragmentManager(), CONFIRM_DIALOG_NAMED_SLOT_ID);
            break;
        }
        case NAMED_EXTSD_SLOT: {
            NamedSlotIdInputDialog d = NamedSlotIdInputDialog.newInstance(
                    this, NamedSlotIdInputDialog.EXTSD_SLOT);
            d.show(getFragmentManager(), CONFIRM_DIALOG_NAMED_SLOT_ID);
            break;
        }
        }
    }

    @Override
    public void onSelectedNamedSlotRomId(String romId) {
        mSelectedRomId = romId;
        onHaveRomId();
    }

    @Override
    public void onChangeInstallLocationClicked(boolean changeInstallLocation) {
        if (changeInstallLocation) {
            showRomIdSelectionDialog();
        } else {
            mSelectedRomId = mZipRomId;
            onHaveRomId();
        }
    }

    @Override
    public void onItemMoved(int fromPosition, int toPosition) {
        Collections.swap(mPendingActions, fromPosition, toPosition);
        mAdapter.notifyItemMoved(fromPosition, toPosition);
    }

    @Override
    public void onItemDismissed(int position) {
        mPendingActions.remove(position);
        mAdapter.notifyItemRemoved(position);

        if (mPendingActions.isEmpty()) {
            mActivityCallback.onReady(false);
        }
    }

    private void onHaveRomId() {
        if (mSelectedRomId.equals(mCurrentRomId)) {
            GenericConfirmDialog d = GenericConfirmDialog.newInstanceFromFragment(
                    null, -1, null, getString(R.string.in_app_flashing_error_no_overwrite_rom), null);
            d.show(getFragmentManager(), CONFIRM_DIALOG_ERROR);
        } else {
            mActivityCallback.onReady(true);

            MbtoolAction action = null;

            if (mAddType == Type.ROM_INSTALLER) {
                RomInstallerParams params = new RomInstallerParams(
                        mSelectedUri, mSelectedUriFileName, mSelectedRomId);
                action = new MbtoolAction(params);
            } else if (mAddType == Type.BACKUP_RESTORE) {
                BackupRestoreParams params = new BackupRestoreParams(Action.RESTORE, mSelectedRomId,
                        mSelectedBackupTargets, mSelectedBackupName, mSelectedBackupDir, false);
                action = new MbtoolAction(params);
            }

            if (action != null) {
                mPendingActions.add(action);
                mAdapter.notifyItemInserted(mPendingActions.size() - 1);
            }
        }
    }

    private void showRomIdSelectionDialog() {
        RomIdSelectionDialog dialog = RomIdSelectionDialog.newInstance(this, mBuiltinRoms);
        dialog.show(getFragmentManager(), CONFIRM_DIALOG_ROM_ID);
    }

    public void onActionBarCheckItemClicked() {
        Intent intent = new Intent(getActivity(), MbtoolTaskOutputActivity.class);
        intent.putExtra(MbtoolTaskOutputFragment.PARAM_ACTIONS, getPendingActions());
        startActivityForResult(intent, PERFORM_ACTIONS);
    }

    @NonNull
    private MbtoolAction[] getPendingActions() {
        return mPendingActions.toArray(new MbtoolAction[mPendingActions.size()]);
    }

    private static class BuiltinRomsLoader extends AsyncTaskLoader<LoaderResult> {
        private LoaderResult mResult;

        public BuiltinRomsLoader(Context context) {
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
            RomInformation currentRom = null;

            MbtoolConnection conn = null;

            try {
                conn = new MbtoolConnection(getContext());
                MbtoolInterface iface = conn.getInterface();

                currentRom = RomUtils.getCurrentRom(getContext(), iface);
            } catch (Exception e) {
                // Ignore
            } finally {
                IOUtils.closeQuietly(conn);
            }

            mResult = new LoaderResult();
            mResult.builtinRoms = RomUtils.getBuiltinRoms(getContext());
            mResult.currentRom = currentRom;
            return mResult;
        }
    }

    protected static class LoaderResult {
        RomInformation[] builtinRoms;
        RomInformation currentRom;
    }

    private static class PendingActionViewHolder extends ViewHolder {
        CardView vCard;
        TextView vTitle;
        TextView vSubtitle1;
        TextView vSubtitle2;
        TextView vSubtitle3;

        public PendingActionViewHolder(View itemView) {
            super(itemView);
            vCard = (CardView) itemView;
            vTitle = (TextView) itemView.findViewById(R.id.action_title);
            vSubtitle1 = (TextView) itemView.findViewById(R.id.action_subtitle1);
            vSubtitle2 = (TextView) itemView.findViewById(R.id.action_subtitle2);
            vSubtitle3 = (TextView) itemView.findViewById(R.id.action_subtitle3);
        }
    }

    private static class PendingActionCardAdapter extends
            RecyclerView.Adapter<PendingActionViewHolder> {
        private Context mContext;
        private List<MbtoolAction> mItems;

        public PendingActionCardAdapter(Context context, List<MbtoolAction> items) {
            mContext = context;
            mItems = items;
        }

        @Override
        public PendingActionViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View view = LayoutInflater
                    .from(parent.getContext())
                    .inflate(R.layout.card_v7_pending_action, parent, false);
            return new PendingActionViewHolder(view);
        }

        @Override
        public void onBindViewHolder(PendingActionViewHolder holder, int position) {
            MbtoolAction pa = mItems.get(position);

            switch (pa.getType()) {
            case ROM_INSTALLER: {
                RomInstallerParams params = pa.getRomInstallerParams();

                holder.vTitle.setText(R.string.in_app_flashing_action_flash_file);
                holder.vSubtitle1.setText(mContext.getString(
                        R.string.in_app_flashing_filename, params.getDisplayName()));
                holder.vSubtitle2.setText(mContext.getString(
                        R.string.in_app_flashing_location, params.getRomId()));
                holder.vSubtitle3.setVisibility(View.GONE);
                break;
            }
            case BACKUP_RESTORE: {
                BackupRestoreParams params = pa.getBackupRestoreParams();

                holder.vTitle.setText(R.string.in_app_flashing_action_restore_backup);
                holder.vSubtitle1.setText(mContext.getString(
                        R.string.in_app_flashing_backup_name, params.getBackupName()));
                holder.vSubtitle2.setText(mContext.getString(
                        R.string.in_app_flashing_restore_targets,
                        Arrays.toString(params.getTargets())));
                holder.vSubtitle3.setText(mContext.getString(
                        R.string.in_app_flashing_location, params.getRomId()));
                holder.vSubtitle3.setVisibility(View.VISIBLE);
                break;
            }
            }
        }

        @Override
        public int getItemCount() {
            return mItems.size();
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

    private class SwitcherEventCallback implements VerifyZipTaskListener {
        @Override
        public void onVerifiedZip(int taskId, Uri uri, final VerificationResult result,
                                  final String romId) {
            if (taskId == mTaskIdVerifyZip) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        InAppFlashingFragment.this.onVerifiedZip(romId, result);
                    }
                });
            }
        }
    }
}
