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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ProgressBar;

import com.github.chenxiaolong.dualbootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.RootCheckerEventCollector;
import com.github.chenxiaolong.dualbootpatcher.RootCheckerEventCollector.RootAcknowledgedEvent;
import com.github.chenxiaolong.dualbootpatcher.RootFile;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.ChoseRomEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.SetKernelEvent;
import com.nhaarman.listviewanimations.swinginadapters.AnimationAdapter;
import com.nhaarman.listviewanimations.swinginadapters.prepared.AlphaInAnimationAdapter;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Iterator;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.Card.OnCardClickListener;
import it.gmariotti.cardslib.library.internal.Card.OnLongCardClickListener;
import it.gmariotti.cardslib.library.internal.CardArrayAdapter;
import it.gmariotti.cardslib.library.view.CardListView;
import it.gmariotti.cardslib.library.view.CardViewNative;

public class SwitcherListFragment extends Fragment implements OnDismissListener,
        EventCollectorListener {
    public static final String TAG_CHOOSE_ROM = SwitcherListFragment.class.getSimpleName() + "1";
    public static final String TAG_SET_KERNEL = SwitcherListFragment.class.getSimpleName() + "2";
    public static final int ACTION_CHOOSE_ROM = 1;
    public static final int ACTION_SET_KERNEL = 2;

    private static final String EXTRA_PERFORMING_ACTION = "performingAction";
    private static final String EXTRA_SELECTED_ROM_ID = "selectedRomId";
    private static final String EXTRA_SHOWING_DIALOG = "showingDialog";
    private static final String EXTRA_SHOWING_RENAME_DIALOG = "showingRenameDialog";
    private static final String EXTRA_ROMS = "roms";
    private static final String EXTRA_ROM_NAMES = "romNames";
    private static final String EXTRA_ROM_IMAGE_RES_IDS = "romImageResIds";
    private static final String EXTRA_ROM_DIALOG_NAME = "romDialogName";

    private static final int REQUEST_IMAGE = 1234;

    private boolean mPerformingAction;

    private SwitcherEventCollector mEventCollector;
    private RootCheckerEventCollector mRootChecker;

    private Bundle mSavedInstanceState;

    private int mCardListResId;

    private NoRootCard mNoRootCard;
    private CardViewNative mNoRootCardView;
    private ArrayList<Card> mCards;
    private CardArrayAdapter mCardArrayAdapter;
    private CardListView mCardListView;
    private ProgressBar mProgressBar;
    private int mAction;
    private boolean mCardsLoaded;

    private ArrayList<BaseEvent> mEvents = new ArrayList<BaseEvent>();

    private AlertDialog mDialog;
    private AlertDialog mRenameDialog;
    private RomInformation mSelectedRom;
    private RomDialogCard mRomDialogCard;
    private boolean mRebootDialogShowing;
    private String mRomDialogName;

    private RomInformation[] mRoms;
    private String[] mRomNames;
    private int[] mRomImageResIds;

    private static SwitcherListFragment newInstance() {
        return new SwitcherListFragment();
    }

    public static SwitcherListFragment newInstance(int action) {
        SwitcherListFragment f = newInstance();

        Bundle args = new Bundle();
        args.putInt("action", action);

        f.setArguments(args);

        return f;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (getArguments() != null) {
            mAction = getArguments().getInt("action");
        }

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

        mRootChecker = RootCheckerEventCollector.getInstance(getFragmentManager());

        mSavedInstanceState = savedInstanceState;

        if (savedInstanceState != null) {
            mPerformingAction = savedInstanceState.getBoolean(EXTRA_PERFORMING_ACTION);

            mRoms = (RomInformation[]) savedInstanceState.getParcelableArray(EXTRA_ROMS);
            mRomNames = savedInstanceState.getStringArray(EXTRA_ROM_NAMES);
            mRomImageResIds = savedInstanceState.getIntArray(EXTRA_ROM_IMAGE_RES_IDS);

            String selectedRomId = savedInstanceState.getString(EXTRA_SELECTED_ROM_ID);
            if (selectedRomId != null) {
                mSelectedRom = RomUtils.getRomFromId(selectedRomId);
            }

            if (savedInstanceState.getBoolean(EXTRA_SHOWING_DIALOG)) {
                buildDialog();
            }

            if (savedInstanceState.getBoolean(EXTRA_SHOWING_RENAME_DIALOG)) {
                mRebootDialogShowing = true;
            }

            mRomDialogName = savedInstanceState.getString(EXTRA_ROM_DIALOG_NAME);
        }

        if (mAction == ACTION_CHOOSE_ROM) {
            mProgressBar = (ProgressBar) getActivity().findViewById(
                    R.id.card_list_loading_choose_rom);
        } else if (mAction == ACTION_SET_KERNEL) {
            mProgressBar = (ProgressBar) getActivity().findViewById(
                    R.id.card_list_loading_set_kernel);
        }

        initNoRootCard();
        mNoRootCardView.setVisibility(View.GONE);

        initCardList();

        // Show progress bar on initial load, not on rotation
        if (savedInstanceState != null) {
            refreshProgressVisibility(false);
        }

        if (mAction == ACTION_CHOOSE_ROM) {
            mRootChecker.createListener(TAG_CHOOSE_ROM);
        } else if (mAction == ACTION_SET_KERNEL) {
            mRootChecker.createListener(TAG_SET_KERNEL);
        }

        mRootChecker.requestRoot();
    }

    private void reloadFragment() {
        FragmentManager fm = getFragmentManager();
        FragmentTransaction ft = fm.beginTransaction();
        ft.detach(this);
        ft.attach(this);
        ft.commit();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        if (mAction == ACTION_CHOOSE_ROM) {
            mCardListResId = R.id.card_list_choose_rom;

            return inflater.inflate(R.layout.fragment_switcher_choose_rom,
                    container, false);
        } else if (mAction == ACTION_SET_KERNEL) {
            mCardListResId = R.id.card_list_set_kernel;

            return inflater.inflate(R.layout.fragment_switcher_set_kernel,
                    container, false);
        }

        return null;
    }

    @Override
    public void onResume() {
        super.onResume();

        if (mAction == ACTION_CHOOSE_ROM) {
            mEventCollector.attachListener(TAG_CHOOSE_ROM, this);
            mRootChecker.attachListener(TAG_CHOOSE_ROM, this);
        } else if (mAction == ACTION_SET_KERNEL) {
            mEventCollector.attachListener(TAG_SET_KERNEL, this);
            mRootChecker.attachListener(TAG_SET_KERNEL, this);
        }
    }

    @Override
    public void onPause() {
        super.onPause();

        if (mAction == ACTION_CHOOSE_ROM) {
            mEventCollector.detachListener(TAG_CHOOSE_ROM);
            mRootChecker.detachListener(TAG_CHOOSE_ROM);
        } else if (mAction == ACTION_SET_KERNEL) {
            mEventCollector.detachListener(TAG_SET_KERNEL);
            mRootChecker.detachListener(TAG_SET_KERNEL);
        }
    }

    @Override
    public void onStart() {
        super.onStart();

        if (mRebootDialogShowing) {
            buildRenameDialog();
            if (mRomDialogName != null) {
                mRomDialogCard.setName(mRomDialogName);
            }
        }
    }

    @Override
    public void onStop() {
        super.onStop();

        if (mDialog != null) {
            mDialog.dismiss();
            mDialog = null;
        }

        if (mRenameDialog != null) {
            mRenameDialog.dismiss();
            mRenameDialog = null;
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean(EXTRA_PERFORMING_ACTION, mPerformingAction);
        if (mSelectedRom != null) {
            outState.putString(EXTRA_SELECTED_ROM_ID, mSelectedRom.id);
        }
        if (mDialog != null) {
            outState.putBoolean(EXTRA_SHOWING_DIALOG, true);
        }
        if (mRenameDialog != null) {
            outState.putBoolean(EXTRA_SHOWING_RENAME_DIALOG, mRebootDialogShowing);
        }
        if (mRomDialogCard != null) {
            outState.putString(EXTRA_ROM_DIALOG_NAME, mRomDialogCard.getName());
        }

        if (mRoms != null) {
            outState.putParcelableArray(EXTRA_ROMS, mRoms);
            outState.putStringArray(EXTRA_ROM_NAMES, mRomNames);
            outState.putIntArray(EXTRA_ROM_IMAGE_RES_IDS, mRomImageResIds);
        }

        // mCards will be null when switching to the SuperUser/SuperSU activity
        // for approving root access
        if (mCards != null) {
            processQueuedEvents();

            for (Card c : mCards) {
                RomCard card = (RomCard) c;
                card.onSaveInstanceState(outState);
            }
        }
    }

    private void initNoRootCard() {
        mNoRootCard = new NoRootCard(getActivity());
        mNoRootCard.setClickable(true);
        mNoRootCard.setOnClickListener(new OnCardClickListener() {
            @Override
            public void onClick(Card card, View view) {
                mRootChecker.resetAttempt();
                reloadFragment();
            }
        });

        int id = 0;

        if (mAction == ACTION_CHOOSE_ROM) {
            id = R.id.card_noroot_choose_rom;
        } else if (mAction == ACTION_SET_KERNEL) {
            id = R.id.card_noroot_set_kernel;
        }

        mNoRootCardView = (CardViewNative) getActivity().findViewById(id);
        mNoRootCardView.setCard(mNoRootCard);
    }

    private void initCardList() {
        mCards = new ArrayList<>();
        mCardArrayAdapter = new CardArrayAdapter(getActivity(), mCards);

        mCardListView = (CardListView) getActivity().findViewById(mCardListResId);
        if (mCardListView != null) {
            AnimationAdapter animArrayAdapter = new AlphaInAnimationAdapter(mCardArrayAdapter);
            animArrayAdapter.setAbsListView(mCardListView);
            mCardListView.setExternalAdapter(animArrayAdapter, mCardArrayAdapter);
            refreshRomListVisibility(false);
        }
    }

    private void initCards() {
        Context context = getActivity().getApplicationContext();
        new ObtainRomsTask(context).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        if (mDialog == dialog) {
            mDialog = null;
        }
        if (mRenameDialog == dialog) {
            mRenameDialog = null;
            mRomDialogCard = null;
        }
    }

    private void refreshProgressVisibility(boolean visible) {
        mProgressBar.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    private void refreshRomListVisibility(boolean visible) {
        mCardListView.setVisibility(visible ? View.VISIBLE : View.GONE);
    }

    private void updateCardUI() {
        if (mPerformingAction) {
            // Keep screen on
            getActivity().getWindow().addFlags(
                    WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // Show progress spinner in navigation bar
            switch (mAction) {
            case ACTION_CHOOSE_ROM:
                ((MainActivity) getActivity()).showProgress(
                        MainActivity.FRAGMENT_CHOOSE_ROM, true);
                break;
            case ACTION_SET_KERNEL:
                ((MainActivity) getActivity()).showProgress(
                        MainActivity.FRAGMENT_SET_KERNEL, true);
                break;
            }

            // Disable all cards
            for (int i = 0; mCards != null && i < mCards.size(); i++) {
                RomCard card = (RomCard) mCards.get(i);
                card.setEnabled(false);
            }
            mCardArrayAdapter.notifyDataSetChanged();
        } else {
            // Don't keep screen on
            getActivity().getWindow().clearFlags(
                    WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

            // Hide progress spinner in navigation bar
            switch (mAction) {
            case ACTION_CHOOSE_ROM:
                ((MainActivity) getActivity()).showProgress(
                        MainActivity.FRAGMENT_CHOOSE_ROM, false);
                break;
            case ACTION_SET_KERNEL:
                ((MainActivity) getActivity()).showProgress(
                        MainActivity.FRAGMENT_SET_KERNEL, false);
                break;
            }

            // Re-enable all cards
            for (int i = 0; mCards != null && i < mCards.size(); i++) {
                RomCard card = (RomCard) mCards.get(i);
                card.setEnabled(true);
            }
            mCardArrayAdapter.notifyDataSetChanged();
        }
    }

    private void showProgress(RomCard card, boolean show) {
        card.hideMessage();
        card.setProgressShowing(show);
    }

    private RomCard findCard(RomInformation info) {
        for (Card c : mCards) {
            RomCard card = (RomCard) c;
            if (info.equals(card.getRom())) {
                return card;
            }
        }
        return null;
    }

    private RomCard findCardFromId(String id) {
        for (Card c : mCards) {
            RomCard card = (RomCard) c;
            if (card.getRom().id.equals(id)) {
                return card;
            }
        }
        return null;
    }

    private void showCompletionMessage(RomCard card, boolean failed) {
        for (Card c : mCards) {
            RomCard curCard = (RomCard) c;
            if (curCard == card) {
                curCard.showCompletionMessage(mAction, failed);
            } else {
                curCard.hideMessage();
            }
        }
        mCardArrayAdapter.notifyDataSetChanged();
    }

    private void buildDialog() {
        if (mDialog == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            builder.setTitle(R.string.switcher_ask_set_kernel_title);

            String message = String.format(
                    getActivity().getString(R.string.switcher_ask_set_kernel_desc),
                    RomUtils.getName(getActivity(), mSelectedRom));

            builder.setMessage(message);

            DialogInterface.OnClickListener listener = new OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    startAction(mSelectedRom);
                }
            };

            builder.setPositiveButton(R.string.proceed, listener);
            builder.setNegativeButton(R.string.cancel, null);
            mDialog = builder.show();
            mDialog.setOnDismissListener(this);
        }
    }

    private void buildRenameDialog() {
        if (mRenameDialog == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());

            String defaultName = RomUtils.getDefaultName(getActivity(), mSelectedRom);
            String name = RomUtils.getName(getActivity(), mSelectedRom);
            builder.setTitle(String.format(getActivity().getString(
                    R.string.rename_rom_title), defaultName));
            builder.setMessage(String.format(getActivity().getString(
                    R.string.rename_rom_desc), defaultName));

            LayoutInflater inflater = getActivity().getLayoutInflater();

            View v = inflater.inflate(R.layout.dialog_rename_rom, null);

            View.OnClickListener listener = new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    Intent intent = new Intent();
                    intent.setType("image/*");
                    intent.setAction(Intent.ACTION_GET_CONTENT);
                    intent.addCategory(Intent.CATEGORY_OPENABLE);
                    startActivityForResult(intent, REQUEST_IMAGE);
                }
            };

            final File tempThumbnail = new File(mSelectedRom.thumbnailPath + ".tmp");

            // Save a temporary copy of the current image when the dialog first appears. All
            // changes are done on the temporary image and then copied back if the OK button is
            // pressed.
            try {
                File curThumbnail = new File(mSelectedRom.thumbnailPath);
                if (curThumbnail.isFile() && !tempThumbnail.exists() && !mRebootDialogShowing) {
                    org.apache.commons.io.FileUtils.copyFile(curThumbnail, tempThumbnail);
                }
            } catch (IOException e) {
                e.printStackTrace();
            }

            int imageResId = RomUtils.getIconResource(mSelectedRom);

            mRomDialogCard = new RomDialogCard(getActivity(), mSelectedRom,
                    name, imageResId, listener);

            CardViewNative cardView = (CardViewNative) v.findViewById(R.id.rom_card);
            cardView.setCard(mRomDialogCard);

            builder.setView(v);

            builder.setPositiveButton(R.string.ok, new OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    RootFile thumbnail = new RootFile(mSelectedRom.thumbnailPath, false);

                    if (tempThumbnail.isFile()) {
                        RootFile temp = new RootFile(tempThumbnail, false);
                        temp.moveTo(thumbnail);
                        thumbnail.chmod(0755);
                    } else {
                        thumbnail.delete();
                    }

                    RomUtils.setName(mSelectedRom, mRomDialogCard.getName());
                    refreshNames();

                    mRebootDialogShowing = false;
                }
            });

            builder.setNegativeButton(R.string.cancel, new OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    if (tempThumbnail.isFile()) {
                        tempThumbnail.delete();
                    }

                    mRebootDialogShowing = false;
                }
            });

            builder.setNeutralButton(R.string.reset_icon, new OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    // Dummy
                }
            });

            mRenameDialog = builder.show();
            mRenameDialog.setOnDismissListener(this);
            mRenameDialog.setCanceledOnTouchOutside(false);

            // Set listener on the button after the dialog is created so the dialog isn't dismissed
            ((AlertDialog) mRenameDialog).getButton(AlertDialog.BUTTON_NEUTRAL)
                    .setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    tempThumbnail.delete();

                    refreshRenameDialog();
                }
            });

            mRebootDialogShowing = true;
        }
    }

    private void refreshRenameDialog() {
        if (mRenameDialog != null) {
            CardViewNative cardView = (CardViewNative) mRenameDialog.findViewById(R.id.rom_card);
            cardView.refreshCard(mRomDialogCard);
        }
    }

    private void refreshNames() {
        for (int i = 0; i < mRoms.length; i++) {
            final RomInformation info = mRoms[i];
            final RomCard card = (RomCard) mCards.get(i);

            String name = RomUtils.getName(getActivity(), info);
            mRomNames[i] = name;
            card.setName(name);
        }

        mCardArrayAdapter.notifyDataSetChanged();
    }

    private void startActionAskingIfNeeded(final RomInformation info) {
        if (mAction == ACTION_CHOOSE_ROM) {
            startAction(info);
        } else if (mAction == ACTION_SET_KERNEL) {
            buildDialog();
        }
    }

    private void startAction(RomInformation info) {
        RomCard card = findCard(info);
        if (card != null) {
            showProgress(card, true);
        }

        mPerformingAction = true;
        updateCardUI();

        if (mAction == ACTION_CHOOSE_ROM) {
            mEventCollector.chooseRom(info.id);
        } else if (mAction == ACTION_SET_KERNEL) {
            mEventCollector.setKernel(info.id);
        }
    }

    private void processChoseRomEvent(ChoseRomEvent event) {
        RomCard card = findCardFromId(event.kernelId);
        if (card != null) {
            showProgress(card, false);
        }

        mPerformingAction = false;
        updateCardUI();

        showCompletionMessage(card, event.failed);
    }

    private void processSetKernelEvent(SetKernelEvent event) {
        RomCard card = findCardFromId(event.kernelId);
        if (card != null) {
            showProgress(card, false);
        }

        mPerformingAction = false;
        updateCardUI();

        showCompletionMessage(card, event.failed);
    }

    private void processQueuedEvents() {
        Iterator<BaseEvent> iter = mEvents.iterator();
        while (iter.hasNext()) {
            BaseEvent event = iter.next();

            if (event instanceof ChoseRomEvent) {
                processChoseRomEvent((ChoseRomEvent) event);
            } else if (event instanceof SetKernelEvent) {
                processSetKernelEvent((SetKernelEvent) event);
            }

            iter.remove();
        }
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
        }

        super.onActivityResult(request, result, data);
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof ChoseRomEvent) {
            ChoseRomEvent e = (ChoseRomEvent) event;

            if (mCardsLoaded) {
                processChoseRomEvent(e);
            } else {
                mEvents.add(event);
            }
        } else if (event instanceof SetKernelEvent) {
            SetKernelEvent e = (SetKernelEvent) event;

            if (mCardsLoaded) {
                processSetKernelEvent(e);
            } else {
                mEvents.add(event);
            }
        } else if (event instanceof RootAcknowledgedEvent) {
            RootAcknowledgedEvent e = (RootAcknowledgedEvent) event;

            if (!e.allowed) {
                mNoRootCardView.setVisibility(View.VISIBLE);
                refreshProgressVisibility(false);
            } else {
                mNoRootCardView.setVisibility(View.GONE);
                initCards();
            }
        }
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

                return ThumbnailUtils.extractThumbnail(bitmap, 96, 96);
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
            File f = new File(mSelectedRom.thumbnailPath);

            FileOutputStream out = null;

            try {
                out = new FileOutputStream(f);
                thumbnail.compress(Bitmap.CompressFormat.WEBP, 100, out);
            } catch (FileNotFoundException e) {
                e.printStackTrace();
            } finally {
                if (out != null) {
                    try {
                        out.close();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            if (getActivity() == null) {
                return;
            }

            refreshRenameDialog();
        }
    }

    protected class ObtainRomsTask extends AsyncTask<Void, Void, ObtainRomsTask.RomInfoResult> {
        private final Context mContext;

        public class RomInfoResult {
            RomInformation[] roms;
            String[] names;
            int[] imageResIds;
        }

        public ObtainRomsTask(Context context) {
            mContext = context;
        }

        @Override
        protected RomInfoResult doInBackground(Void... params) {
            // If roms were already loaded, don't load them again
            if (mRoms != null) {
                return null;
            }

            RomInfoResult result = new RomInfoResult();

            result.roms = RomUtils.getRoms();
            result.names = new String[result.roms.length];
            result.imageResIds = new int[result.roms.length];

            for (int i = 0; i < result.roms.length; i++) {
                result.names[i] = RomUtils.getName(mContext, result.roms[i]);
                result.imageResIds[i] = RomUtils.getIconResource(result.roms[i]);
            }

            return result;
        }

        @Override
        protected void onPostExecute(RomInfoResult result) {
            if (getActivity() == null) {
                return;
            }

            if (result != null) {
                mRoms = result.roms;
                mRomNames = result.names;
                mRomImageResIds = result.imageResIds;
            }

            mCards.clear();

            for (int i = 0; i < mRoms.length; i++) {
                final RomInformation info = mRoms[i];

                RomCard card = new RomCard(getActivity(), info, mRomNames[i], mRomImageResIds[i]);

                if (mSavedInstanceState != null) {
                    card.onRestoreInstanceState(mSavedInstanceState);
                }

                card.setOnClickListener(new OnCardClickListener() {
                    @Override
                    public void onClick(Card card, View view) {
                        mSelectedRom = info;
                        startActionAskingIfNeeded(info);
                    }
                });

                card.setOnLongClickListener(new OnLongCardClickListener() {
                    @Override
                    public boolean onLongClick(Card card, View view) {
                        mSelectedRom = info;
                        buildRenameDialog();
                        return true;
                    }
                });

                mCards.add(card);
            }

            mCardArrayAdapter.notifyDataSetChanged();
            updateCardUI();

            refreshProgressVisibility(false);
            refreshRomListVisibility(true);

            processQueuedEvents();

            mCardsLoaded = true;
        }
    }
}
