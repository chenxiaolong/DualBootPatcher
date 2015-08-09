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

import android.app.Activity;
import android.app.FragmentManager;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmAutomatedSwitchRomDialog
        .ConfirmAutomatedSwitchRomDialogListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.SwitchedRomEvent;

public class AutomatedSwitcherActivity extends AppCompatActivity implements EventCollectorListener,
        ConfirmAutomatedSwitchRomDialogListener {
    private static final String TAG = AutomatedSwitcherActivity.class.getSimpleName();

    private static final String PREF_SHOW_CONFIRM_DIALOG = "show_confirm_automated_switcher_dialog";

    public static final String EXTRA_ROM_ID = "rom_id";
    public static final String EXTRA_REBOOT = "reboot";

    public static final String RESULT_CODE = "code";
    public static final String RESULT_MESSAGE = "message";

    private SwitcherEventCollector mEventCollector;

    private SharedPreferences mPrefs;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.automated_switcher_layout);

        Bundle args = getIntent().getExtras();
        if (args == null || getIntent().getStringExtra(EXTRA_ROM_ID) == null) {
            finish();
            return;
        }

        FragmentManager fm = getFragmentManager();
        mEventCollector = (SwitcherEventCollector) fm.findFragmentByTag(SwitcherEventCollector.TAG);

        if (mEventCollector == null) {
            mEventCollector = new SwitcherEventCollector();
            fm.beginTransaction().add(mEventCollector, SwitcherEventCollector.TAG).commit();
        }

        mPrefs = getSharedPreferences("settings", 0);
        if (savedInstanceState == null) {
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
    public void onEventReceived(BaseEvent bEvent) {
        if (bEvent instanceof SwitchedRomEvent) {
            boolean reboot = getIntent().getBooleanExtra(EXTRA_REBOOT, false);
            if (reboot) {
                // Don't return if we're rebooting
                SwitcherUtils.reboot(this);
                return;
            }

            SwitchedRomEvent event = (SwitchedRomEvent) bEvent;

            GenericProgressDialog d = (GenericProgressDialog)
                    getFragmentManager().findFragmentByTag("automated_switch_rom_waiting");
            if (d != null) {
                d.dismiss();
            }

            Intent intent = new Intent();

            switch (event.result) {
            case SUCCEEDED:
                intent.putExtra(RESULT_CODE, "SWITCHING_SUCCEEDED");
                Toast.makeText(this, R.string.choose_rom_success, Toast.LENGTH_SHORT).show();
                break;
            case FAILED:
                intent.putExtra(RESULT_CODE, "SWITCHING_FAILED");
                intent.putExtra(RESULT_MESSAGE,
                        String.format("Failed to switch to %s", event.kernelId));
                Toast.makeText(this, R.string.choose_rom_failure, Toast.LENGTH_SHORT).show();
                break;
            case UNKNOWN_BOOT_PARTITION:
                intent.putExtra(RESULT_CODE, "UNKNOWN_BOOT_PARTITION");
                intent.putExtra(RESULT_MESSAGE, "Failed to determine boot partition");
                String codename = RomUtils.getDeviceCodename(this);
                Toast.makeText(this, getString(R.string.unknown_boot_partition, codename),
                        Toast.LENGTH_SHORT).show();
                break;
            }

            setResult(Activity.RESULT_OK, intent);
            finish();
        }
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

        mEventCollector.setApplicationContext(getApplicationContext());
        mEventCollector.chooseRom(getIntent().getStringExtra(EXTRA_ROM_ID));
    }
}
