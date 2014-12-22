/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ProgressBar;
import android.widget.ScrollView;

import com.github.chenxiaolong.dualbootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.patcher.MainOptsCard.MainOptsSelectedListener;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.FinishedPatchingEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.RequestedFileEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.SetMaxProgressEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.SetProgressEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherEventCollector.UpdateDetailsEvent;
import com.github.chenxiaolong.dualbootpatcher.patcher.PresetCard.PresetSelectedListener;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.FileInfo;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PartConfig;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatchInfo;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.Card.OnCardClickListener;
import it.gmariotti.cardslib.library.internal.Card.OnLongCardClickListener;
import it.gmariotti.cardslib.library.view.CardViewNative;

public class PatchFileFragment extends Fragment implements EventCollectorListener,
        MainOptsSelectedListener, PresetSelectedListener {
    public static final String TAG = PatchFileFragment.class.getSimpleName();

    private static final String EXTRA_CONFIG_STATE = "config_state";

    private boolean mShowingProgress;

    private PatcherEventCollector mEventCollector;

    private Bundle mSavedInstanceState;

    private ScrollView mScrollView;
    private ProgressBar mProgressBar;

    private MainOptsCard mMainOptsCard;
    private FileChooserCard mFileChooserCard;
    private PresetCard mPresetCard;
    private CustomOptsCard mCustomOptsCard;
    private DetailsCard mDetailsCard;
    private ProgressCard mProgressCard;

    private CardViewNative mMainOptsCardView;
    private CardViewNative mFileChooserCardView;
    private CardViewNative mPresetCardView;
    private CardViewNative mCustomOptsCardView;
    private CardViewNative mDetailsCardView;
    private CardViewNative mProgressCardView;

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
        mMainOptsCard = new MainOptsCard(getActivity(), mPCS, this);
        mMainOptsCardView = (CardViewNative) getActivity().findViewById(R.id.card_mainopts);
        mMainOptsCardView.setCard(mMainOptsCard);
    }

    private void initFileChooserCard() {
        // Card for choosing the file to patch, starting the patching
        // process, and resetting the patching process
        mFileChooserCard = new FileChooserCard(getActivity(), mPCS);
        mFileChooserCard.setClickable(true);

        if (mPCS.mState == PatcherConfigState.STATE_CHOSE_FILE) {
            setTapActionPatchFile();
        } else {
            setTapActionChooseFile();
        }

        mFileChooserCardView = (CardViewNative) getActivity().findViewById(R.id.card_file_chooser);
        mFileChooserCardView.setCard(mFileChooserCard);
    }

    private void initPresetCard() {
        // Card that allows the user to select a preset patchinfo to patch an unsupported file
        mPresetCard = new PresetCard(getActivity(), mPCS, this);
        mPresetCardView = (CardViewNative) getActivity().findViewById(R.id.card_preset);
        mPresetCardView.setCard(mPresetCard);
    }

    private void initCustomOptsCard() {
        // Card that allows the user to manually change the patcher options when patching an
        // unsupported file
        mCustomOptsCard = new CustomOptsCard(getActivity(), mPCS);
        mCustomOptsCardView = (CardViewNative) getActivity().findViewById(R.id.card_customopts);
        mCustomOptsCardView.setCard(mCustomOptsCard);
    }

    private void initDetailsCard() {
        // Card to show patching details
        mDetailsCard = new DetailsCard(getActivity(), mPCS);
        mDetailsCardView = (CardViewNative) getActivity().findViewById(R.id.card_details);
        mDetailsCardView.setCard(mDetailsCard);
    }

    private void initProgressCard() {
        // Card to show a progress bar for the patching process (really only
        // the compression part though, since that takes the longest)
        mProgressCard = new ProgressCard(getActivity(), mPCS);
        mProgressCardView = (CardViewNative) getActivity().findViewById(R.id.card_progress);
        mProgressCardView.setCard(mProgressCard);
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
            mMainOptsCard.onRestoreInstanceState(mSavedInstanceState);
            mFileChooserCard.onRestoreInstanceState(mSavedInstanceState);
            mDetailsCard.onRestoreInstanceState(mSavedInstanceState);
            mProgressCard.onRestoreInstanceState(mSavedInstanceState);
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

        mMainOptsCard.onSaveInstanceState(outState);
        mFileChooserCard.onSaveInstanceState(outState);
        mDetailsCard.onSaveInstanceState(outState);
        mProgressCard.onSaveInstanceState(outState);
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
        mMainOptsCard.refreshState();
        mFileChooserCard.refreshState();
        mPresetCard.refreshState();
        mCustomOptsCard.refreshState();
        mDetailsCard.refreshState();
        mProgressCard.refreshState();

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

        mMainOptsCard.refreshPatchers();
        mMainOptsCard.refreshDevices();
        // PartConfigs are initialized when a patcher is selected

        mPresetCard.refreshPresets();

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
                        mCustomOptsCard.isHasBootImageEnabled());

                if (mPCS.mPatchInfo.hasBootImage(PatchInfo.Default())) {
                    mPCS.mPatchInfo.setRamdisk(PatchInfo.Default(),
                            mPCS.mDevice.getCodename() + "/default");

                    String bootImagesText = mCustomOptsCard.getBootImage();
                    if (bootImagesText != null) {
                        String[] bootImages = bootImagesText.split(",");
                        mPCS.mPatchInfo.setBootImages(PatchInfo.Default(), bootImages);
                    }
                }

                mPCS.mPatchInfo.setDeviceCheck(PatchInfo.Default(),
                        mCustomOptsCard.isDeviceCheckEnabled());
            }
        }

        FileInfo fileInfo = new FileInfo();
        fileInfo.setFilename(mPCS.mFilename);
        fileInfo.setDevice(mMainOptsCard.getDevice());
        fileInfo.setPartConfig(mMainOptsCard.getPartConfig());
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
        mFileChooserCard.setProgressShowing(true);

        mPCS.mSupported = PatcherConfigState.NOT_SUPPORTED;

        // If the patcher doesn't use the patchinfo files, then just assume everything is supported.

        if (!mPCS.mPatcher.usesPatchInfo()) {
            mPCS.mSupported |= PatcherConfigState.SUPPORTED_FILE;
            mPCS.mSupported |= PatcherConfigState.SUPPORTED_PARTCONFIG;
        }

        // Otherwise, check if it really is supported
        else if ((mPCS.mPatchInfo = PatcherUtils.sPC.findMatchingPatchInfo(
                mMainOptsCard.getDevice(), mPCS.mFilename)) != null) {
            mPCS.mSupported |= PatcherConfigState.SUPPORTED_FILE;

            final String key = mPCS.mPatchInfo.keyFromFilename(mPCS.mFilename);
            String[] configs = mPCS.mPatchInfo.getSupportedConfigs(key);
            PartConfig curConfig = mMainOptsCard.getPartConfig();

            boolean hasAll = false;
            boolean hasCurConfig = false;
            boolean hasNotCurConfig = false;

            for (String config : configs) {
                if (config.equals("all")) {
                    hasAll = true;
                } else if (config.equals(curConfig.getId())) {
                    hasCurConfig = true;
                } else if (config.equals("!" + curConfig.getId())) {
                    hasNotCurConfig = true;
                }
            }

            if ((hasAll || hasCurConfig) && !hasNotCurConfig) {
                mPCS.mSupported |= PatcherConfigState.SUPPORTED_PARTCONFIG;
            }
        }

        setTapActionPatchFile();

        mFileChooserCard.onFileChosen();
        mFileChooserCard.setProgressShowing(false);

        updateCardUI();
    }

    private void setTapActionChooseFile() {
        mFileChooserCard.setOnClickListener(new OnCardClickListener() {
            @Override
            public void onClick(Card card, View view) {
                mEventCollector.startFileChooser();
            }
        });

        mFileChooserCard.setOnLongClickListener(null);
    }

    private void setTapActionPatchFile() {
        mFileChooserCard.setOnClickListener(new OnCardClickListener() {
            @Override
            public void onClick(Card card, View view) {
                startPatching();
            }
        });

        mFileChooserCard.setOnLongClickListener(new OnLongCardClickListener() {
            @Override
            public boolean onLongClick(Card card, View view) {
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

        boolean changedPatcher = false;

        if (mPCS.mPatcher == null) {
            mPCS.mPatcher = PatcherUtils.sPC.createPatcher(patcherId);
            changedPatcher = true;
        }

        // Rebuild list of partconfigs supported by the patcher and recheck to see if the
        // selected file is supported
        refreshPartConfigs(changedPatcher);
        checkSupported();
    }

    /**
     * Refresh partition configuration after a patcher has been selected.
     */
    private void refreshPartConfigs(boolean changedPatcher) {
        if (changedPatcher) {
            mPCS.refreshPartConfigs();
        }
        mMainOptsCard.refreshPartConfigs(changedPatcher);
    }

    @Override
    public void onDeviceSelected(Device device) {
        mPCS.mDevice = device;
        mPCS.mPatchInfos = PatcherUtils.sPC.getPatchInfos(mPCS.mDevice);

        checkSupported();

        // Reload presets specific to the device
        mPresetCard.refreshPresets();
    }

    @Override
    public void onPartConfigSelected(PartConfig config) {
        mPCS.mPartConfig = config;

        // Recheck to see if the partition configuration is supported for the selected file
        checkSupported();
    }

    @Override
    public void onPresetSelected(PatchInfo info) {
        // Disable custom options if a preset is selected
        mCustomOptsCard.setUsingPreset(info != null);

        mPCS.mPatchInfo = info;
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof UpdateDetailsEvent) {
            UpdateDetailsEvent e = (UpdateDetailsEvent) event;

            mDetailsCard.setDetails(e.text);
        } else if (event instanceof SetMaxProgressEvent) {
            SetMaxProgressEvent e = (SetMaxProgressEvent) event;

            mProgressCard.setMaxProgress(e.maxProgress);
        } else if (event instanceof SetProgressEvent) {
            SetProgressEvent e = (SetProgressEvent) event;

            mProgressCard.setProgress(e.progress);
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
            mFileChooserCard.onFinishedPatching(e.failed, e.message, e.newFile);

            // Reset for next round of patching
            mPresetCard.reset();
            mCustomOptsCard.reset();
            mDetailsCard.reset();
            mProgressCard.reset();

            // Tap to choose the next file
            setTapActionChooseFile();
            mFileChooserCardView.refreshCard(mFileChooserCard);
        } else if (event instanceof RequestedFileEvent) {
            mPCS.mState = PatcherConfigState.STATE_CHOSE_FILE;

            RequestedFileEvent e = (RequestedFileEvent) event;
            mPCS.mFilename = e.file;
            checkSupported();
        }
    }
}
