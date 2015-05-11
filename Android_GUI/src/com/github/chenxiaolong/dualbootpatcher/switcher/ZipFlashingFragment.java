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

import android.app.Fragment;
import android.app.FragmentManager;
import android.app.LoaderManager;
import android.content.AsyncTaskLoader;
import android.content.Context;
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
import android.widget.ProgressBar;
import android.widget.TextView;

import com.getbase.floatingactionbutton.AddFloatingActionButton;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.DataSlotIdInputDialog
        .DataSlotIdInputDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.FirstUseDialog.FirstUseDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomIdSelectionDialog
        .RomIdSelectionDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.RomIdSelectionDialog.RomIdType;
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
        FirstUseDialogListener, RomIdSelectionDialogListener, DataSlotIdInputDialogListener,
        LoaderManager.LoaderCallbacks<LoaderResult> {
    private static final String TAG = ZipFlashingFragment.class.getSimpleName();

    private static final int PERFORM_ACTIONS = 1234;

    private static final String EXTRA_PENDING_ACTIONS = "pending_actions";
    private static final String EXTRA_SELECTED_FILE = "selected_file";
    private static final String EXTRA_SELECTED_ROM_ID = "selected_rom_id";

    private static final String PREF_SHOW_FIRST_USE_DIALOG = "zip_flashing_first_use_show_dialog";

    private OnReadyStateChangedListener mCallback;

    private SharedPreferences mPrefs;

    private SwitcherEventCollector mSwitcherEC;
    private FileChooserEventCollector mFileChooserEC;
    private String mSelectedFile;
    private String mSelectedRomId;
    private String mCurrentRomId;

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

        if (savedInstanceState != null) {
            mSelectedFile = savedInstanceState.getString(EXTRA_SELECTED_FILE);
            mSelectedRomId = savedInstanceState.getString(EXTRA_SELECTED_ROM_ID);
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
                FirstUseDialog d = FirstUseDialog.newInstance(
                        this, R.string.zip_flashing_title, R.string.zip_flashing_dialog_first_use);
                d.show(getFragmentManager(), FirstUseDialog.TAG);
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
    }

    @Override
    public void onLoaderReset(Loader<LoaderResult> loader) {
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof RequestedFileEvent) {
            RequestedFileEvent e = (RequestedFileEvent) event;
            mSelectedFile = e.file;

            RomIdSelectionDialog dialog = RomIdSelectionDialog.newInstance(this, mBuiltinRoms);
            dialog.show(getFragmentManager(), RomIdSelectionDialog.TAG);
        } else if (event instanceof VerifiedZipEvent) {
            GenericProgressDialog dialog = (GenericProgressDialog) getFragmentManager()
                    .findFragmentByTag(GenericProgressDialog.TAG);
            if (dialog != null) {
                dialog.dismiss();
            }

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
                String error;

                switch (e.result) {
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

                GenericConfirmDialog d = GenericConfirmDialog.newInstance(null, error);
                d.show(getFragmentManager(), GenericConfirmDialog.TAG);
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
        case NAMED_DATA_SLOT:
            DataSlotIdInputDialog d = DataSlotIdInputDialog.newInstance(this);
            d.show(getFragmentManager(), DataSlotIdInputDialog.TAG);
            break;
        }
    }

    @Override
    public void onSelectedDataSlotRomId(String romId) {
        mSelectedRomId = romId;
        onHaveRomId();
    }

    private void onHaveRomId() {
        if (mSelectedRomId.equals(mCurrentRomId)) {
            GenericConfirmDialog d = GenericConfirmDialog.newInstance(
                    0, R.string.zip_flashing_error_no_overwrite_rom);
            d.show(getFragmentManager(), GenericConfirmDialog.TAG);
        } else {
            GenericProgressDialog d = GenericProgressDialog.newInstance(
                    R.string.zip_flashing_dialog_verifying_zip, R.string.please_wait);
            d.show(getFragmentManager(), GenericProgressDialog.TAG);

            mSwitcherEC.verifyZip(mSelectedFile);
        }
    }

    public void onActionBarCheckItemClicked() {
        Intent intent = new Intent(getActivity(), ZipFlashingOutputActivity.class);
        intent.putExtra(ZipFlashingOutputFragment.PARAM_PENDING_ACTIONS, getPendingActions());
        startActivityForResult(intent, PERFORM_ACTIONS);
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
