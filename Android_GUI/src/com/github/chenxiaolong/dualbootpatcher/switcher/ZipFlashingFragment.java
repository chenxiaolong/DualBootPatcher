/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.app.AlertDialog;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.LoaderManager;
import android.content.AsyncTaskLoader;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.Loader;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.CheckBox;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ButtonCallback;
import com.afollestad.materialdialogs.MaterialDialog.ListCallback;
import com.getbase.floatingactionbutton.AddFloatingActionButton;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.VerifiedZipEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.LoaderResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction.Type;
import com.github.chenxiaolong.multibootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.multibootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.multibootpatcher.FileChooserEventCollector;
import com.github.chenxiaolong.multibootpatcher.FileChooserEventCollector.RequestedFileEvent;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolUtils.Feature;
import com.nhaarman.listviewanimations.ArrayAdapter;
import com.nhaarman.listviewanimations.itemmanipulation.DynamicListView;
import com.nhaarman.listviewanimations.itemmanipulation.swipedismiss.OnDismissCallback;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;

public class ZipFlashingFragment extends Fragment implements EventCollectorListener,
        OnDismissListener, LoaderManager.LoaderCallbacks<LoaderResult> {
    private static final String TAG = ZipFlashingFragment.class.getSimpleName();

    private static final int PERFORM_ACTIONS = 1234;

    private static final String EXTRA_PENDING_ACTIONS = "pending_actions";
    private static final String EXTRA_SELECTED_FILE = "selected_file";
    private static final String EXTRA_SELECTED_ROM_ID = "selected_rom_id";
    private static final String EXTRA_VERIFICATION_ERROR = "verification_error";
    private static final String EXTRA_SELECTION_DIALOG = "selection_dialog";
    private static final String EXTRA_PROGRESS_DIALOG = "progress_dialog";
    private static final String EXTRA_CONFIRM_DIALOG = "confirm_dialog";
    private static final String EXTRA_FIRST_TIME_DIALOG = "first_time_dialog";

    private static final int SELECTION_DIALOG_ROM_ID = 1;
    private static final int PROGRESS_DIALOG_VERIFYING_ZIP = 1;
    private static final int CONFIRM_DIALOG_VERIFY_FAIL = 1;
    private static final int CONFIRM_DIALOG_NO_OVERWRITE_ROM = 2;
    private static final int FIRST_TIME_DIALOG = 1;

    private static final String PREF_SHOW_FIRST_USE_DIALOG = "zip_flashing_first_use_show_dialog";

    private int mSelectionDialogType;
    private AlertDialog mSelectionDialog;
    private int mProgressDialogType;
    private AlertDialog mProgressDialog;
    private int mConfirmDialogType;
    private AlertDialog mConfirmDialog;
    private int mFirstTimeDialogType;
    private AlertDialog mFirstTimeDialog;

    private OnReadyStateChangedListener mCallback;

    private SharedPreferences mPrefs;

    private Bundle mSavedInstanceState;

    private SwitcherEventCollector mSwitcherEC;
    private FileChooserEventCollector mFileChooserEC;
    private String mSelectedFile;
    private String mSelectedRomId;
    private String mCurrentRomId;

    private String mVerificationError;

    private DynamicListView mCardListView;
    private ProgressBar mProgressBar;
    private PendingActionCardAdapter mAdapter;
    private AddFloatingActionButton mFabFlashZip;

    private ArrayList<RomInformation> mBuiltinRoms = new ArrayList<>();
    private String[] mBuiltinRomNames;

    public interface OnReadyStateChangedListener {
        void onReady(boolean ready);

        void onFinished();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        FragmentManager fm = getFragmentManager();
        mSwitcherEC = (SwitcherEventCollector) fm.findFragmentByTag(SwitcherEventCollector.TAG);
        mFileChooserEC = (FileChooserEventCollector) fm.findFragmentByTag
                (FileChooserEventCollector.TAG);

        if (mSwitcherEC == null) {
            mSwitcherEC = new SwitcherEventCollector();
            fm.beginTransaction().add(mSwitcherEC, SwitcherEventCollector.TAG).commit();
        }

        if (mFileChooserEC == null) {
            mFileChooserEC = new FileChooserEventCollector();
            fm.beginTransaction().add(mFileChooserEC, FileChooserEventCollector.TAG).commit();
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_zip_flashing, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mProgressBar = (ProgressBar) getActivity().findViewById(R.id.card_list_loading);

        mAdapter = new PendingActionCardAdapter(getActivity());

        if (savedInstanceState != null) {
            ArrayList<PendingAction> savedActions =
                    savedInstanceState.getParcelableArrayList(EXTRA_PENDING_ACTIONS);
            mAdapter.addAll(savedActions);
        }

        mCardListView = (DynamicListView) getActivity().findViewById(R.id.card_list);
        mCardListView.setAdapter(mAdapter);
        mCardListView.enableDragAndDrop();
        mCardListView.setOnItemLongClickListener(new MyOnItemLongClickListener(mCardListView));
        mCardListView.enableSwipeToDismiss(new MyOnDismissCallback(mAdapter));

        mFabFlashZip = (AddFloatingActionButton) getActivity().findViewById(R.id.fab_add_zip);
        mFabFlashZip.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                mFileChooserEC.startFileChooser();
            }
        });

        mSavedInstanceState = savedInstanceState;

        if (savedInstanceState != null) {
            mSelectedFile = savedInstanceState.getString(EXTRA_SELECTED_FILE);
            mSelectedRomId = savedInstanceState.getString(EXTRA_SELECTED_ROM_ID);
            mVerificationError = savedInstanceState.getString(EXTRA_VERIFICATION_ERROR);
        }

        try {
            mCallback = (OnReadyStateChangedListener) getActivity();
        } catch (ClassCastException e) {
            throw new ClassCastException(getActivity().toString()
                    + " must implement OnReadyStateChangedListener");
        }

        mCallback.onReady(!mAdapter.isEmpty());

        mPrefs = getActivity().getSharedPreferences("settings", 0);

        if (savedInstanceState == null) {
            boolean shouldShow = mPrefs.getBoolean(PREF_SHOW_FIRST_USE_DIALOG, true);
            if (shouldShow) {
                buildFirstTimeDialog(FIRST_TIME_DIALOG);
            }
        }

        getActivity().getLoaderManager().initLoader(0, null, this);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        ArrayList<PendingAction> pendingActionsArray = new ArrayList<>(mAdapter.getItems());
        outState.putParcelableArrayList(EXTRA_PENDING_ACTIONS, pendingActionsArray);

        outState.putString(EXTRA_SELECTED_FILE, mSelectedFile);
        outState.putString(EXTRA_SELECTED_ROM_ID, mSelectedRomId);
        outState.putString(EXTRA_VERIFICATION_ERROR, mVerificationError);

        if (mSelectionDialog != null) {
            outState.putInt(EXTRA_SELECTION_DIALOG, mSelectionDialogType);
        }
        if (mProgressDialog != null) {
            outState.putInt(EXTRA_PROGRESS_DIALOG, mProgressDialogType);
        }
        if (mConfirmDialog != null) {
            outState.putInt(EXTRA_CONFIRM_DIALOG, mConfirmDialogType);
        }
        if (mFirstTimeDialog != null) {
            outState.putInt(EXTRA_FIRST_TIME_DIALOG, mFirstTimeDialogType);
        }
    }

    @Override
    public void onStop() {
        super.onStop();

        if (mSelectionDialog != null) {
            mSelectionDialog.dismiss();
            mSelectionDialog = null;
        }
        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }
        if (mConfirmDialog != null) {
            mConfirmDialog.dismiss();
            mConfirmDialog = null;
        }
        if (mFirstTimeDialog != null) {
            mFirstTimeDialog.dismiss();
            mFirstTimeDialog = null;
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        mSwitcherEC.attachListener(TAG, this);
        mFileChooserEC.attachListener(TAG, this);
    }

    @Override
    public void onPause() {
        super.onPause();
        mSwitcherEC.detachListener(TAG);
        mFileChooserEC.detachListener(TAG);
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        if (mSelectionDialog == dialog) {
            mSelectionDialog = null;
        } else if (mProgressDialog == dialog) {
            mProgressDialog = null;
        } else if (mConfirmDialog == dialog) {
            mConfirmDialog = null;
        } else if (mFirstTimeDialog == dialog) {
            mFirstTimeDialog = null;
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

            mBuiltinRomNames = new String[mBuiltinRoms.size()];
            for (int i = 0; i < mBuiltinRoms.size(); i++) {
                mBuiltinRomNames[i] = mBuiltinRoms.get(i).getDefaultName();
            }
        }

        if (result.currentRom != null) {
            mCurrentRomId = result.currentRom.getId();
        }

        mProgressBar.setVisibility(View.GONE);

        if (mSavedInstanceState != null) {
            int selectionDialogType = mSavedInstanceState.getInt(EXTRA_SELECTION_DIALOG);
            if (selectionDialogType > 0) {
                buildSelectionDialog(selectionDialogType);
            }

            int progressDialogType = mSavedInstanceState.getInt(EXTRA_PROGRESS_DIALOG);
            if (progressDialogType > 0) {
                buildProgressDialog(progressDialogType);
            }

            int confirmDialogType = mSavedInstanceState.getInt(EXTRA_CONFIRM_DIALOG);
            if (confirmDialogType > 0) {
                buildConfirmDialog(confirmDialogType);
            }

            int firstTimeDialogType = mSavedInstanceState.getInt(EXTRA_FIRST_TIME_DIALOG);
            if (firstTimeDialogType > 0) {
                buildFirstTimeDialog(firstTimeDialogType);
            }
        }
    }

    @Override
    public void onLoaderReset(Loader<LoaderResult> loader) {
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof RequestedFileEvent) {
            RequestedFileEvent e = (RequestedFileEvent) event;
            mSelectedFile = e.file;

            buildSelectionDialog(SELECTION_DIALOG_ROM_ID);
        } else if (event instanceof VerifiedZipEvent) {
            mProgressDialog.dismiss();
            mProgressDialog = null;

            VerifiedZipEvent e = (VerifiedZipEvent) event;

            if (e.result == VerificationResult.NO_ERROR) {
                mCallback.onReady(true);

                PendingAction pa = new PendingAction();
                pa.type = Type.INSTALL_ZIP;
                pa.zipFile = mSelectedFile;
                pa.romId = mSelectedRomId;
                mAdapter.add(pa);
                mAdapter.notifyDataSetChanged();
            } else {
                switch (e.result) {
                case ERROR_ZIP_NOT_FOUND:
                    mVerificationError = getString(R.string.zip_flashing_error_zip_not_found);
                    break;

                case ERROR_ZIP_READ_FAIL:
                    mVerificationError = getString(R.string.zip_flashing_error_zip_read_fail);
                    break;

                case ERROR_NOT_MULTIBOOT:
                    mVerificationError = getString(R.string.zip_flashing_error_not_multiboot);
                    break;

                case ERROR_VERSION_TOO_OLD:
                    mVerificationError = String.format(
                            getString(R.string.zip_flashing_error_version_too_old),
                            MbtoolUtils.getMinimumRequiredVersion(Feature.IN_APP_INSTALLATION));
                    break;
                }

                buildConfirmDialog(CONFIRM_DIALOG_VERIFY_FAIL);
            }
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
        case PERFORM_ACTIONS:
            mCallback.onFinished();
            break;
        }

        super.onActivityResult(requestCode, resultCode, data);
    }

    public void onActionBarCheckItemClicked() {
        Intent intent = new Intent(getActivity(), ZipFlashingOutputActivity.class);
        intent.putExtra(ZipFlashingOutputFragment.PARAM_PENDING_ACTIONS, getPendingActions());
        startActivityForResult(intent, PERFORM_ACTIONS);
    }

    private void buildSelectionDialog(int type) {
        if (mSelectionDialog != null) {
            throw new IllegalStateException("Tried to create selection dialog twice!");
        }

        switch (type) {
        case SELECTION_DIALOG_ROM_ID:
            mSelectionDialog = new MaterialDialog.Builder(getActivity())
                    .title(R.string.zip_flashing_dialog_installation_location)
                    .items(mBuiltinRomNames)
                    .negativeText(R.string.cancel)
                    .itemsCallbackSingleChoice(-1, new ListCallback() {
                        @Override
                        public void onSelection(MaterialDialog dialog, View view, int which,
                                                CharSequence text) {
                            mSelectedRomId = mBuiltinRoms.get(which).getId();

                            if (mSelectedRomId.equals(mCurrentRomId)) {
                                buildConfirmDialog(CONFIRM_DIALOG_NO_OVERWRITE_ROM);
                            } else {
                                buildProgressDialog(PROGRESS_DIALOG_VERIFYING_ZIP);
                                mSwitcherEC.verifyZip(mSelectedFile);
                            }
                        }
                    })
                    .build();
            break;
        default:
            throw new IllegalStateException("Invalid selection dialog type");
        }

        mSelectionDialogType = type;

        mSelectionDialog.setOnDismissListener(this);
        mSelectionDialog.setCanceledOnTouchOutside(false);
        mSelectionDialog.setCancelable(false);
        mSelectionDialog.show();
    }

    private void buildProgressDialog(int type) {
        if (mProgressDialog != null) {
            throw new IllegalStateException("Tried to create progress dialog twice!");
        }

        int titleResId;
        int messageResId;

        switch (type) {
        case PROGRESS_DIALOG_VERIFYING_ZIP:
            titleResId = R.string.zip_flashing_dialog_verifying_zip;
            messageResId = R.string.please_wait;
            break;
        default:
            throw new IllegalStateException("Invalid progress dialog type");
        }

        mProgressDialogType = type;

        mProgressDialog = new MaterialDialog.Builder(getActivity())
                .title(titleResId)
                .content(messageResId)
                .progress(true, 0)
                .build();

        mProgressDialog.setOnDismissListener(this);
        mProgressDialog.setCanceledOnTouchOutside(false);
        mProgressDialog.setCancelable(false);
        mProgressDialog.show();
    }

    private void buildConfirmDialog(int type) {
        if (mConfirmDialog != null) {
            throw new IllegalStateException("Tried to create confirm dialog twice!");
        }

        switch (type) {
        case CONFIRM_DIALOG_VERIFY_FAIL:
            mConfirmDialog = new MaterialDialog.Builder(getActivity())
                    .content(mVerificationError)
                    .positiveText(R.string.ok)
                    .build();
            break;
        case CONFIRM_DIALOG_NO_OVERWRITE_ROM:
            mConfirmDialog = new MaterialDialog.Builder(getActivity())
                    .content(R.string.zip_flashing_error_no_overwrite_rom)
                    .positiveText(R.string.ok)
                    .build();
            break;
        default:
            throw new IllegalStateException("Invalid confirm dialog type");
        }

        mConfirmDialogType = type;

        mConfirmDialog.setOnDismissListener(this);
        mConfirmDialog.setCanceledOnTouchOutside(false);
        mConfirmDialog.setCancelable(false);
        mConfirmDialog.show();
    }

    private void buildFirstTimeDialog(int type) {
        if (mFirstTimeDialog != null) {
            throw new IllegalStateException("Tried to create first time dialog twice!");
        }

        switch (type) {
        case FIRST_TIME_DIALOG:
            String message = String.format(getString(R.string.zip_flashing_dialog_first_use));

            mFirstTimeDialog = new MaterialDialog.Builder(getActivity())
                    .title(R.string.zip_flashing_title)
                    .customView(R.layout.dialog_first_time, true)
                    .positiveText(R.string.ok)
                    .callback(new ButtonCallback() {
                        @Override
                        public void onPositive(MaterialDialog dialog) {
                            CheckBox cb = (CheckBox) dialog.findViewById(R.id.checkbox);

                            Editor e = mPrefs.edit();
                            e.putBoolean(PREF_SHOW_FIRST_USE_DIALOG, !cb.isChecked());
                            e.apply();
                        }
                    })
                    .build();

            TextView tv = (TextView)
                    ((MaterialDialog) mFirstTimeDialog).getCustomView().findViewById(R.id.message);
            tv.setText(message);
            break;
        default:
            throw new IllegalStateException("Invalid first time dialog type");
        }

        mFirstTimeDialogType = type;

        mFirstTimeDialog.setOnDismissListener(this);
        mFirstTimeDialog.setCanceledOnTouchOutside(false);
        mFirstTimeDialog.setCancelable(false);
        mFirstTimeDialog.show();
    }

    private PendingAction[] getPendingActions() {
        PendingAction[] pa = new PendingAction[mAdapter.getCount()];
        mAdapter.getItems().toArray(pa);
        return pa;
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
            mResult = new LoaderResult();
            mResult.builtinRoms = RomUtils.getBuiltinRoms(getContext());
            mResult.currentRom = RomUtils.getCurrentRom(getContext());
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

        Type type;
        String zipFile;
        String romId;

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
                return null;
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

    private static class PendingActionCardAdapter extends ArrayAdapter<PendingAction> {
        private final Context mContext;

        public PendingActionCardAdapter(final Context context) {
            mContext = context;
        }

        @Override
        public long getItemId(final int position) {
            return getItem(position).hashCode();
        }

        @Override
        public boolean hasStableIds() {
            return true;
        }

        @Override
        public View getView(final int position, final View convertView, final ViewGroup parent) {
            View view = convertView;
            if (view == null) {
                view = LayoutInflater.from(mContext).inflate(
                        R.layout.card_v7_pending_action, parent, false);
            }

            TextView vTitle = (TextView) view.findViewById(R.id.action_title);
            TextView vSubtitle1 = (TextView) view.findViewById(R.id.action_subtitle1);
            TextView vSubtitle2 = (TextView) view.findViewById(R.id.action_subtitle2);

            String zipFile = mContext.getString(R.string.zip_flashing_zip_file);
            String location = mContext.getString(R.string.zip_flashing_location);

            PendingAction pa = getItem(position);

            switch (pa.type) {
            case INSTALL_ZIP:
                vTitle.setText(R.string.zip_flashing_install_zip);
            }

            vSubtitle1.setText(String.format(zipFile, new File(pa.zipFile).getName()));
            vSubtitle2.setText(String.format(location, pa.romId));

            return view;
        }
    }

    private static class MyOnItemLongClickListener implements AdapterView.OnItemLongClickListener {
        private final DynamicListView mListView;

        MyOnItemLongClickListener(final DynamicListView listView) {
            mListView = listView;
        }

        @Override
        public boolean onItemLongClick(final AdapterView<?> parent, final View view,
                                       final int position, final long id) {
            if (mListView != null) {
                try {
                    mListView.startDragging(position - mListView.getHeaderViewsCount());
                } catch (IllegalStateException e) {
                    // Not sure why the following happens sometimes
                    // java.lang.IllegalStateException: User must be touching the DynamicListView!
                    // See https://github.com/nhaarman/ListViewAnimations/issues/325
                }
            }
            return true;
        }
    }

    private class MyOnDismissCallback implements OnDismissCallback {
        private final ArrayAdapter<PendingAction> mAdapter;

        MyOnDismissCallback(final ArrayAdapter<PendingAction> adapter) {
            mAdapter = adapter;
        }

        @Override
        public void onDismiss(@NonNull final ViewGroup listView,
                              @NonNull final int[] reverseSortedPositions) {
            for (int position : reverseSortedPositions) {
                mAdapter.remove(position);
            }

            if (mAdapter.isEmpty()) {
                mCallback.onReady(false);
            }
        }
    }
}
