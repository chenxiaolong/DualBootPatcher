/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.socket;

import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v7.app.AppCompatActivity;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
        .GenericConfirmDialogListener;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog
        .GenericYesNoDialogListener;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherService;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.UpdateMbtoolWithRootTask
        .UpdateMbtoolWithRootTaskListener;

import java.util.ArrayList;

public class MbtoolErrorActivity extends AppCompatActivity implements ServiceConnection,
        GenericYesNoDialogListener, GenericConfirmDialogListener {
    private static final String DIALOG_TAG =
            MbtoolErrorActivity.class.getCanonicalName() + ".dialog";

    // Argument extras
    public static final String EXTRA_REASON = "reason";
    // Saved state extras
    private static final String EXTRA_STATE_TASK_ID_UPDATE_MBTOOL = "state.update_mbtool";
    private static final String EXTRA_STATE_TASK_IDS_TO_REMOVE = "state.task_ids_to_remove";
    // Result extras
    public static final String EXTRA_RESULT_CAN_RETRY = "result.can_retry";

    private int mTaskIdUpdateMbtool = -1;

    /** Task IDs to remove */
    private ArrayList<Integer> mTaskIdsToRemove = new ArrayList<>();

    /** Switcher service */
    private SwitcherService mService;
    /** Callback for events from the service */
    private final SwitcherEventCallback mCallback = new SwitcherEventCallback();

    /** Handler for processing events from the service on the UI thread */
    private final Handler mHandler = new Handler(Looper.getMainLooper());

    /** mbtool connection failure reason */
    private Reason mReason;

    private boolean mIsInitialRun;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_mbtool_error);

        Intent intent = getIntent();
        mReason = (Reason) intent.getSerializableExtra(EXTRA_REASON);
        if (mReason == Reason.PROTOCOL_ERROR) {
            // This is a relatively "standard" error. Not really one that the user needs to know
            // about...
            finish();
            return;
        }

        mIsInitialRun = savedInstanceState == null;

        if (savedInstanceState != null) {
            mTaskIdUpdateMbtool = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_UPDATE_MBTOOL);
            mTaskIdsToRemove = savedInstanceState.getIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE);
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt(EXTRA_STATE_TASK_ID_UPDATE_MBTOOL, mTaskIdUpdateMbtool);
        outState.putIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE, mTaskIdsToRemove);
    }

    @Override
    protected void onStart() {
        super.onStart();

        // Start and bind to the service
        Intent intent = new Intent(this, SwitcherService.class);
        bindService(intent, this, BIND_AUTO_CREATE);
        startService(intent);
    }

    @Override
    protected void onStop() {
        super.onStop();

        // If we connected to the service and registered the callback, now we unregister it
        if (mService != null) {
            if (mTaskIdUpdateMbtool >= 0) {
                mService.removeCallback(mTaskIdUpdateMbtool, mCallback);
            }
        }

        // Unbind from our service
        unbindService(this);
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

        if (mTaskIdUpdateMbtool >= 0) {
            mService.addCallback(mTaskIdUpdateMbtool, mCallback);
        }

        onReady();
    }

    @Override
    public void onServiceDisconnected(ComponentName name) {
        mService = null;
    }

    @Override
    public void onBackPressed() {
        // Ignore
    }

    private void removeCachedTaskId(int taskId) {
        if (mService != null) {
            mService.removeCachedTask(taskId);
        } else {
            mTaskIdsToRemove.add(taskId);
        }
    }

    private void onReady() {
        if (!mIsInitialRun) {
            return;
        }

        DialogFragment dialog;

        switch (mReason) {
        case DAEMON_NOT_RUNNING: {
            GenericProgressDialog.Builder builder = new GenericProgressDialog.Builder();
            builder.message(R.string.mbtool_dialog_starting_mbtool);
            dialog = builder.build();
            startMbtoolUpdate();
            break;
        }
        case SIGNATURE_CHECK_FAIL: {
            GenericYesNoDialog.Builder builder = new GenericYesNoDialog.Builder();
            builder.message(R.string.mbtool_dialog_signature_check_fail);
            builder.positive(R.string.proceed);
            builder.negative(R.string.cancel);
            dialog = builder.buildFromActivity(DIALOG_TAG);
            break;
        }
        case INTERFACE_NOT_SUPPORTED:
        case VERSION_TOO_OLD:
            GenericYesNoDialog.Builder builder = new GenericYesNoDialog.Builder();
            builder.message(R.string.mbtool_dialog_version_too_old);
            builder.positive(R.string.proceed);
            builder.negative(R.string.cancel);
            dialog = builder.buildFromActivity(DIALOG_TAG);
            break;
        default:
            throw new IllegalStateException("Invalid mbtool error reason: " + mReason.name());
        }

        dialog.show(getSupportFragmentManager(), DIALOG_TAG);
    }

    private void onMbtoolUpdateFinished(boolean success) {
        removeCachedTaskId(mTaskIdUpdateMbtool);
        mTaskIdUpdateMbtool = -1;

        // Dismiss progress dialog
        DialogFragment dialog =
                (DialogFragment) getSupportFragmentManager().findFragmentByTag(DIALOG_TAG);
        if (dialog != null) {
            dialog.dismiss();
        }

        if (!success) {
            GenericConfirmDialog.Builder builder = new GenericConfirmDialog.Builder();
            builder.message(R.string.mbtool_dialog_update_failed);
            builder.buttonText(R.string.ok);
            dialog = builder.buildFromActivity(DIALOG_TAG);
            dialog.show(getSupportFragmentManager(), DIALOG_TAG);
        } else {
            exit(true);
        }
    }

    private void exit(boolean success) {
        Intent intent = new Intent();
        intent.putExtra(EXTRA_RESULT_CAN_RETRY, success);
        setResult(RESULT_OK, intent);

        finish();
    }

    @Override
    public void onConfirmYesNo(@Nullable String tag, boolean choice) {
        if (DIALOG_TAG.equals(tag) && choice) {
            GenericProgressDialog.Builder builder = new GenericProgressDialog.Builder();
            builder.message(R.string.mbtool_dialog_updating_mbtool);
            builder.build().show(getSupportFragmentManager(), DIALOG_TAG);

            startMbtoolUpdate();
        } else {
            finish();
        }
    }

    @Override
    public void onConfirmOk(@Nullable String tag) {
        if (DIALOG_TAG.equals(tag)) {
            exit(false);
        } else {
            finish();
        }
    }

    private void startMbtoolUpdate() {
        if (mTaskIdUpdateMbtool < 0) {
            mTaskIdUpdateMbtool = mService.updateMbtoolWithRoot();
            mService.addCallback(mTaskIdUpdateMbtool, mCallback);
            mService.enqueueTaskId(mTaskIdUpdateMbtool);
        }
    }

    private class SwitcherEventCallback implements UpdateMbtoolWithRootTaskListener {
        @Override
        public void onMbtoolUpdateSucceeded(int taskId) {
            if (taskId == mTaskIdUpdateMbtool) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        MbtoolErrorActivity.this.onMbtoolUpdateFinished(true);
                    }
                });
            }
        }

        @Override
        public void onMbtoolUpdateFailed(int taskId) {
            if (taskId == mTaskIdUpdateMbtool) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        MbtoolErrorActivity.this.onMbtoolUpdateFinished(false);
                    }
                });
            }
        }
    }
}
