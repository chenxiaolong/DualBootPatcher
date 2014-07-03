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

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.Card.OnCardClickListener;
import it.gmariotti.cardslib.library.internal.CardArrayAdapter;
import it.gmariotti.cardslib.library.view.CardListView;
import it.gmariotti.cardslib.library.view.CardView;

import java.util.ArrayList;

import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ProgressBar;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherTaskFragment.ChoseRomListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherTaskFragment.SetKernelListener;

public class SwitcherListFragment extends Fragment implements ChoseRomListener,
        SetKernelListener {
    public static final String TAG_CHOOSE_ROM = "choose_rom";
    public static final String TAG_SET_KERNEL = "set_kernel";
    public static final int ACTION_CHOOSE_ROM = 1;
    public static final int ACTION_SET_KERNEL = 2;

    private boolean mShowingProgress;
    private boolean mPerformingAction;
    private boolean mAttemptedRoot;
    private boolean mHaveRootAccess;

    private SwitcherTaskFragment mTaskFragment;

    private int mCardListResId;

    private NoRootCard mNoRootCard;
    private CardView mNoRootCardView;
    private ArrayList<Card> mCards;
    private CardArrayAdapter mCardArrayAdapter;
    private CardListView mCardListView;
    private ProgressBar mProgressBar;
    private int mAction;
    private RomInformation[] mRoms;

    private static SwitcherListFragment newInstance() {
        SwitcherListFragment f = new SwitcherListFragment();

        return f;
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

        FragmentManager fm = getFragmentManager();
        mTaskFragment = (SwitcherTaskFragment) fm
                .findFragmentByTag(SwitcherTaskFragment.TAG);

        if (mTaskFragment == null) {
            mTaskFragment = new SwitcherTaskFragment();
            fm.beginTransaction().add(mTaskFragment, SwitcherTaskFragment.TAG)
                    .commit();
        }

        if (getArguments() != null) {
            mAction = getArguments().getInt("action");
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (savedInstanceState != null) {
            mAttemptedRoot = savedInstanceState.getBoolean("attemptedRoot");
            mHaveRootAccess = savedInstanceState.getBoolean("haveRootAccess");
        }

        // Get root access
        if (!mAttemptedRoot) {
            mAttemptedRoot = true;
            mHaveRootAccess = CommandUtils.requestRootAccess();
        }

        if (mAction == ACTION_CHOOSE_ROM) {
            mProgressBar = (ProgressBar) getActivity().findViewById(
                    R.id.card_list_loading_choose_rom);
        } else if (mAction == ACTION_SET_KERNEL) {
            mProgressBar = (ProgressBar) getActivity().findViewById(
                    R.id.card_list_loading_set_kernel);
        }

        initNoRootCard();

        if (!mHaveRootAccess) {
            mProgressBar.setVisibility(View.GONE);
        } else {
            mNoRootCardView.setVisibility(View.GONE);
            initCards(savedInstanceState);
        }
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
            mTaskFragment.attachChoseRomListener(this);
        } else if (mAction == ACTION_SET_KERNEL) {
            mTaskFragment.attachSetKernelListener(this);
        }
    }

    @Override
    public void onPause() {
        super.onPause();
        if (mAction == ACTION_CHOOSE_ROM) {
            mTaskFragment.detachChoseRomListener();
        } else if (mAction == ACTION_SET_KERNEL) {
            mTaskFragment.detachSetKernelListener();
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean("performingAction", mPerformingAction);
        outState.putBoolean("attemptedRoot", mAttemptedRoot);
        outState.putBoolean("haveRootAccess", mHaveRootAccess);

        // mCards will be null when switching to the SuperUser/SuperSU activity
        // for approving root access
        if (mCards != null) {
            for (int i = 0; i < mCards.size(); i++) {
                RomCard card = (RomCard) mCards.get(i);
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
                mAttemptedRoot = false;
                reloadFragment();
            }
        });

        int id = 0;

        if (mAction == ACTION_CHOOSE_ROM) {
            id = R.id.card_noroot_choose_rom;
        } else if (mAction == ACTION_SET_KERNEL) {
            id = R.id.card_noroot_set_kernel;
        }

        mNoRootCardView = (CardView) getActivity().findViewById(id);
        mNoRootCardView.setCard(mNoRootCard);
    }

    private void initCards(Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            mPerformingAction = savedInstanceState
                    .getBoolean("performingAction");
        }

        ArrayList<Card> cards = new ArrayList<Card>();
        mCardArrayAdapter = new CardArrayAdapter(getActivity(), cards);

        mCardListView = (CardListView) getActivity().findViewById(
                mCardListResId);
        if (mCardListView != null) {
            mCardListView.setAdapter(mCardArrayAdapter);
        }

        mShowingProgress = true;
        updateMainUI();

        Context context = getActivity().getApplicationContext();
        new CardLoaderTask(context, savedInstanceState).execute();
    }

    private class CardLoaderTask extends AsyncTask<Void, Void, Void> {
        private final Context mContext;
        private final Bundle mSavedInstanceState;
        private String[] mNames;
        private String[] mVersions;
        private int[] mImageResIds;

        public CardLoaderTask(Context context, Bundle savedInstanceState) {
            mContext = context;
            mSavedInstanceState = savedInstanceState;
        }

        @Override
        protected Void doInBackground(Void... params) {
            if (mRoms == null) {
                mRoms = RomUtils.getRoms();
            }

            mNames = new String[mRoms.length];
            mVersions = new String[mRoms.length];
            mImageResIds = new int[mRoms.length];

            for (int i = 0; i < mRoms.length; i++) {
                mNames[i] = RomUtils.getName(mContext, mRoms[i]);
                mVersions[i] = RomUtils.getVersion(mRoms[i]);
                if (mVersions[i] == null) {
                    mVersions[i] = getActivity().getString(
                            R.string.couldnt_determine_version);
                }
                mImageResIds[i] = RomUtils.getIconResource(mRoms[i]);
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            // Don't die on a configuration change
            if (getActivity() == null) {
                return;
            }

            mCards = new ArrayList<Card>();

            for (int i = 0; i < mRoms.length; i++) {
                final RomInformation info = mRoms[i];

                RomCard card = new RomCard(getActivity(), info, mNames[i],
                        mVersions[i], mImageResIds[i]);

                if (mSavedInstanceState != null) {
                    card.onRestoreInstanceState(mSavedInstanceState);
                }

                card.setOnClickListener(new OnCardClickListener() {
                    @Override
                    public void onClick(Card card, View view) {
                        startAction(info);
                    }
                });

                mCards.add(card);
            }

            mCardArrayAdapter.addAll(mCards);
            mCardArrayAdapter.notifyDataSetChanged();
            updateCardUI();

            mShowingProgress = false;
            updateMainUI();
        }
    }

    private void updateMainUI() {
        if (mShowingProgress) {
            mCardListView.setVisibility(View.GONE);
            mProgressBar.setVisibility(View.VISIBLE);
        } else {
            mCardListView.setVisibility(View.VISIBLE);
            mProgressBar.setVisibility(View.GONE);
        }
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
        for (int i = 0; i < mCards.size(); i++) {
            RomCard card = (RomCard) mCards.get(i);
            if (info.equals(card.getRom())) {
                return card;
            }
        }
        return null;
    }

    private RomCard findCardFromId(String kernelId) {
        for (int i = 0; i < mCards.size(); i++) {
            RomCard card = (RomCard) mCards.get(i);
            if (card.getRom().kernelId.equals(kernelId)) {
                return card;
            }
        }
        return null;
    }

    private void showCompletionMessage(RomCard card, boolean failed) {
        for (int i = 0; i < mCards.size(); i++) {
            RomCard curCard = (RomCard) mCards.get(i);
            if (curCard == card) {
                curCard.showCompletionMessage(mAction, failed);
            } else {
                curCard.hideMessage();
            }
        }
        mCardArrayAdapter.notifyDataSetChanged();
    }

    private void startAction(RomInformation info) {
        RomCard card = findCard(info);
        if (card != null) {
            showProgress(card, true);
        }

        mPerformingAction = true;
        updateCardUI();

        if (mAction == ACTION_CHOOSE_ROM) {
            chooseRom(info.kernelId);
        } else if (mAction == ACTION_SET_KERNEL) {
            setKernel(info.kernelId);
        }
    }

    private void chooseRom(String kernelId) {
        Context context = getActivity().getApplicationContext();
        Intent intent = new Intent(context, SwitcherService.class);
        intent.putExtra(SwitcherService.ACTION,
                SwitcherService.ACTION_CHOOSE_ROM);
        intent.putExtra("kernelId", kernelId);
        context.startService(intent);
    }

    private void setKernel(String kernelId) {
        Context context = getActivity().getApplicationContext();
        Intent intent = new Intent(context, SwitcherService.class);
        intent.putExtra(SwitcherService.ACTION,
                SwitcherService.ACTION_SET_KERNEL);
        intent.putExtra("kernelId", kernelId);
        context.startService(intent);
    }

    @Override
    public void onChoseRom(boolean failed, String message, String kernelId) {
        RomCard card = findCardFromId(kernelId);
        if (card != null) {
            showProgress(card, false);
        }

        mPerformingAction = false;
        updateCardUI();

        showCompletionMessage(card, failed);
    }

    @Override
    public void onSetKernel(boolean failed, String message, String kernelId) {
        RomCard card = findCardFromId(kernelId);
        if (card != null) {
            showProgress(card, false);
        }

        mPerformingAction = false;
        updateCardUI();

        showCompletionMessage(card, failed);
    }
}
