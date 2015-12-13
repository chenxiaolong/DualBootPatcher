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
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Parcelable;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask.TaskState;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.FlashZipsTask.FlashZipsTaskListener;

import org.apache.commons.io.IOUtils;
import org.apache.commons.io.output.NullOutputStream;

import java.io.IOException;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.nio.charset.Charset;
import java.util.ArrayList;

import jackpal.androidterm.emulatorview.EmulatorView;
import jackpal.androidterm.emulatorview.TermSession;

public class ZipFlashingOutputFragment extends Fragment implements ServiceConnection {
    public static final String TAG = ZipFlashingOutputFragment.class.getSimpleName();

    private static final String EXTRA_IS_RUNNING = "is_running";
    private static final String EXTRA_TASK_ID_FLASH_ZIPS = "task_id_flash_zips";

    public static final String PARAM_PENDING_ACTIONS = "pending_actions";

    private EmulatorView mEmulatorView;
    private TermSession mSession;
    private PipedOutputStream mOS;

    public boolean mIsRunning = true;

    private int mTaskIdFlashZips = -1;

    /** Task IDs to remove */
    private ArrayList<Integer> mTaskIdsToRemove = new ArrayList<>();

    /** Switcher service */
    private SwitcherService mService;
    /** Callback for events from the service */
    private final SwitcherEventCallback mCallback = new SwitcherEventCallback();

    /** Handler for processing events from the service on the UI thread */
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (savedInstanceState != null) {
            mTaskIdFlashZips = savedInstanceState.getInt(EXTRA_TASK_ID_FLASH_ZIPS);
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

        mEmulatorView = (EmulatorView) getActivity().findViewById(R.id.terminal);
        DisplayMetrics metrics = new DisplayMetrics();
        getActivity().getWindowManager().getDefaultDisplay().getMetrics(metrics);
        mEmulatorView.setDensity(metrics);

        if (savedInstanceState != null) {
            mIsRunning = savedInstanceState.getBoolean(EXTRA_IS_RUNNING);
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(EXTRA_IS_RUNNING, mIsRunning);
        outState.putInt(EXTRA_TASK_ID_FLASH_ZIPS, mTaskIdFlashZips);
    }

    @Override
    public void onStart() {
        super.onStart();

        // Create terminal
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

        mEmulatorView.attachSession(mSession);

        // Start and bind to the service
        Intent intent = new Intent(getActivity(), SwitcherService.class);
        getActivity().bindService(intent, this, Context.BIND_AUTO_CREATE);
        getActivity().startService(intent);
    }

    @Override
    public void onStop() {
        super.onStop();

        IOUtils.closeQuietly(mOS);

        // Destroy session
        mSession.finish();
        mSession = null;

        if (getActivity().isFinishing()) {
            if (mTaskIdFlashZips >= 0) {
                removeCachedTaskId(mTaskIdFlashZips);
            }
        }

        // If we connected to the service and registered the callback, now we unregister it
        if (mService != null) {
            mService.unregisterCallback(mCallback);
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

        // Register callback
        mService.registerCallback(mCallback);

        // Remove old task IDs
        for (int taskId : mTaskIdsToRemove) {
            mService.removeCachedTask(taskId);
        }
        mTaskIdsToRemove.clear();

        if (mTaskIdFlashZips < 0) {
            Parcelable[] parcelables = getArguments().getParcelableArray(PARAM_PENDING_ACTIONS);
            PendingAction[] actions = new PendingAction[parcelables.length];
            System.arraycopy(parcelables, 0, actions, 0, parcelables.length);
            mTaskIdFlashZips = mService.flashZips(actions);
        } else {
            String[] lines = mService.getResultFlashZipsOutputLines(mTaskIdFlashZips);
            for (String line : lines) {
                onNewOutputLine(line);
            }
            if (mService.getCachedTaskState(mTaskIdFlashZips) == TaskState.FINISHED) {
                onFinishedFlashing();
            }
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

    private void onNewOutputLine(String line) {
        try {
            String crlf  = line.replace("\n", "\r\n");
            mOS.write(crlf.getBytes(Charset.forName("UTF-8")));
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void onFinishedFlashing() {
        mIsRunning = false;
    }

    public boolean isRunning() {
        return mIsRunning;
    }

    private class SwitcherEventCallback implements FlashZipsTaskListener {
        @Override
        public void onFlashedZips(int taskId, int totalActions, int failedActions) {
            if (taskId == mTaskIdFlashZips) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onFinishedFlashing();
                    }
                });
            }
        }

        @Override
        public void onCommandOutput(int taskId, final String line) {
            if (taskId == mTaskIdFlashZips) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onNewOutputLine(line);
                    }
                });
            }
        }
    }
}
