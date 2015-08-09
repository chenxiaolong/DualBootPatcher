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
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;

import com.github.chenxiaolong.dualbootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherEventCollector.SwitchedRomEvent;

public class AutomatedSwitcherActivity extends AppCompatActivity implements EventCollectorListener {
    private static final String TAG = AutomatedSwitcherActivity.class.getSimpleName();

    public static final String EXTRA_ROM_ID = "rom_id";
    public static final String EXTRA_REBOOT = "reboot";

    public static final String RESULT_CODE = "code";
    public static final String RESULT_MESSAGE = "message";

    private SwitcherEventCollector mEventCollector;

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

        if (savedInstanceState == null) {
            GenericProgressDialog d = GenericProgressDialog.newInstance(
                    R.string.switching_rom, R.string.please_wait);
            d.show(getFragmentManager(), "automated_switch_rom_waiting");

            mEventCollector.setApplicationContext(getApplicationContext());
            mEventCollector.chooseRom(getIntent().getStringExtra(EXTRA_ROM_ID));
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
                break;
            case FAILED:
                intent.putExtra(RESULT_CODE, "SWITCHING_FAILED");
                intent.putExtra(RESULT_MESSAGE,
                        String.format("Failed to switch to %s", event.kernelId));
                break;
            case UNKNOWN_BOOT_PARTITION:
                intent.putExtra(RESULT_CODE, "UNKNOWN_BOOT_PARTITION");
                intent.putExtra(RESULT_MESSAGE, "Failed to determine boot partition");
                break;
            }

            setResult(Activity.RESULT_OK, intent);
            finish();
        }
    }
}
