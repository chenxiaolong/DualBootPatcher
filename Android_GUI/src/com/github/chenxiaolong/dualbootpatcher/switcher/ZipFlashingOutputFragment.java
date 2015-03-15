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
import android.os.Bundle;
import android.os.Parcelable;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.FlashedZipsEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.NewOutputEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction;
import com.github.chenxiaolong.multibootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.multibootpatcher.EventCollector.EventCollectorListener;

import org.apache.commons.io.output.NullOutputStream;

import java.io.IOException;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.nio.charset.Charset;

import jackpal.androidterm.emulatorview.EmulatorView;
import jackpal.androidterm.emulatorview.TermSession;

public class ZipFlashingOutputFragment extends Fragment implements EventCollectorListener {
    public static final String TAG = ZipFlashingOutputFragment.class.getSimpleName();

    private static final String EXTRA_IS_RUNNING = "is_running";

    public static final String PARAM_PENDING_ACTIONS = "pending_actions";

    // Just keep this static for ease of use since we're never going to have two terminals
    private static TermSession mSession;
    private static PipedOutputStream mOS;

    private SwitcherEventCollector mSwitcherEC;

    public boolean mIsRunning = true;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        FragmentManager fm = getFragmentManager();
        mSwitcherEC = (SwitcherEventCollector) fm.findFragmentByTag(SwitcherEventCollector.TAG);

        if (mSwitcherEC == null) {
            mSwitcherEC = new SwitcherEventCollector();
            fm.beginTransaction().add(mSwitcherEC, SwitcherEventCollector.TAG).commit();
        }

        // Create terminal
        if (mSession == null) {
            mSession = new TermSession();
            // We don't care about any input because this is kind of a "dumb" terminal output, not
            // a proper interactive one
            mSession.setTermOut(new NullOutputStream());

            mOS = new PipedOutputStream();
            try {
                mSession.setTermIn(new PipedInputStream(mOS));
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_zip_flashing_output, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        EmulatorView emulatorView = (EmulatorView) getActivity().findViewById(R.id.terminal);
        emulatorView.attachSession(mSession);
        DisplayMetrics metrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
        emulatorView.setDensity(metrics);

        if (savedInstanceState == null) {
            Parcelable[] parcelables = getArguments().getParcelableArray(PARAM_PENDING_ACTIONS);
            PendingAction[] actions = new PendingAction[parcelables.length];
            System.arraycopy(parcelables, 0, actions, 0, parcelables.length);

            // Set mSwitcherEC's Context since it grabs it in onAttach(), which may not have run yet
            mSwitcherEC.setApplicationContext(getActivity());
            mSwitcherEC.flashZips(actions);
        } else {
            mIsRunning = savedInstanceState.getBoolean(EXTRA_IS_RUNNING);
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(EXTRA_IS_RUNNING, mIsRunning);
    }

    @Override
    public void onResume() {
        super.onResume();
        mSwitcherEC.attachListener(TAG, this);
    }

    @Override
    public void onPause() {
        super.onPause();
        mSwitcherEC.detachListener(TAG);
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof NewOutputEvent) {
            NewOutputEvent nole = (NewOutputEvent) event;
            try {
                String crlf  = nole.data.replace("\n", "\r\n");
                mOS.write(crlf.getBytes(Charset.forName("UTF-8")));
            } catch (IOException e) {
                e.printStackTrace();
            }
        } else if (event instanceof FlashedZipsEvent) {
            mIsRunning = false;
        }
    }

    // Should be called by parent activity when it exits
    public void onCleanup() {
        // Destroy session
        mSession.finish();
        mSession = null;
    }

    public boolean isRunning() {
        return mIsRunning;
    }
}
