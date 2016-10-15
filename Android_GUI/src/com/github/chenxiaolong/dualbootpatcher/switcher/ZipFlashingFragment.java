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
import android.content.Context;
import android.content.Intent;
import android.content.Loader;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Parcel;
import android.os.Parcelable;
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

import com.getbase.floatingactionbutton.AddFloatingActionButton;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog.FirstUseDialogListener;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.switcher.ChangeInstallLocationDialog
        .ChangeInstallLocationDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.NamedSlotIdInputDialog
        .NamedSlotIdInputDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomIdSelectionDialog
        .RomIdSelectionDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomIdSelectionDialog.RomIdType;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.LoaderResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction.Type;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.VerifyZipTask.VerifyZipTaskListener;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback;
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback
        .OnItemMovedOrDismissedListener;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class ZipFlashingFragment extends Fragment implements FirstUseDialogListener,
        RomIdSelectionDialogListener, NamedSlotIdInputDialogListener,
        ChangeInstallLocationDialogListener, LoaderCallbacks<LoaderResult>,
        ServiceConnection, OnItemMovedOrDismissedListener {
    private static final int PERFORM_ACTIONS = 1234;

    private static final String EXTRA_PENDING_ACTIONS = "pending_actions";
    private static final String EXTRA_SELECTED_FILE = "selected_file";
    private static final String EXTRA_SELECTED_ROM_ID = "selected_rom_id";
    private static final String EXTRA_ZIP_ROM_ID = "zip_rom_id";
    private static final String EXTRA_TASK_ID_VERIFY_ZIP = "task_id_verify_zip";

    private static final String PREF_SHOW_FIRST_USE_DIALOG = "zip_flashing_first_use_show_dialog";

    /** Request code for file picker (used in {@link #onActivityResult(int, int, Intent)}) */
    private static final int ACTIVITY_REQUEST_FILE = 1000;

    private static final String PROGRESS_DIALOG_VERIFY_ZIP =
            ZipFlashingFragment.class.getCanonicalName() + ".progress.verify_zip";
    private static final String CONFIRM_DIALOG_FIRST_USE =
            ZipFlashingFragment.class.getCanonicalName() + ".confirm.first_use";
    private static final String CONFIRM_DIALOG_INSTALL_LOCATION =
            ZipFlashingFragment.class.getCanonicalName() + ".confirm.install_location";
    private static final String CONFIRM_DIALOG_NAMED_SLOT_ID =
            ZipFlashingFragment.class.getCanonicalName() + ".confirm.named_slot_id";
    private static final String CONFIRM_DIALOG_ROM_ID =
            ZipFlashingFragment.class.getCanonicalName() + ".confirm.rom_id";
    private static final String CONFIRM_DIALOG_ERROR =
            ZipFlashingFragment.class.getCanonicalName() + ".confirm.error";

    private OnReadyStateChangedListener mActivityCallback;

    private SharedPreferences mPrefs;

    private String mSelectedFile;
    private String mSelectedRomId;
    private String mCurrentRomId;
    private String mZipRomId;

    private ProgressBar mProgressBar;

    private ArrayList<PendingAction> mPendingActions = new ArrayList<>();
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

    public interface OnReadyStateChangedListener {
        void onReady(boolean ready);

        void onFinished();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_zip_flashing, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (savedInstanceState != null) {
            ArrayList<PendingAction> savedActions =
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

        AddFloatingActionButton fabFlashZip = (AddFloatingActionButton) getActivity()
                .findViewById(R.id.fab_add_zip);
        fabFlashZip.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                // Show file chooser
                Intent intent = FileUtils.getFileOpenIntent(getActivity(), "*/*");
                startActivityForResult(intent, ACTIVITY_REQUEST_FILE);
            }
        });

        if (savedInstanceState != null) {
            mSelectedFile = savedInstanceState.getString(EXTRA_SELECTED_FILE);
            mSelectedRomId = savedInstanceState.getString(EXTRA_SELECTED_ROM_ID);
            mZipRomId = savedInstanceState.getString(EXTRA_ZIP_ROM_ID);
            mTaskIdVerifyZip = savedInstanceState.getInt(EXTRA_TASK_ID_VERIFY_ZIP);
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
                        this, R.string.zip_flashing_title, R.string.zip_flashing_dialog_first_use);
                d.show(getFragmentManager(), CONFIRM_DIALOG_FIRST_USE);
            }
        }

        getActivity().getLoaderManager().initLoader(0, null, this);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putParcelableArrayList(EXTRA_PENDING_ACTIONS, mPendingActions);

        outState.putString(EXTRA_SELECTED_FILE, mSelectedFile);
        outState.putString(EXTRA_SELECTED_ROM_ID, mSelectedRomId);
        outState.putString(EXTRA_ZIP_ROM_ID, mZipRomId);
        outState.putInt(EXTRA_TASK_ID_VERIFY_ZIP, mTaskIdVerifyZip);
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
            if (romId != null) {
                ChangeInstallLocationDialog cild =
                        ChangeInstallLocationDialog.newInstance(this, romId);
                cild.show(getFragmentManager(), CONFIRM_DIALOG_INSTALL_LOCATION);
            } else {
                showRomIdSelectionDialog();
            }
        } else {
            String error;

            switch (result) {
            case ERROR_ZIP_NOT_FOUND:
                error = getString(R.string.zip_flashing_error_zip_not_found);
                break;

            case ERROR_ZIP_READ_FAIL:
                error = getString(R.string.zip_flashing_error_zip_read_fail);
                break;

            case ERROR_NOT_MULTIBOOT:
                error = getString(R.string.zip_flashing_error_not_multiboot);
                break;

            case ERROR_VERSION_TOO_OLD:
                error = String.format(
                        getString(R.string.zip_flashing_error_version_too_old),
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

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
        case PERFORM_ACTIONS:
            mActivityCallback.onFinished();
            break;
        case ACTIVITY_REQUEST_FILE:
            if (data != null && resultCode == Activity.RESULT_OK) {
                mSelectedFile = FileUtils.getPathFromUri(getActivity(), data.getData());

                GenericProgressDialog d = GenericProgressDialog.newInstance(
                        R.string.zip_flashing_dialog_verifying_zip, R.string.please_wait);
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
        mTaskIdVerifyZip = mService.verifyZip(mSelectedFile);
        mService.addCallback(mTaskIdVerifyZip, mCallback);
        mService.enqueueTaskId(mTaskIdVerifyZip);
    }

    @Override
    public void onConfirmFirstUse(boolean dontShowAgain) {
        Editor e = mPrefs.edit();
        e.putBoolean(PREF_SHOW_FIRST_USE_DIALOG, !dontShowAgain);
        e.apply();
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
                    null, -1, null, getString(R.string.zip_flashing_error_no_overwrite_rom), null);
            d.show(getFragmentManager(), CONFIRM_DIALOG_ERROR);
        } else {
            mActivityCallback.onReady(true);

            PendingAction pa = new PendingAction();
            pa.type = Type.INSTALL_ZIP;
            pa.zipFile = mSelectedFile;
            pa.romId = mSelectedRomId;
            mPendingActions.add(pa);
            mAdapter.notifyItemInserted(mPendingActions.size() - 1);
        }
    }

    private void showRomIdSelectionDialog() {
        RomIdSelectionDialog dialog = RomIdSelectionDialog.newInstance(this, mBuiltinRoms);
        dialog.show(getFragmentManager(), CONFIRM_DIALOG_ROM_ID);
    }

    public void onActionBarCheckItemClicked() {
        Intent intent = new Intent(getActivity(), ZipFlashingOutputActivity.class);
        intent.putExtra(ZipFlashingOutputFragment.PARAM_PENDING_ACTIONS, getPendingActions());
        startActivityForResult(intent, PERFORM_ACTIONS);
    }

    private PendingAction[] getPendingActions() {
        return mPendingActions.toArray(new PendingAction[mPendingActions.size()]);
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

    public static class PendingAction implements Parcelable {
        public enum Type {
            INSTALL_ZIP
        }

        public Type type;
        public String zipFile;
        public String romId;

        public PendingAction() {
        }

        protected PendingAction(Parcel in) {
            type = (Type) in.readSerializable();
            zipFile = in.readString();
            romId = in.readString();
        }

        @Override
        public String toString() {
            switch (type) {
            case INSTALL_ZIP:
                return "Install " + zipFile + " to " + romId;
            default:
                // Not reached
                throw new IllegalStateException("Invalid pending action type");
            }
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeSerializable(type);
            dest.writeString(zipFile);
            dest.writeString(romId);
        }

        @SuppressWarnings("unused")
        public static final Parcelable.Creator<PendingAction> CREATOR =
                new Parcelable.Creator<PendingAction>() {
            @Override
            public PendingAction createFromParcel(Parcel in) {
                return new PendingAction(in);
            }

            @Override
            public PendingAction[] newArray(int size) {
                return new PendingAction[size];
            }
        };
    }

    private static class PendingActionViewHolder extends ViewHolder {
        CardView vCard;
        TextView vTitle;
        TextView vSubtitle1;
        TextView vSubtitle2;

        public PendingActionViewHolder(View itemView) {
            super(itemView);
            vCard = (CardView) itemView;
            vTitle = (TextView) itemView.findViewById(R.id.action_title);
            vSubtitle1 = (TextView) itemView.findViewById(R.id.action_subtitle1);
            vSubtitle2 = (TextView) itemView.findViewById(R.id.action_subtitle2);
        }
    }

    private static class PendingActionCardAdapter extends
            RecyclerView.Adapter<PendingActionViewHolder> {
        private Context mContext;
        private List<PendingAction> mItems;

        public PendingActionCardAdapter(Context context, List<PendingAction> items) {
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
            PendingAction pa = mItems.get(position);

            String zipFile = mContext.getString(R.string.zip_flashing_zip_file);
            String location = mContext.getString(R.string.zip_flashing_location);

            switch (pa.type) {
            case INSTALL_ZIP:
                holder.vTitle.setText(R.string.zip_flashing_install_zip);
            }

            holder.vSubtitle1.setText(String.format(zipFile, new File(pa.zipFile).getName()));
            holder.vSubtitle2.setText(String.format(location, pa.romId));
        }

        @Override
        public int getItemCount() {
            return mItems.size();
        }
    }

    private class SwitcherEventCallback implements VerifyZipTaskListener {
        @Override
        public void onVerifiedZip(int taskId, String path, final VerificationResult result,
                                  final String romId) {
            if (taskId == mTaskIdVerifyZip) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        ZipFlashingFragment.this.onVerifiedZip(romId, result);
                    }
                });
            }
        }
    }
}
