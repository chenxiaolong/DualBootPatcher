/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.app.AppCompatActivity;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SwitchRomResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmAutomatedSwitchRomDialog
        .ConfirmAutomatedSwitchRomDialogListener;

import java.util.ArrayList;

public class AutomatedSwitcherActivity extends AppCompatActivity implements
        ConfirmAutomatedSwitchRomDialogListener, ServiceConnection {
    private static final String PREF_SHOW_CONFIRM_DIALOG = "show_confirm_automated_switcher_dialog";
    private static final String PREF_ALLOW_3RD_PARTY_INTENTS = "allow_3rd_party_intents";

    private static final String EXTRA_TASK_ID_SWITCH_ROM = "task_id_switch_rom";

    /** Intent filter for messages we care about from the service */
    private static final IntentFilter SERVICE_INTENT_FILTER = new IntentFilter();

    public static final String EXTRA_ROM_ID = "rom_id";
    public static final String EXTRA_REBOOT = "reboot";

    public static final String RESULT_CODE = "code";
    public static final String RESULT_MESSAGE = "message";

    private SharedPreferences mPrefs;

    private boolean mInitialRun = false;

    private int mTaskIdSwitchRom = -1;

    /** Task IDs to remove */
    private ArrayList<Integer> mTaskIdsToRemove = new ArrayList<>();

    /** Switcher service */
    private SwitcherService mService;
    /** Broadcast receiver for events from the service */
    private SwitcherEventReceiver mReceiver = new SwitcherEventReceiver();

    static {
        SERVICE_INTENT_FILTER.addAction(SwitcherService.ACTION_SWITCHED_ROM);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.automated_switcher_layout);

        mPrefs = getSharedPreferences("settings", 0);

        if (!mPrefs.getBoolean(PREF_ALLOW_3RD_PARTY_INTENTS, false)) {
            Toast.makeText(this, R.string.third_party_intents_not_allowed, Toast.LENGTH_LONG).show();
            finish();
            return;
        }

        Bundle args = getIntent().getExtras();
        if (args == null || getIntent().getStringExtra(EXTRA_ROM_ID) == null) {
            finish();
            return;
        }

        if (savedInstanceState == null) {
            mInitialRun = true;
        } else {
            mTaskIdSwitchRom = savedInstanceState.getInt(EXTRA_TASK_ID_SWITCH_ROM);
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putInt(EXTRA_TASK_ID_SWITCH_ROM, mTaskIdSwitchRom);
    }

    @Override
    public void onStart() {
        super.onStart();

        // Start and bind to the service
        Intent intent = new Intent(this, SwitcherService.class);
        bindService(intent, this, BIND_AUTO_CREATE);
        startService(intent);

        // Register our broadcast receiver for our service
        LocalBroadcastManager.getInstance(this).registerReceiver(
                mReceiver, SERVICE_INTENT_FILTER);
    }

    @Override
    public void onStop() {
        super.onStop();

        // Unbind from our service
        unbindService(this);
        mService = null;

        // Unregister the broadcast receiver
        LocalBroadcastManager.getInstance(this).unregisterReceiver(mReceiver);
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

        if (mInitialRun) {
            boolean shouldShow = mPrefs.getBoolean(PREF_SHOW_CONFIRM_DIALOG, true);
            if (shouldShow) {
                ConfirmAutomatedSwitchRomDialog d = ConfirmAutomatedSwitchRomDialog.newInstance(
                        getIntent().getStringExtra(EXTRA_ROM_ID));
                d.show(getFragmentManager(), ConfirmAutomatedSwitchRomDialog.TAG);
            } else {
                switchRom();
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

    private void onSwitchedRom(String romId, SwitchRomResult result) {
        removeCachedTaskId(mTaskIdSwitchRom);
        mTaskIdSwitchRom = -1;

        boolean reboot = getIntent().getBooleanExtra(EXTRA_REBOOT, false);
        if (result == SwitchRomResult.SUCCEEDED && reboot) {
            // Don't return if we're rebooting
            SwitcherUtils.reboot(this);
            return;
        }

        GenericProgressDialog d = (GenericProgressDialog)
                getFragmentManager().findFragmentByTag("automated_switch_rom_waiting");
        if (d != null) {
            d.dismiss();
        }

        Intent intent = new Intent();

        switch (result) {
        case SUCCEEDED:
            intent.putExtra(RESULT_CODE, "SWITCHING_SUCCEEDED");
            Toast.makeText(this, R.string.choose_rom_success, Toast.LENGTH_LONG).show();
            break;
        case FAILED:
            intent.putExtra(RESULT_CODE, "SWITCHING_FAILED");
            intent.putExtra(RESULT_MESSAGE,
                    String.format("Failed to switch to %s", romId));
            Toast.makeText(this, R.string.choose_rom_failure, Toast.LENGTH_LONG).show();
            break;
        case CHECKSUM_INVALID:
            intent.putExtra(RESULT_CODE, "SWITCHING_FAILED");
            intent.putExtra(RESULT_MESSAGE,
                    String.format("Mismatched checksums for %s's images", romId));
            Toast.makeText(this, R.string.choose_rom_checksums_invalid, Toast.LENGTH_LONG).show();
            break;
        case CHECKSUM_NOT_FOUND:
            intent.putExtra(RESULT_CODE, "SWITCHING_FAILED");
            intent.putExtra(RESULT_MESSAGE,
                    String.format("Missing checksums for %s's images", romId));
            Toast.makeText(this, R.string.choose_rom_checksums_missing, Toast.LENGTH_LONG).show();
            break;
        case UNKNOWN_BOOT_PARTITION:
            intent.putExtra(RESULT_CODE, "UNKNOWN_BOOT_PARTITION");
            intent.putExtra(RESULT_MESSAGE, "Failed to determine boot partition");
            String codename = RomUtils.getDeviceCodename(this);
            Toast.makeText(this, getString(R.string.unknown_boot_partition, codename),
                    Toast.LENGTH_SHORT).show();
            break;
        }

        setResult(RESULT_OK, intent);
        finish();
    }

    @Override
    public void onConfirmSwitchRom(boolean dontShowAgain) {
        Editor e = mPrefs.edit();
        e.putBoolean(PREF_SHOW_CONFIRM_DIALOG, !dontShowAgain);
        e.apply();

        switchRom();
    }

    @Override
    public void onCancelSwitchRom() {
        finish();
    }

    private void switchRom() {
        GenericProgressDialog d = GenericProgressDialog.newInstance(
                R.string.switching_rom, R.string.please_wait);
        d.show(getFragmentManager(), "automated_switch_rom_waiting");

        mTaskIdSwitchRom = mService.switchRom(getIntent().getStringExtra(EXTRA_ROM_ID), false);
    }

    private class SwitcherEventReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();

            // If we're not yet connected to the service, don't do anything. The state will be
            // restored upon connection.
            if (mService == null) {
                return;
            }

            if (!SERVICE_INTENT_FILTER.hasAction(action)) {
                return;
            }

            int taskId = intent.getIntExtra(SwitcherService.RESULT_TASK_ID, -1);

            if (SwitcherService.ACTION_SWITCHED_ROM.equals(action)
                    && taskId == mTaskIdSwitchRom) {
                String romId = intent.getStringExtra(
                        SwitcherService.RESULT_SWITCHED_ROM_ROM_ID);
                SwitchRomResult result = (SwitchRomResult) intent.getSerializableExtra(
                        SwitcherService.RESULT_SWITCHED_ROM_RESULT);
                onSwitchedRom(romId, result);
            }
        }
    }
}
