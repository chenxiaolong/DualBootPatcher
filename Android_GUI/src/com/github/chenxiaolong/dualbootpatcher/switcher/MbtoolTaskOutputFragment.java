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
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolErrorActivity;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.MbtoolTask.MbtoolTaskListener;

import org.apache.commons.io.IOUtils;
import org.apache.commons.io.output.NullOutputStream;

import java.io.IOException;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Arrays;

import jackpal.androidterm.emulatorview.EmulatorView;
import jackpal.androidterm.emulatorview.TermSession;

public class MbtoolTaskOutputFragment extends Fragment implements ServiceConnection {
    public static final String FRAGMENT_TAG = MbtoolTaskOutputFragment.class.getCanonicalName();

    private static final String EXTRA_IS_RUNNING = FRAGMENT_TAG + ".is_running";
    private static final String EXTRA_TASK_ID = FRAGMENT_TAG + ".task_id";

    public static final String PARAM_ACTIONS = "actions";

    private MbtoolAction[] mActions;

    private EmulatorView mEmulatorView;
    private TermSession mSession;
    private PipedOutputStream mOS;

    public boolean mIsRunning = true;

    private int mTaskId = -1;

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
            mTaskId = savedInstanceState.getInt(EXTRA_TASK_ID);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_mbtool_tasks_output, container, false);
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

        Parcelable[] parcelables = getArguments().getParcelableArray(PARAM_ACTIONS);
        mActions = Arrays.copyOf(parcelables, parcelables.length, MbtoolAction[].class);

        int titleResId = -1;

        int countBackup = 0;
        int countRestore = 0;
        int countFlash = 0;

        for (MbtoolAction a : mActions) {
            switch (a.getType()) {
            case ROM_INSTALLER:
                countFlash++;
                break;
            case BACKUP_RESTORE:
                switch (a.getBackupRestoreParams().getAction()) {
                case BACKUP:
                    countBackup++;
                    break;
                case RESTORE:
                    countRestore++;
                    break;
                }
                break;
            }
        }

        if (countBackup + countRestore + countFlash > 1) {
            titleResId = R.string.in_app_flashing_title_flash_files;
        } else if (countBackup > 0) {
            titleResId = R.string.in_app_flashing_title_backup_rom;
        } else if (countRestore > 0) {
            titleResId = R.string.in_app_flashing_title_restore_rom;
        } else if (countFlash > 0) {
            titleResId = R.string.in_app_flashing_title_flash_file;
        }

        if (titleResId >= 0) {
            getActivity().setTitle(titleResId);
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(EXTRA_IS_RUNNING, mIsRunning);
        outState.putInt(EXTRA_TASK_ID, mTaskId);
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
            throw new IllegalStateException("Failed to set terminal input stream to pipe", e);
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
            if (mTaskId >= 0) {
                removeCachedTaskId(mTaskId);
                mTaskId = -1;
            }
        }

        // If we connected to the service and registered the callback, now we unregister it
        if (mService != null) {
            if (mTaskId >= 0) {
                mService.removeCallback(mTaskId, mCallback);
            }
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

        // Remove old task IDs
        for (int taskId : mTaskIdsToRemove) {
            mService.removeCachedTask(taskId);
        }
        mTaskIdsToRemove.clear();

        if (mTaskId < 0) {
            mTaskId = mService.mbtoolActions(mActions);
            mService.addCallback(mTaskId, mCallback);
            mService.enqueueTaskId(mTaskId);
        } else {
            // FIXME: Hack so mCallback.onCommandOutput() is not called on the main thread, which
            //        would result in a deadlock
            new Thread("Add service callback for " + this) {
                @Override
                public void run() {
                    mService.addCallback(mTaskId, mCallback);
                }
            }.start();
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

    private void onFinishedTasks() {
        mIsRunning = false;
    }

    public boolean isRunning() {
        return mIsRunning;
    }

    private class SwitcherEventCallback implements MbtoolTaskListener {
        @Override
        public void onMbtoolTasksCompleted(int taskId, MbtoolAction[] actions, int attempted,
                                           int succeeded) {
            if (taskId == mTaskId) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onFinishedTasks();
                    }
                });
            }
        }

        @Override
        public void onCommandOutput(int taskId, final String line) {
            // This must *NOT* be called on the main thread or else it'll result in a dead lock
            ThreadUtils.enforceExecutionOnNonMainThread();

            if (taskId == mTaskId) {
                try {
                    String crlf  = line.replace("\n", "\r\n");
                    mOS.write(crlf.getBytes(Charset.forName("UTF-8")));
                    mOS.flush();
                } catch (IOException e) {
                    // Ignore. This will happen if the pipe is closed (eg. on configuration change)
                }
            }
        }

        @Override
        public void onMbtoolConnectionFailed(int taskId, final Reason reason) {
            mHandler.post(new Runnable() {
                @Override
                public void run() {
                    Intent intent = new Intent(getActivity(), MbtoolErrorActivity.class);
                    intent.putExtra(MbtoolErrorActivity.EXTRA_REASON, reason);
                    startActivity(intent);
                }
            });
        }
    }
}
