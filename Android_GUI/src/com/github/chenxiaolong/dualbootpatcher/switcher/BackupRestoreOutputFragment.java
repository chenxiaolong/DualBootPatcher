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
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolErrorActivity;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BackupRestoreRomTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BackupRestoreRomTask
        .BackupRestoreRomTaskListener;

import org.apache.commons.io.IOUtils;
import org.apache.commons.io.output.NullOutputStream;

import java.io.IOException;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.nio.charset.Charset;
import java.util.ArrayList;

import jackpal.androidterm.emulatorview.EmulatorView;
import jackpal.androidterm.emulatorview.TermSession;

public class BackupRestoreOutputFragment extends Fragment implements ServiceConnection {
    public static final String TAG = BackupRestoreOutputFragment.class.getSimpleName();

    private static final String EXTRA_IS_RUNNING = TAG + ".is_running";
    private static final String EXTRA_TASK_ID_BACKUP_RESTORE = TAG + ".task_id_backup_restore";

    public static final String PARAM_ACTION = "action";
    public static final String PARAM_ROM_ID = "rom_id";
    public static final String PARAM_TARGETS = "targets";
    public static final String PARAM_NAME = "name";
    public static final String PARAM_BACKUP_DIR = "backup_dir";
    public static final String PARAM_FORCE = "force";

    private EmulatorView mEmulatorView;
    private TermSession mSession;
    private PipedOutputStream mOS;

    public boolean mIsRunning = true;

    private int mTaskIdBackupRestore = -1;

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
            mTaskIdBackupRestore = savedInstanceState.getInt(EXTRA_TASK_ID_BACKUP_RESTORE);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_backup_restore_output, container, false);
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

        // Set activity title depending on the action
        String action = getArguments().getString(PARAM_ACTION);
        if (BackupRestoreRomTask.ACTION_BACKUP_ROM.equals(action)) {
            getActivity().setTitle(R.string.br_backup_title);
        } else if (BackupRestoreRomTask.ACTION_RESTORE_ROM.equals(action)) {
            getActivity().setTitle(R.string.br_restore_title);
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putBoolean(EXTRA_IS_RUNNING, mIsRunning);
        outState.putInt(EXTRA_TASK_ID_BACKUP_RESTORE, mTaskIdBackupRestore);
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
            if (mTaskIdBackupRestore >= 0) {
                removeCachedTaskId(mTaskIdBackupRestore);
                mTaskIdBackupRestore = -1;
            }
        }

        // If we connected to the service and registered the callback, now we unregister it
        if (mService != null) {
            if (mTaskIdBackupRestore >= 0) {
                mService.removeCallback(mTaskIdBackupRestore, mCallback);
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

        if (mTaskIdBackupRestore < 0) {
            Bundle args = getArguments();
            String action = args.getString(PARAM_ACTION);
            String romId = args.getString(PARAM_ROM_ID);
            String[] targets = args.getStringArray(PARAM_TARGETS);
            String backupName = args.getString(PARAM_NAME);
            String backupDir = args.getString(PARAM_BACKUP_DIR);
            boolean force = args.getBoolean(PARAM_FORCE);

            mTaskIdBackupRestore = mService.backupRestoreAction(
                    action, romId, targets, backupName, backupDir, force);
            mService.addCallback(mTaskIdBackupRestore, mCallback);
            mService.enqueueTaskId(mTaskIdBackupRestore);
        } else {
            mService.addCallback(mTaskIdBackupRestore, mCallback);
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

    private void onFinishedTask() {
        mIsRunning = false;
    }

    public boolean isRunning() {
        return mIsRunning;
    }

    private class SwitcherEventCallback implements BackupRestoreRomTaskListener {
        @Override
        public void onBackupRestoreFinished(int taskId, String action, boolean success) {
            if (taskId == mTaskIdBackupRestore) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onFinishedTask();
                    }
                });
            }
        }

        @Override
        public void onCommandOutput(int taskId, final String line) {
            if (taskId == mTaskIdBackupRestore) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        onNewOutputLine(line);
                    }
                });
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
