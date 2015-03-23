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

import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction;
import com.github.chenxiaolong.multibootpatcher.EventCollector;

public class SwitcherEventCollector extends EventCollector {
    public static final String TAG = SwitcherEventCollector.class.getSimpleName();

    private Context mContext;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(SwitcherService.STATE);

                if (SwitcherService.STATE_CHOSE_ROM.equals(state)) {
                    boolean failed = bundle.getBoolean(SwitcherService.RESULT_FAILED);
                    String kernelId = bundle.getString(SwitcherService.RESULT_KERNEL_ID);

                    sendEvent(new SwitchedRomEvent(failed, kernelId));
                } else if (SwitcherService.STATE_SET_KERNEL.equals(state)) {
                    boolean failed = bundle.getBoolean(SwitcherService.RESULT_FAILED);
                    String kernelId = bundle.getString(SwitcherService.RESULT_KERNEL_ID);

                    sendEvent(new SetKernelEvent(failed, kernelId));
                } else if (SwitcherService.STATE_VERIFIED_ZIP.equals(state)) {
                    VerificationResult result = (VerificationResult) bundle.getSerializable(
                            SwitcherService.RESULT_VERIFY_ZIP);

                    sendEvent(new VerifiedZipEvent(result));
                } else if (SwitcherService.STATE_FLASHED_ZIPS.equals(state)) {
                    int totalActions = bundle.getInt(SwitcherService.RESULT_TOTAL_ACTIONS);
                    int failedActions = bundle.getInt(SwitcherService.RESULT_FAILED_ACTIONS);

                    sendEvent(new FlashedZipsEvent(totalActions, failedActions));
                } else if (SwitcherService.STATE_COMMAND_OUTPUT_DATA.equals(state)) {
                    String line = bundle.getString(SwitcherService.RESULT_OUTPUT_DATA);

                    sendEvent(new NewOutputEvent(line));
                } else if (SwitcherService.STATE_WIPED_ROM.equals(state)) {
                    short[] succeeded = bundle.getShortArray(
                            SwitcherService.RESULT_TARGETS_SUCCEEDED);
                    short[] failed = bundle.getShortArray(SwitcherService.RESULT_TARGETS_FAILED);

                    sendEvent(new WipedRomEvent(succeeded, failed));
                }
            }
        }
    };

    @Override
    public void onAttach(Activity activity) {
        super.onAttach(activity);
        setApplicationContext(activity);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mContext != null) {
            mContext.unregisterReceiver(mReceiver);
        }
    }

    public void setApplicationContext(Context context) {
        if (mContext == null) {
            mContext = context.getApplicationContext();
            mContext.registerReceiver(mReceiver, new IntentFilter(
                    SwitcherService.BROADCAST_INTENT));
        }
    }

    // Start tasks

    public void chooseRom(String kernelId) {
        Intent intent = new Intent(mContext, SwitcherService.class);
        intent.putExtra(SwitcherService.ACTION, SwitcherService.ACTION_CHOOSE_ROM);
        intent.putExtra(SwitcherService.PARAM_KERNEL_ID, kernelId);
        mContext.startService(intent);
    }

    public void setKernel(String kernelId) {
        Intent intent = new Intent(mContext, SwitcherService.class);
        intent.putExtra(SwitcherService.ACTION, SwitcherService.ACTION_SET_KERNEL);
        intent.putExtra(SwitcherService.PARAM_KERNEL_ID, kernelId);
        mContext.startService(intent);
    }

    public void verifyZip(String zipFile) {
        Intent intent = new Intent(mContext, SwitcherService.class);
        intent.putExtra(SwitcherService.ACTION, SwitcherService.ACTION_VERIFY_ZIP);
        intent.putExtra(SwitcherService.PARAM_ZIP_FILE, zipFile);
        mContext.startService(intent);
    }

    public void flashZips(PendingAction[] actions) {
        Intent intent = new Intent(mContext, SwitcherService.class);
        intent.putExtra(SwitcherService.ACTION, SwitcherService.ACTION_FLASH_ZIPS);
        intent.putExtra(SwitcherService.PARAM_PENDING_ACTIONS, actions);
        mContext.startService(intent);
    }

    public void wipeRom(String romId, short[] targets) {
        Intent intent = new Intent(mContext, SwitcherService.class);
        intent.putExtra(SwitcherService.ACTION, SwitcherService.ACTION_WIPE_ROM);
        intent.putExtra(SwitcherService.PARAM_ROM_ID, romId);
        intent.putExtra(SwitcherService.PARAM_WIPE_TARGETS, targets);
        mContext.startService(intent);
    }

    // Events

    public class SwitchedRomEvent extends BaseEvent {
        boolean failed;
        String kernelId;

        public SwitchedRomEvent(boolean failed, String kernelId) {
            this.failed = failed;
            this.kernelId = kernelId;
        }
    }

    public class SetKernelEvent extends BaseEvent {
        boolean failed;
        String kernelId;

        public SetKernelEvent(boolean failed, String kernelId) {
            this.failed = failed;
            this.kernelId = kernelId;
        }
    }

    public class VerifiedZipEvent extends BaseEvent {
        VerificationResult result;

        public VerifiedZipEvent(VerificationResult result) {
            this.result = result;
        }
    }

    public class FlashedZipsEvent extends BaseEvent {
        int totalActions;
        int failedActions;

        public FlashedZipsEvent(int totalActions, int failedActions) {
            this.totalActions = totalActions;
            this.failedActions = failedActions;
        }
    }

    public class NewOutputEvent extends BaseEvent {
        String data;

        public NewOutputEvent(String data) {
            this.data = data;
        }
    }

    public class WipedRomEvent extends BaseEvent {
        short[] succeeded;
        short[] failed;

        public WipedRomEvent(short[] succeeded, short[] failed) {
            this.succeeded = succeeded;
            this.failed = failed;
        }
    }
}
