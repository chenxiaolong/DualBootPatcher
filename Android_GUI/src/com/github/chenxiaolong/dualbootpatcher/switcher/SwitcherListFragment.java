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

import android.app.Activity;
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
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.widget.CardView;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.Toast;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ButtonCallback;
import com.afollestad.materialdialogs.MaterialDialog.ListCallbackMulti;
import com.getbase.floatingactionbutton.FloatingActionButton;
import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.SetKernelEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.SwitchedRomEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.WipedRomEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherListFragment.LoaderResult;
import com.github.chenxiaolong.multibootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.multibootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.multibootpatcher.adapters.RomCardAdapter;
import com.github.chenxiaolong.multibootpatcher.adapters.RomCardAdapter.RomCardActionListener;

import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;

import mbtool.daemon.v2.WipeTarget;

public class SwitcherListFragment extends Fragment implements OnDismissListener,
        EventCollectorListener, RomCardActionListener,
        LoaderManager.LoaderCallbacks<LoaderResult> {
    public static final String TAG = SwitcherListFragment.class.getSimpleName();

    private static final String EXTRA_PERFORMING_ACTION = "performingAction";
    private static final String EXTRA_SELECTED_ROM = "selectedRom";
    private static final String EXTRA_PROGRESS_DIALOG = "progressDialog";
    private static final String EXTRA_CONFIRM_DIALOG = "confirmDialog";
    private static final String EXTRA_INPUT_DIALOG = "inputDialog";
    private static final String EXTRA_SELECTION_DIALOG = "selectionDialog";

    private static final int PROGRESS_DIALOG_SWITCH_ROM = 1;
    private static final int PROGRESS_DIALOG_SET_KERNEL = 2;
    private static final int PROGRESS_DIALOG_WIPE_ROM = 3;
    private static final int CONFIRM_DIALOG_SET_KERNEL = 1;
    private static final int CONFIRM_DIALOG_WIPING_EXPERIMENTAL = 2;
    private static final int INPUT_DIALOG_EDIT_NAME = 1;
    private static final int SELECTION_DIALOG_WIPE_TARGETS = 1;

    private static final int REQUEST_IMAGE = 1234;
    private static final int REQUEST_FLASH_ZIP = 2345;

    private boolean mPerformingAction;

    private SwitcherEventCollector mEventCollector;

    private CardView mErrorCardView;
    private RomCardAdapter mRomCardAdapter;
    private RecyclerView mCardListView;
    private ProgressBar mProgressBar;
    private FloatingActionButton mFabFlashZip;

    private int mProgressDialogType;
    private AlertDialog mProgressDialog;
    private int mConfirmDialogType;
    private AlertDialog mConfirmDialog;
    private int mInputDialogType;
    private AlertDialog mInputDialog;
    private int mSelectionDialogType;
    private AlertDialog mSelectionDialog;

    private ArrayList<RomInformation> mRoms;
    private RomInformation mCurrentRom;
    private RomInformation mSelectedRom;

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

            int progressDialogType = savedInstanceState.getInt(EXTRA_PROGRESS_DIALOG);
            if (progressDialogType > 0) {
                buildProgressDialog(progressDialogType);
            }

            int confirmDialogType = savedInstanceState.getInt(EXTRA_CONFIRM_DIALOG);
            if (confirmDialogType > 0) {
                buildConfirmDialog(confirmDialogType);
            }

            int inputDialogType = savedInstanceState.getInt(EXTRA_INPUT_DIALOG);
            if (inputDialogType > 0) {
                buildInputDialog(inputDialogType);
            }

            int selectionDialogType = savedInstanceState.getInt(EXTRA_SELECTION_DIALOG);
            if (selectionDialogType > 0) {
                buildSelectionDialog(selectionDialogType);
            }
        }

        mProgressBar = (ProgressBar) getActivity().findViewById(R.id.card_list_loading);

        mFabFlashZip = (FloatingActionButton) getActivity()
                .findViewById(R.id.fab_flash_zip);
        mFabFlashZip.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                Intent intent = new Intent(getActivity(), ZipFlashingActivity.class);
                startActivityForResult(intent, REQUEST_FLASH_ZIP);
            }
        });

        initErrorCard();
        initCardList();
        refreshErrorVisibility(false);

        // Show progress bar on initial load, not on rotation
        if (savedInstanceState != null) {
            refreshProgressVisibility(false);
        }

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
    public void onStop() {
        super.onStop();

        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }

        if (mConfirmDialog != null) {
            mConfirmDialog.dismiss();
            mConfirmDialog = null;
        }

        if (mInputDialog != null) {
            mInputDialog.dismiss();
            mInputDialog = null;
        }

        if (mSelectionDialog != null) {
            mSelectionDialog.dismiss();
            mSelectionDialog = null;
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean(EXTRA_PERFORMING_ACTION, mPerformingAction);
        if (mSelectedRom != null) {
            outState.putParcelable(EXTRA_SELECTED_ROM, mSelectedRom);
        }
        if (mProgressDialog != null) {
            outState.putInt(EXTRA_PROGRESS_DIALOG, mProgressDialogType);
        }
        if (mConfirmDialog != null) {
            outState.putInt(EXTRA_CONFIRM_DIALOG, mConfirmDialogType);
        }
        if (mInputDialog != null) {
            outState.putInt(EXTRA_INPUT_DIALOG, mInputDialogType);
        }
        if (mSelectionDialog != null) {
            outState.putInt(EXTRA_SELECTION_DIALOG, mSelectionDialogType);
        }
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

    @Override
    public void onDismiss(DialogInterface dialog) {
        if (mProgressDialog == dialog) {
            mProgressDialog = null;
        }
        if (mConfirmDialog == dialog) {
            mConfirmDialog = null;
        }
        if (mInputDialog == dialog) {
            mInputDialog = null;
        }
        if (mSelectionDialog == dialog) {
            mSelectionDialog = null;
        }
    }

    private void refreshProgressVisibility(boolean visible) {
        mProgressBar.setVisibility(visible ? View.VISIBLE : View.GONE);
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
            getActivity().getWindow().addFlags(
                    WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // Show progress spinner in navigation bar
            ((MainActivity) getActivity()).showProgress(MainActivity.FRAGMENT_ROMS, true);
        } else {
            // Don't keep screen on
            getActivity().getWindow().clearFlags(
                    WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // Hide progress spinner in navigation bar
            ((MainActivity) getActivity()).showProgress(MainActivity.FRAGMENT_ROMS, false);
        }
    }

    private void buildProgressDialog(int type) {
        if (mProgressDialog != null) {
            throw new IllegalStateException("Tried to create progress dialog twice!");
        }

        int titleResId;
        int messageResId;

        switch (type) {
        case PROGRESS_DIALOG_SWITCH_ROM:
            titleResId = R.string.switching_rom;
            messageResId = R.string.please_wait;
            break;
        case PROGRESS_DIALOG_SET_KERNEL:
            titleResId = R.string.setting_kernel;
            messageResId = R.string.please_wait;
            break;
        case PROGRESS_DIALOG_WIPE_ROM:
            titleResId = R.string.wiping_targets;
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
        case CONFIRM_DIALOG_SET_KERNEL:
            String message = String.format(getString(R.string.switcher_ask_set_kernel_desc),
                    mSelectedRom.getName());

            mConfirmDialog = new MaterialDialog.Builder(getActivity())
                    .title(R.string.switcher_ask_set_kernel_title)
                    .content(message)
                    .positiveText(R.string.proceed)
                    .negativeText(R.string.cancel)
                    .callback(new ButtonCallback() {
                        @Override
                        public void onPositive(MaterialDialog dialog) {
                            setKernel(mSelectedRom);
                        }
                    })
                    .build();
            break;

        case CONFIRM_DIALOG_WIPING_EXPERIMENTAL:
            mConfirmDialog = new MaterialDialog.Builder(getActivity())
                    .content(R.string.wipe_rom_dialog_warning_message)
                    .positiveText(R.string.proceed)
                    .callback(new ButtonCallback() {
                        @Override
                        public void onPositive(MaterialDialog dialog) {
                            buildSelectionDialog(SELECTION_DIALOG_WIPE_TARGETS);
                        }
                    })
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

    private void buildInputDialog(int type) {
        if (mInputDialog != null) {
            throw new IllegalStateException("Tried to create input dialog twice!");
        }

        switch (type) {
        case INPUT_DIALOG_EDIT_NAME:
            String title = String.format(getString(R.string.rename_rom_title),
                    mSelectedRom.getDefaultName());
            String message = String.format(getString(R.string.rename_rom_desc),
                    mSelectedRom.getDefaultName());

            mInputDialog = new MaterialDialog.Builder(getActivity())
                    .title(title)
                    .customView(R.layout.dialog_textbox, true)
                    .positiveText(R.string.ok)
                    .negativeText(R.string.cancel)
                    .callback(new ButtonCallback() {
                        @Override
                        public void onPositive(MaterialDialog dialog) {
                            EditText et = (EditText) dialog.findViewById(R.id.edittext);
                            String newName = et.getText().toString().trim();

                            if (newName.isEmpty()) {
                                mSelectedRom.setName(null);
                            } else {
                                mSelectedRom.setName(newName);
                            }

                            new Thread() {
                                @Override
                                public void run() {
                                    RomUtils.saveConfig(mSelectedRom);
                                }
                            }.start();
                            mRomCardAdapter.notifyDataSetChanged();
                        }
                    })
                    .build();

            TextView tv = (TextView)
                    ((MaterialDialog) mInputDialog).getCustomView().findViewById(R.id.message);
            tv.setText(message);
            break;
        default:
            throw new IllegalStateException("Invalid input dialog type");
        }

        mInputDialogType = type;

        mInputDialog.setOnDismissListener(this);
        mInputDialog.setCanceledOnTouchOutside(false);
        mInputDialog.setCancelable(false);
        mInputDialog.show();
    }

    private void buildSelectionDialog(int type) {
        if (mSelectionDialog != null) {
            throw new IllegalStateException("Tried to create selection dialog twice!");
        }

        switch (type) {
        case SELECTION_DIALOG_WIPE_TARGETS:
            final String[] items = new String[] {
                    getString(R.string.wipe_target_system),
                    getString(R.string.wipe_target_cache),
                    getString(R.string.wipe_target_data),
                    getString(R.string.wipe_target_dalvik_cache),
                    getString(R.string.wipe_target_multiboot_files)
            };

            mSelectionDialog = new MaterialDialog.Builder(getActivity())
                    .title(R.string.wipe_rom_dialog_title)
                    .items(items)
                    .negativeText(R.string.cancel)
                    .positiveText(R.string.ok)
                    .itemsCallbackMultiChoice(null, new ListCallbackMulti() {
                        @Override
                        public void onSelection(MaterialDialog dialog, Integer[] which,
                                                CharSequence[] text) {
                            if (which.length == 0) {
                                Toast.makeText(getActivity(), R.string.wipe_rom_none_selected,
                                        Toast.LENGTH_LONG).show();
                                return;
                            }

                            short[] targets = new short[which.length];

                            for (int i = 0; i < which.length; i++) {
                                int arrIndex = which[i];

                                if (arrIndex == 0) {
                                    targets[i] = WipeTarget.SYSTEM;
                                } else if (arrIndex == 1) {
                                    targets[i] = WipeTarget.CACHE;
                                } else if (arrIndex == 2) {
                                    targets[i] = WipeTarget.DATA;
                                } else if (arrIndex == 3) {
                                    targets[i] = WipeTarget.DALVIK_CACHE;
                                } else if (arrIndex == 4) {
                                    targets[i] = WipeTarget.MULTIBOOT;
                                }
                            }

                            wipeRom(mSelectedRom, targets);
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
            if (mProgressDialog != null) {
                mProgressDialog.dismiss();
                mProgressDialog = null;
            }
            Toast.makeText(getActivity(),
                    event.failed ? R.string.choose_rom_failure : R.string.choose_rom_success,
                    Toast.LENGTH_SHORT).show();
        } else if (bEvent instanceof SetKernelEvent) {
            SetKernelEvent event = (SetKernelEvent) bEvent;
            mPerformingAction = false;
            updateCardUI();
            if (mProgressDialog != null) {
                mProgressDialog.dismiss();
                mProgressDialog = null;
            }
            Toast.makeText(getActivity(),
                    event.failed ? R.string.set_kernel_failure : R.string.set_kernel_success,
                    Toast.LENGTH_SHORT).show();
        } else if (bEvent instanceof WipedRomEvent) {
            WipedRomEvent event = (WipedRomEvent) bEvent;
            mPerformingAction = false;
            updateCardUI();
            if (mProgressDialog != null) {
                mProgressDialog.dismiss();
                mProgressDialog = null;
            }

            if (event.succeeded == null || event.failed == null) {
                Toast.makeText(getActivity(), R.string.wipe_rom_initiate_fail, Toast.LENGTH_LONG)
                        .show();
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

                Toast.makeText(getActivity(), sb.toString(), Toast.LENGTH_LONG).show();

                // We don't want deleted ROMs to still show up
                reloadFragment();
            }
        }
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
        mPerformingAction = true;
        updateCardUI();

        buildProgressDialog(PROGRESS_DIALOG_SWITCH_ROM);
        mEventCollector.chooseRom(info.getId());
    }

    @Override
    public void onSelectedSetKernel(RomInformation info) {
        mSelectedRom = info;

        // Ask for confirmation
        buildConfirmDialog(CONFIRM_DIALOG_SET_KERNEL);
    }

    private void setKernel(RomInformation info) {
        mPerformingAction = true;
        updateCardUI();

        buildProgressDialog(PROGRESS_DIALOG_SET_KERNEL);
        mEventCollector.setKernel(info.getId());
    }

    @Override
    public void onSelectedEditName(RomInformation info) {
        mSelectedRom = info;

        buildInputDialog(INPUT_DIALOG_EDIT_NAME);
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
            Toast.makeText(getActivity(), R.string.wipe_rom_no_wipe_current_rom,
                    Toast.LENGTH_LONG).show();
        } else {
            buildConfirmDialog(CONFIRM_DIALOG_WIPING_EXPERIMENTAL);
        }
    }

    private void wipeRom(RomInformation info, short[] targets) {
        mPerformingAction = true;
        updateCardUI();

        buildProgressDialog(PROGRESS_DIALOG_WIPE_ROM);
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
        } else {
            refreshErrorVisibility(true);
        }

        mCurrentRom = result.currentRom;

        mRomCardAdapter.notifyDataSetChanged();
        updateCardUI();

        refreshProgressVisibility(false);
        refreshRomListVisibility(true);
        refreshFabVisibility(true);
    }

    @Override
    public void onLoaderReset(Loader<LoaderResult> loader) {
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
            return mResult;
        }
    }

    protected static class LoaderResult {
        RomInformation[] roms;
        RomInformation currentRom;
    }
}
