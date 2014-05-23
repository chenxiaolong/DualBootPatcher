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

import android.app.IntentService;
import android.content.Intent;

public class SwitcherService extends IntentService {
    private static final String TAG = "SwitcherService";
    public static final String BROADCAST_INTENT = "com.chenxiaolong.github.dualbootpatcher.BROADCAST_SWITCHER_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_CHOOSE_ROM = "choose_rom";
    public static final String ACTION_SET_KERNEL = "set_kernel";

    public static final String STATE = "state";
    public static final String STATE_CHOSE_ROM = "chose_rom";
    public static final String STATE_SET_KERNEL = "set_kernel";

    public SwitcherService() {
        super(TAG);
    }

    private void onChoseRom(boolean failed, String message, String rom) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_CHOSE_ROM);
        i.putExtra("failed", failed);
        i.putExtra("message", message);
        i.putExtra("rom", rom);
        sendBroadcast(i);
    }

    private void onSetKernel(boolean failed, String message, String rom) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SET_KERNEL);
        i.putExtra("failed", failed);
        i.putExtra("message", message);
        i.putExtra("rom", rom);
        sendBroadcast(i);
    }

    private void chooseRom(String rom) {
        try {
            RestoreKernel task = new RestoreKernel(rom);
            task.start();
            task.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void setKernel(String rom) {
        try {
            BackupKernel task = new BackupKernel(rom);
            task.start();
            task.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_CHOOSE_ROM.equals(action)) {
            String rom = intent.getStringExtra("rom");
            chooseRom(rom);
        } else if (ACTION_SET_KERNEL.equals(action)) {
            String rom = intent.getStringExtra("rom");
            setKernel(rom);
        }
    }

    private class RestoreKernel extends Thread {
        private final String mRom;

        public RestoreKernel(String rom) {
            mRom = rom;
        }

        @Override
        public void run() {
            boolean failed = true;
            String message = "";

            try {
                SwitcherUtils.writeKernel(mRom);
                failed = false;
            } catch (Exception e) {
                message = e.getMessage();
                e.printStackTrace();
            }

            onChoseRom(failed, message, mRom);
        }
    }

    private class BackupKernel extends Thread {
        private final String mRom;

        public BackupKernel(String rom) {
            mRom = rom;
        }

        @Override
        public void run() {
            boolean failed = true;
            String message = "";

            try {
                SwitcherUtils.backupKernel(mRom);
                failed = false;
            } catch (Exception e) {
                message = e.getMessage();
                e.printStackTrace();
            }

            onSetKernel(failed, message, mRom);
        }
    }
}
