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

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.Card.OnCardClickListener;
import it.gmariotti.cardslib.library.internal.Card.OnLongCardClickListener;
import it.gmariotti.cardslib.library.view.CardView;
import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.content.Context;
import android.content.Intent;
import android.media.MediaScannerConnection;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ProgressBar;
import android.widget.ScrollView;

import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation.PatchInfo;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherTaskFragment.PatcherListener;

public class PatchFileFragment extends Fragment implements PatcherListener {
    public static final String TAG = PatchFileFragment.class.getSimpleName();

    private static final int STATE_READY = 0;
    private static final int STATE_CHOSE_FILE = 1;
    private static final int STATE_PATCHING = 2;

    private static final String ARG_FILENAME_OLD = "zipFile";
    private static final String ARG_FILENAME = "filename";
    private static final String ARG_DEVICE = "device";
    private static final String ARG_PARTCONFIG = "partConfig";

    private static final String RESULT_FILENAME_OLD = "newZipFile";
    private static final String RESULT_FILENAME = "filename";
    private static final String RESULT_MESSAGE = "message";
    private static final String RESULT_FAILED = "failed";

    private boolean mShowingProgress;
    private int mState;
    private boolean mAutomated;

    private PatcherTaskFragment mTaskFragment;

    private ScrollView mScrollView;
    private ProgressBar mProgressBar;

    private PatcherInformation mInfo;
    private DevicePartConfigCard mDevicePartConfigCard;
    private FileChooserCard mFileChooserCard;
    private PresetCard mPresetCard;
    private CustomOptsCard mCustomOptsCard;
    private TasksCard mTasksCard;
    private DetailsCard mDetailsCard;
    private ProgressCard mProgressCard;

    private CardView mDevicePartConfigCardView;
    private CardView mFileChooserCardView;
    private CardView mPresetCardView;
    private CardView mCustomOptsCardView;
    private CardView mTasksCardView;
    private CardView mDetailsCardView;
    private CardView mProgressCardView;

    // Work around onItemSelected() being called on initialization
    private boolean mDeviceSpinnerChanged;

    public static PatchFileFragment newInstance() {
        return new PatchFileFragment();
    }

    public static PatchFileFragment newAutomatedInstance(Bundle data) {
        PatchFileFragment f = new PatchFileFragment();

        Bundle args = (Bundle) data.clone();
        args.putBoolean("automated", true);
        if (args.containsKey(ARG_FILENAME_OLD)
                && !args.containsKey(ARG_FILENAME)) {
            args.putString(ARG_FILENAME, args.getString(ARG_FILENAME_OLD));
        }

        f.setArguments(args);

        return f;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        FragmentManager fm = getFragmentManager();
        mTaskFragment = (PatcherTaskFragment) fm
                .findFragmentByTag(PatcherTaskFragment.TAG);

        if (mTaskFragment == null) {
            mTaskFragment = new PatcherTaskFragment();
            fm.beginTransaction().add(mTaskFragment, PatcherTaskFragment.TAG)
                    .commit();
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (getArguments() != null) {
            mAutomated = getArguments().getBoolean("automated");
        }

        mScrollView = (ScrollView) getActivity().findViewById(
                R.id.card_scrollview);
        mProgressBar = (ProgressBar) getActivity().findViewById(
                R.id.card_loading_patch_file);

        initCards(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_patch_file, container, false);
    }

    private void initCards(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            mInfo = savedInstanceState.getParcelable("info");
            mState = savedInstanceState.getInt("state");
        }

        mShowingProgress = true;
        updateMainUI();

        // Card for selecting the device and partition configuration
        mDevicePartConfigCard = new DevicePartConfigCard(getActivity());

        mDevicePartConfigCardView = (CardView) getActivity().findViewById(
                R.id.card_device_partconfig);
        mDevicePartConfigCardView.setCard(mDevicePartConfigCard);

        // Card for choosing the file to patch, starting the patching
        // process, and resetting the patching process
        mFileChooserCard = new FileChooserCard(getActivity());
        mFileChooserCard.setClickable(true);

        if (savedInstanceState != null) {
            mFileChooserCard.onRestoreInstanceState(savedInstanceState);
        }

        if (mFileChooserCard.isFileSelected()) {
            setTapActionPatchFile();
        } else {
            setTapActionChooseFile();
        }

        mFileChooserCardView = (CardView) getActivity().findViewById(
                R.id.card_file_chooser);
        mFileChooserCardView.setCard(mFileChooserCard);

        // Card that allows the user to select a preset patchinfo to patch an
        // unsupported file
        mPresetCard = new PresetCard(getActivity());

        mPresetCardView = (CardView) getActivity().findViewById(
                R.id.card_preset);
        mPresetCardView.setCard(mPresetCard);

        // Card that allows the user to manually change the patcher options when
        // patching an unsupported file
        mCustomOptsCard = new CustomOptsCard(getActivity());

        if (savedInstanceState != null) {
            mCustomOptsCard.onRestoreInstanceState(savedInstanceState);
        }

        mCustomOptsCardView = (CardView) getActivity().findViewById(
                R.id.card_customopts);
        mCustomOptsCardView.setCard(mCustomOptsCard);

        // Card to show the list of tasks to perform during the patching
        // process
        mTasksCard = new TasksCard(getActivity());

        if (savedInstanceState != null) {
            mTasksCard.onRestoreInstanceState(savedInstanceState);
        }

        mTasksCardView = (CardView) getActivity().findViewById(R.id.card_tasks);
        mTasksCardView.setCard(mTasksCard);

        // Card to show more details on the current task (eg. files being
        // patched and copied, etc)
        mDetailsCard = new DetailsCard(getActivity());

        if (savedInstanceState != null) {
            mDetailsCard.onRestoreInstanceState(savedInstanceState);
        }

        mDetailsCardView = (CardView) getActivity().findViewById(
                R.id.card_details);
        mDetailsCardView.setCard(mDetailsCard);

        // Card to show a progress bar for the patching process (really only
        // the compression part though, since that takes the longest)
        mProgressCard = new ProgressCard(getActivity());

        if (savedInstanceState != null) {
            mProgressCard.onRestoreInstanceState(savedInstanceState);
        }

        mProgressCardView = (CardView) getActivity().findViewById(
                R.id.card_progress);
        mProgressCardView.setCard(mProgressCard);

        // Update UI based on patch state
        updateCardUI();

        if (mAutomated) {
            startAutomatedPatching();
            return;
        }

        // Load patcher information
        if (mInfo == null) {
            Context context = getActivity().getApplicationContext();
            new CardLoaderTask(context).execute();
        } else {
            // Ensures that everything is needed right away (needed for, eg.
            // orientation change)
            gotPatcherInformation();
        }
    }

    private class CardLoaderTask extends AsyncTask<Void, Void, Void> {
        private final Context mContext;

        public CardLoaderTask(Context context) {
            mContext = context;
        }

        @Override
        protected Void doInBackground(Void... params) {
            if (mInfo == null) {
                mInfo = PatcherUtils.getPatcherInformation(mContext);
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            // Don't die on a configuration change
            if (getActivity() == null) {
                return;
            }

            gotPatcherInformation();
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        mTaskFragment.attachListener(this);
    }

    @Override
    public void onPause() {
        super.onPause();
        mTaskFragment.detachListener();
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelable("info", mInfo);
        outState.putInt("state", mState);

        mFileChooserCard.onSaveInstanceState(outState);
        mCustomOptsCard.onSaveInstanceState(outState);
        mTasksCard.onSaveInstanceState(outState);
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
        switch (mState) {
        case STATE_PATCHING:
            // Keep screen on
            getActivity().getWindow().addFlags(
                    WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // Show progress spinner in navigation bar
            ((MainActivity) getActivity()).showProgress(
                    MainActivity.FRAGMENT_PATCH_FILE, true);

            // Disable all configuration cards
            mDevicePartConfigCard.setEnabled(false);
            mFileChooserCard.setEnabled(false);
            mFileChooserCardView.refreshCard(mFileChooserCard);
            mPresetCard.setEnabled(false);
            mCustomOptsCard.setEnabled(false);

            if (mFileChooserCard.isSupported()) {
                mPresetCardView.setVisibility(View.GONE);
                mCustomOptsCardView.setVisibility(View.GONE);
            } else {
                mPresetCardView.setVisibility(View.VISIBLE);
                mCustomOptsCardView.setVisibility(View.VISIBLE);
            }

            // Show progress cards
            mTasksCardView.setVisibility(View.VISIBLE);
            mDetailsCardView.setVisibility(View.VISIBLE);
            mProgressCardView.setVisibility(View.VISIBLE);

            break;

        case STATE_CHOSE_FILE:
            mDevicePartConfigCard.setEnabled(true);
            mFileChooserCard.setEnabled(true);
            mFileChooserCardView.refreshCard(mFileChooserCard);
            mPresetCard.setEnabled(true);
            mCustomOptsCard.setEnabled(true);

            if (mFileChooserCard.isSupported()) {
                mPresetCardView.setVisibility(View.GONE);
                mCustomOptsCardView.setVisibility(View.GONE);
            } else {
                mPresetCardView.setVisibility(View.VISIBLE);
                mCustomOptsCardView.setVisibility(View.VISIBLE);
            }

            // Hide progress cards
            mTasksCardView.setVisibility(View.GONE);
            mDetailsCardView.setVisibility(View.GONE);
            mProgressCardView.setVisibility(View.GONE);

            break;

        case STATE_READY:
            // Don't keep screen on
            getActivity().getWindow().clearFlags(
                    WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // Hide progress spinner in navigation bar
            ((MainActivity) getActivity()).showProgress(
                    MainActivity.FRAGMENT_PATCH_FILE, false);

            // Enable configuration cards again
            mDevicePartConfigCard.setEnabled(true);
            mFileChooserCard.setEnabled(true);
            mFileChooserCardView.refreshCard(mFileChooserCard);
            mPresetCard.setEnabled(true);
            mCustomOptsCard.setEnabled(true);

            // Hide unsupported file cards
            mPresetCardView.setVisibility(View.GONE);
            mCustomOptsCardView.setVisibility(View.GONE);

            // Hide progress cards
            mTasksCardView.setVisibility(View.GONE);
            mDetailsCardView.setVisibility(View.GONE);
            mProgressCardView.setVisibility(View.GONE);

            break;
        }

        // Always hide configuration cards in automated mode
        if (mAutomated) {
            mDevicePartConfigCardView.setVisibility(View.GONE);
            mFileChooserCardView.setVisibility(View.GONE);
            mPresetCardView.setVisibility(View.GONE);
            mCustomOptsCardView.setVisibility(View.GONE);
        }
    }

    private void gotPatcherInformation() {
        mDevicePartConfigCard.setPatcherInfo(mInfo);
        mPresetCard.setPatcherInfo(mInfo, mDevicePartConfigCard.getDevice());
        mCustomOptsCard
                .setPatcherInfo(mInfo, mDevicePartConfigCard.getDevice());
        setDevicePartConfigActions();
        setPresetActions();
        setCustomOptsActions();

        mShowingProgress = false;
        updateMainUI();
    }

    private void startAutomatedPatching() {
        mShowingProgress = false;
        updateMainUI();

        mState = STATE_PATCHING;
        updateCardUI();

        // Scroll to the bottom
        mScrollView.post(new Runnable() {
            @Override
            public void run() {
                mScrollView.fullScroll(View.FOCUS_DOWN);
            }
        });

        Context context = getActivity().getApplicationContext();
        Intent intent = new Intent(context, PatcherService.class);
        intent.putExtra(PatcherService.ACTION, PatcherService.ACTION_PATCH_FILE);

        String filename = getArguments().getString(ARG_FILENAME);

        intent.putExtra(PatcherUtils.PARAM_FILENAME, filename);

        String device = getArguments().getString(ARG_DEVICE);
        if (device == null) {
            device = Build.DEVICE;
        }

        intent.putExtra(PatcherUtils.PARAM_DEVICE, device);

        String partConfig = getArguments().getString(ARG_PARTCONFIG);
        intent.putExtra(PatcherUtils.PARAM_PARTCONFIG, partConfig);

        intent.putExtra(PatcherUtils.PARAM_SUPPORTED, true);

        context.startService(intent);
    }

    private void startPatching() {
        mState = STATE_PATCHING;
        updateCardUI();

        // Scroll to the bottom
        mScrollView.post(new Runnable() {
            @Override
            public void run() {
                mScrollView.fullScroll(View.FOCUS_DOWN);
            }
        });

        Context context = getActivity().getApplicationContext();
        Intent intent = new Intent(context, PatcherService.class);
        intent.putExtra(PatcherService.ACTION, PatcherService.ACTION_PATCH_FILE);

        intent.putExtra(PatcherUtils.PARAM_FILENAME,
                mFileChooserCard.getFilename());
        intent.putExtra(PatcherUtils.PARAM_DEVICE,
                mDevicePartConfigCard.getDevice().mCodeName);
        intent.putExtra(PatcherUtils.PARAM_PARTCONFIG,
                mDevicePartConfigCard.getPartConfig().mId);

        boolean supported = mFileChooserCard.isSupported();
        intent.putExtra(PatcherUtils.PARAM_SUPPORTED, supported);

        if (!supported) {
            PatchInfo preset = mPresetCard.getPreset();
            if (preset != null) {
                intent.putExtra(PatcherUtils.PARAM_PRESET, preset);
            }

            boolean useAutopatcher = mCustomOptsCard.isUsingAutopatcher();
            intent.putExtra(PatcherUtils.PARAM_USE_AUTOPATCHER, useAutopatcher);
            if (useAutopatcher) {
                intent.putExtra(PatcherUtils.PARAM_AUTOPATCHER,
                        mCustomOptsCard.getAutopatcher());
            }

            boolean usePatch = mCustomOptsCard.isUsingPatch();
            intent.putExtra(PatcherUtils.PARAM_USE_PATCH, usePatch);
            if (usePatch) {
                intent.putExtra(PatcherUtils.PARAM_PATCH,
                        mCustomOptsCard.getPatch());
            }

            intent.putExtra(PatcherUtils.PARAM_DEVICE_CHECK,
                    mCustomOptsCard.isDeviceCheckEnabled());

            boolean hasBootImage = mCustomOptsCard.isHasBootImageEnabled();
            intent.putExtra(PatcherUtils.PARAM_HAS_BOOT_IMAGE, hasBootImage);
            if (hasBootImage) {
                intent.putExtra(PatcherUtils.PARAM_RAMDISK,
                        mCustomOptsCard.getRamdisk());

                String init = mCustomOptsCard.getPatchedInit();
                if (init != null) {
                    intent.putExtra(PatcherUtils.PARAM_PATCHED_INIT, init);
                }

                intent.putExtra(PatcherUtils.PARAM_BOOT_IMAGE,
                        mCustomOptsCard.getBootImage());
            }
        }

        context.startService(intent);
    }

    @Override
    public void finishedPatching(boolean failed, String message, String newFile) {
        mState = STATE_READY;
        updateCardUI();

        // Update MTP cache
        if (!failed) {
            MediaScannerConnection.scanFile(getActivity(),
                    new String[] { newFile }, null, null);
        }

        if (mAutomated) {
            Intent data = new Intent();
            data.putExtra(RESULT_FILENAME_OLD, newFile);
            data.putExtra(RESULT_FILENAME, newFile);
            data.putExtra(RESULT_MESSAGE, message);
            data.putExtra(RESULT_FAILED, failed);
            getActivity().setResult(Activity.RESULT_OK, data);
            getActivity().finish();
            return;
        }

        // Scroll back to the top
        mScrollView.post(new Runnable() {
            @Override
            public void run() {
                mScrollView.fullScroll(View.FOCUS_UP);
            }
        });

        // Display message
        mFileChooserCard.onFinishedPatching(failed, message, newFile);

        // Reset for next round of patching
        mPresetCard.reset();
        mCustomOptsCard.reset();
        mTasksCard.reset();
        mDetailsCard.reset();
        mProgressCard.reset();

        // Tap to choose the next file
        setTapActionChooseFile();
        mFileChooserCardView.refreshCard(mFileChooserCard);
    }

    private void checkSupported(String filename) {
        // Show progress on file chooser card
        mFileChooserCard.setProgressShowing(true);

        String device = mDevicePartConfigCard.getDevice().mCodeName;
        String partConfig = mDevicePartConfigCard.getPartConfig().mId;

        Context context = getActivity().getApplicationContext();
        Intent intent = new Intent(context, PatcherService.class);
        intent.putExtra(PatcherService.ACTION,
                PatcherService.ACTION_CHECK_SUPPORTED);
        intent.putExtra(PatcherUtils.PARAM_FILENAME, filename);
        intent.putExtra(PatcherUtils.PARAM_PARTCONFIG, partConfig);
        intent.putExtra(PatcherUtils.PARAM_DEVICE, device);
        context.startService(intent);
    }

    @Override
    public void checkedSupported(String zipFile, boolean supported) {
        mFileChooserCard.onFileChosen(zipFile, supported);
        setTapActionPatchFile();
        mFileChooserCardView.refreshCard(mFileChooserCard);
        mFileChooserCard.setProgressShowing(false);

        mState = STATE_CHOSE_FILE;
        updateCardUI();
    }

    private void setTapActionChooseFile() {
        mFileChooserCard.setOnClickListener(new OnCardClickListener() {
            @Override
            public void onClick(Card card, View view) {
                mTaskFragment.startFileChooser();
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
                mTaskFragment.startFileChooser();
                return true;
            }
        });
    }

    private void setDevicePartConfigActions() {
        mDevicePartConfigCard.mDeviceSpinner
                .setOnItemSelectedListener(new OnItemSelectedListener() {
                    @Override
                    public void onItemSelected(AdapterView<?> parent,
                            View view, int position, long id) {
                        // Work around onItemSelected() being called on
                        // initialization
                        if (mDeviceSpinnerChanged) {
                            // Recheck to determine if the file is supported
                            // when the target device is changed
                            String file = mFileChooserCard.getFilename();
                            if (file != null) {
                                checkSupported(file);
                            }
                        }

                        // Reload presets specific to the device
                        mPresetCard.reloadPresets(mDevicePartConfigCard
                                .getDevice());

                        // Reload ramdisks specific to the device
                        mCustomOptsCard.reloadRamdisks(mDevicePartConfigCard
                                .getDevice());

                        mDeviceSpinnerChanged = true;
                    }

                    @Override
                    public void onNothingSelected(AdapterView<?> parent) {
                    }
                });
    }

    private void setPresetActions() {
        mPresetCard.mPresetSpinner
                .setOnItemSelectedListener(new OnItemSelectedListener() {
                    @Override
                    public void onItemSelected(AdapterView<?> parent,
                            View view, int position, long id) {
                        mPresetCard.updatePresetDescription(position);
                        mCustomOptsCard.setUsingPreset(position != 0);
                    }

                    @Override
                    public void onNothingSelected(AdapterView<?> parent) {
                    }
                });
    }

    private void setCustomOptsActions() {
        mCustomOptsCard.mChoosePatchButton
                .setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        mTaskFragment.startFileChooserDiff();
                    }
                });
    }

    @Override
    public void addTask(String task) {
        mTasksCard.addTask(task);
    }

    @Override
    public void updateTask(String text) {
        mTasksCard.updateTask(text);
    }

    @Override
    public void updateDetails(String text) {
        mDetailsCard.setDetails(text);
    }

    @Override
    public void setProgress(int i) {
        mProgressCard.setProgress(i);
    }

    @Override
    public void setMaxProgress(int i) {
        mProgressCard.setMaxProgress(i);
    }

    @Override
    public void requestedFile(String file) {
        checkSupported(file);
    }

    @Override
    public void requestedDiffFile(String file) {
        mCustomOptsCard.onDiffFileSelected(file);
    }
}
