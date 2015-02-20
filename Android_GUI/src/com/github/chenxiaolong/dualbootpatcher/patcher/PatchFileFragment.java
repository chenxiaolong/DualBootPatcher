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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Context;
import android.content.Intent;
import android.media.MediaScannerConnection;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.widget.CardView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnLongClickListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ProgressBar;
import android.widget.ScrollView;

import com.github.chenxiaolong.dualbootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.patcher.MainOptsCW.MainOptsListener;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.FinishedPatchingEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.RequestedFileEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.SetMaxProgressEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.SetProgressEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.UpdateDetailsEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PresetCW.PresetListener;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.FileInfo;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatchInfo;

public class PatchFileFragment extends Fragment implements EventCollectorListener,
        MainOptsListener, PresetListener {
    public static final String TAG = PatchFileFragment.class.getSimpleName();

    private static final String EXTRA_CONFIG_STATE = "config_state";

    private boolean mShowingProgress;

    private PatcherEventCollector mEventCollector;

    private Bundle mSavedInstanceState;

    private ScrollView mScrollView;
    private ProgressBar mProgressBar;

    // CardView wrappers
    private MainOptsCW mMainOptsCW;
    private FileChooserCW mFileChooserCW;
    private PresetCW mPresetCW;
    private CustomOptsCW mCustomOptsCW;
    private DetailsCW mDetailsCW;
    private ProgressCW mProgressCW;

    private CardView mMainOptsCardView;
    private CardView mFileChooserCardView;
    private CardView mPresetCardView;
    private CardView mCustomOptsCardView;
    private CardView mDetailsCardView;
    private CardView mProgressCardView;

    private PatcherConfigState mPCS;

    public static PatchFileFragment newInstance() {
        return new PatchFileFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        FragmentManager fm = getFragmentManager();
        mEventCollector = (PatcherEventCollector) fm.findFragmentByTag(PatcherEventCollector.TAG);

        if (mEventCollector == null) {
            mEventCollector = new PatcherEventCollector();
            fm.beginTransaction().add(mEventCollector, PatcherEventCollector.TAG).commit();
        }

        if (savedInstanceState != null) {
            mPCS = savedInstanceState.getParcelable(EXTRA_CONFIG_STATE);
        } else {
            mPCS = new PatcherConfigState();
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);
        mSavedInstanceState = savedInstanceState;

        mScrollView = (ScrollView) getActivity().findViewById(R.id.card_scrollview);
        mProgressBar = (ProgressBar) getActivity().findViewById(R.id.card_loading_patch_file);

        initCards();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_patch_file, container, false);
    }

    private void initMainOptsCard() {
        // Card for selecting the device and partition configuration
        mMainOptsCardView = (CardView) getActivity().findViewById(R.id.card_mainopts);
        mMainOptsCW = new MainOptsCW(getActivity(), mPCS, mMainOptsCardView, this);
    }

    private void initFileChooserCard() {
        // Card for choosing the file to patch, starting the patching
        // process, and resetting the patching process
        mFileChooserCardView = (CardView) getActivity().findViewById(R.id.card_file_chooser);
        mFileChooserCW = new FileChooserCW(getActivity(), mPCS, mFileChooserCardView);
        mFileChooserCardView.setClickable(true);

        if (mPCS.mState == PatcherConfigState.STATE_CHOSE_FILE) {
            setTapActionPatchFile();
        } else {
            setTapActionChooseFile();
        }
    }

    private void initPresetCard() {
        // Card that allows the user to select a preset patchinfo to patch an unsupported file
        mPresetCardView = (CardView) getActivity().findViewById(R.id.card_preset);
        mPresetCW = new PresetCW(getActivity(), mPCS, mPresetCardView, this);
    }

    private void initCustomOptsCard() {
        // Card that allows the user to manually change the patcher options when patching an
        // unsupported file
        mCustomOptsCardView = (CardView) getActivity().findViewById(R.id.card_customopts);
        mCustomOptsCW = new CustomOptsCW(getActivity(), mPCS, mCustomOptsCardView);
    }

    private void initDetailsCard() {
        // Card to show patching details
        mDetailsCardView = (CardView) getActivity().findViewById(R.id.card_details);
        mDetailsCW = new DetailsCW(mPCS, mDetailsCardView);
    }

    private void initProgressCard() {
        // Card to show a progress bar for the patching process (really only
        // the compression part though, since that takes the longest)
        mProgressCardView = (CardView) getActivity().findViewById(R.id.card_progress);
        mProgressCW = new ProgressCW(getActivity(), mPCS, mProgressCardView);
    }

    private void initCards() {
        mShowingProgress = true;
        updateMainUI();

        initMainOptsCard();
        initFileChooserCard();
        initPresetCard();
        initCustomOptsCard();
        initDetailsCard();
        initProgressCard();

        // Update UI based on patch state
        updateCardUI();

        // Initialize patcher
        if (PatcherUtils.sPC == null) {
            Context context = getActivity().getApplicationContext();
            new PatcherInitializationTask(context).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
        } else {
            // Ensures that everything is needed right away (needed for, eg. orientation change)
            patcherInitialized();
        }
    }

    private void restoreCardStates() {
        if (mSavedInstanceState != null) {
            mFileChooserCW.onRestoreInstanceState(mSavedInstanceState);
            mDetailsCW.onRestoreInstanceState(mSavedInstanceState);
            mProgressCW.onRestoreInstanceState(mSavedInstanceState);
        }
    }

    private class PatcherInitializationTask extends AsyncTask<Void, Void, Void> {
        private final Context mContext;

        public PatcherInitializationTask(Context context) {
            mContext = context;
        }

        @Override
        protected Void doInBackground(Void... params) {
            PatcherUtils.initializePatcher(mContext);
            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            // Don't die on a configuration change
            if (getActivity() == null) {
                return;
            }

            patcherInitialized();
        }
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
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putParcelable(EXTRA_CONFIG_STATE, mPCS);

        mFileChooserCW.onSaveInstanceState(outState);
        mDetailsCW.onSaveInstanceState(outState);
        mProgressCW.onSaveInstanceState(outState);
    }

    private void updateMainUI() {
        if (mShowingProgress) {
            mScrollView.setVisibility(View.GONE);
            mProgressBar.setVisibility(View.VISIBLE);
        } else {
            mScrollView.setVisibility(View.VISIBLE);
            mProgressBar.setVisibility(View.GONE);
        }
    }

    private void updateCardUI() {
        mMainOptsCW.refreshState();
        mFileChooserCW.refreshState();
        mPresetCW.refreshState();
        mCustomOptsCW.refreshState();
        mDetailsCW.refreshState();
        mProgressCW.refreshState();

        switch (mPCS.mState) {
        case PatcherConfigState.STATE_PATCHING:
            // Keep screen on
            getActivity().getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // Show progress spinner in navigation bar
            ((MainActivity) getActivity()).showProgress(MainActivity.FRAGMENT_PATCH_FILE, true);

            break;

        case PatcherConfigState.STATE_INITIAL:
        case PatcherConfigState.STATE_FINISHED:
            // Don't keep screen on
            getActivity().getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // Hide progress spinner in navigation bar
            ((MainActivity) getActivity()).showProgress(MainActivity.FRAGMENT_PATCH_FILE, false);

            break;
        }
    }

    private void patcherInitialized() {
        mPCS.setupInitial();

        mMainOptsCW.refreshPatchers();
        mMainOptsCW.refreshDevices();

        mPresetCW.refreshPresets();

        restoreCardStates();

        mShowingProgress = false;
        updateMainUI();
    }

    private void startPatching() {
        mPCS.mState = PatcherConfigState.STATE_PATCHING;
        updateCardUI();

        // Scroll to the bottom
        mScrollView.post(new Runnable() {
            @Override
            public void run() {
                mScrollView.fullScroll(View.FOCUS_DOWN);
            }
        });

        if (mPCS.mSupported == PatcherConfigState.NOT_SUPPORTED) {
            if (mPCS.mPatchInfo == null) {
                mPCS.mPatchInfo = new PatchInfo();

                mPCS.mPatchInfo.addAutoPatcher(PatchInfo.Default(), "StandardPatcher", null);

                mPCS.mPatchInfo.setHasBootImage(PatchInfo.Default(),
                        mCustomOptsCW.isHasBootImageEnabled());

                if (mPCS.mPatchInfo.hasBootImage(PatchInfo.Default())) {
                    mPCS.mPatchInfo.setRamdisk(PatchInfo.Default(),
                            mPCS.mDevice.getId() + "/default");

                    String bootImagesText = mCustomOptsCW.getBootImage();
                    if (bootImagesText != null) {
                        String[] bootImages = bootImagesText.split(",");
                        mPCS.mPatchInfo.setBootImages(PatchInfo.Default(), bootImages);
                    }
                }

                mPCS.mPatchInfo.setDeviceCheck(PatchInfo.Default(),
                        mCustomOptsCW.isDeviceCheckEnabled());
            }
        }

        FileInfo fileInfo = new FileInfo();
        fileInfo.setFilename(mPCS.mFilename);
        fileInfo.setDevice(mMainOptsCW.getDevice());
        fileInfo.setPatchInfo(mPCS.mPatchInfo);

        Context context = getActivity().getApplicationContext();
        Intent intent = new Intent(context, PatcherService.class);
        intent.putExtra(PatcherService.ACTION, PatcherService.ACTION_PATCH_FILE);
        intent.putExtra(PatcherUtils.PARAM_PATCHER, mPCS.mPatcher);
        intent.putExtra(PatcherUtils.PARAM_FILEINFO, fileInfo);

        context.startService(intent);
    }

    private void checkSupported() {
        if (mPCS.mState != PatcherConfigState.STATE_CHOSE_FILE) {
            return;
        }

        // Show progress on file chooser card
        mFileChooserCW.setProgressShowing(true);

        mPCS.mSupported = PatcherConfigState.NOT_SUPPORTED;

        // If the patcher doesn't use the patchinfo files, then just assume everything is supported.

        if (!mPCS.mPatcher.usesPatchInfo()) {
            mPCS.mSupported |= PatcherConfigState.SUPPORTED_FILE;
        }

        // Otherwise, check if it really is supported
        else if ((mPCS.mPatchInfo = PatcherUtils.sPC.findMatchingPatchInfo(
                mMainOptsCW.getDevice(), mPCS.mFilename)) != null) {
            mPCS.mSupported |= PatcherConfigState.SUPPORTED_FILE;
        }

        setTapActionPatchFile();

        mFileChooserCW.onFileChosen();
        mFileChooserCW.setProgressShowing(false);

        updateCardUI();
    }

    private void setTapActionChooseFile() {
        mFileChooserCardView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                mEventCollector.startFileChooser();
            }
        });

        mFileChooserCardView.setOnLongClickListener(null);
    }

    private void setTapActionPatchFile() {
        mFileChooserCardView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                startPatching();
            }
        });

        mFileChooserCardView.setOnLongClickListener(new OnLongClickListener() {
            @Override
            public boolean onLongClick(View view) {
                mEventCollector.startFileChooser();
                return true;
            }
        });
    }

    @Override
    public void onPatcherSelected(String patcherName) {
        String patcherId = mPCS.mReversePatcherMap.get(patcherName);

        // Destroy the native C object when the patcher is switched
        if (mPCS.mPatcher != null && !mPCS.mPatcher.getId().equals(patcherId)) {
            PatcherUtils.sPC.destroyPatcher(mPCS.mPatcher);
            mPCS.mPatcher = null;
        }

        if (mPCS.mPatcher == null) {
            mPCS.mPatcher = PatcherUtils.sPC.createPatcher(patcherId);
        }

        // Recheck to see if the selected file is supported
        checkSupported();
    }

    @Override
    public void onDeviceSelected(Device device) {
        mPCS.mDevice = device;
        mPCS.mPatchInfos = PatcherUtils.sPC.getPatchInfos(mPCS.mDevice);

        checkSupported();

        // Reload presets specific to the device
        mPresetCW.refreshPresets();
    }

    @Override
    public void onPresetSelected(PatchInfo info) {
        // Disable custom options if a preset is selected
        mCustomOptsCW.setUsingPreset(info != null);

        mPCS.mPatchInfo = info;
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof UpdateDetailsEvent) {
            UpdateDetailsEvent e = (UpdateDetailsEvent) event;

            mDetailsCW.setDetails(e.text);
        } else if (event instanceof SetMaxProgressEvent) {
            SetMaxProgressEvent e = (SetMaxProgressEvent) event;

            mProgressCW.setMaxProgress(e.maxProgress);
        } else if (event instanceof SetProgressEvent) {
            SetProgressEvent e = (SetProgressEvent) event;

            mProgressCW.setProgress(e.progress);
        } else if (event instanceof FinishedPatchingEvent) {
            FinishedPatchingEvent e = (FinishedPatchingEvent) event;

            mPCS.mState = PatcherConfigState.STATE_FINISHED;
            updateCardUI();

            // Update MTP cache
            if (!e.failed) {
                MediaScannerConnection.scanFile(getActivity(),
                        new String[] { e.newFile }, null, null);
            }

            // Scroll back to the top
            mScrollView.post(new Runnable() {
                @Override
                public void run() {
                    mScrollView.fullScroll(View.FOCUS_UP);
                }
            });

            // Display message
            mFileChooserCW.onFinishedPatching(e.failed, e.message, e.newFile);

            // Reset for next round of patching
            mPresetCW.reset();
            mCustomOptsCW.reset();
            mDetailsCW.reset();
            mProgressCW.reset();

            // Tap to choose the next file
            setTapActionChooseFile();
        } else if (event instanceof RequestedFileEvent) {
            mPCS.mState = PatcherConfigState.STATE_CHOSE_FILE;

            RequestedFileEvent e = (RequestedFileEvent) event;
            mPCS.mFilename = e.file;
            checkSupported();
        }
    }
}
