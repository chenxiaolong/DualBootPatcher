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

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;

public class SwitcherService extends IntentService {
    private static final String TAG = SwitcherService.class.getSimpleName();
    public static final String BROADCAST_INTENT = "com.chenxiaolong.github.dualbootpatcher.BROADCAST_SWITCHER_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_CHOOSE_ROM = "choose_rom";
    public static final String ACTION_SET_KERNEL = "set_kernel";

    public static final String PARAM_KERNEL_ID = "kernel_id";

    public static final String STATE = "state";
    public static final String STATE_CHOSE_ROM = "chose_rom";
    public static final String STATE_SET_KERNEL = "set_kernel";

    public static final String RESULT_FAILED = "failed";
    public static final String RESULT_KERNEL_ID = "kernel_id";

    public SwitcherService() {
        super(TAG);
    }

    private void onChoseRom(boolean failed, String kernelId) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_CHOSE_ROM);
        i.putExtra(RESULT_FAILED, failed);
        i.putExtra(RESULT_KERNEL_ID, kernelId);
        sendBroadcast(i);
    }

    private void onSetKernel(boolean failed, String kernelId) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SET_KERNEL);
        i.putExtra(RESULT_FAILED, failed);
        i.putExtra(RESULT_KERNEL_ID, kernelId);
        sendBroadcast(i);
    }

    private void setupNotification(String action) {
        Intent resultIntent = new Intent(this, MainActivity.class);
        resultIntent.addCategory(Intent.CATEGORY_LAUNCHER);
        resultIntent.setAction(Intent.ACTION_MAIN);

        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, resultIntent, 0);

        Notification.Builder builder = new Notification.Builder(this);
        builder.setSmallIcon(R.drawable.ic_launcher);
        builder.setOngoing(true);

        if (ACTION_CHOOSE_ROM.equals(action)) {
            builder.setContentTitle(getString(R.string.switching_rom));
        } else if (ACTION_SET_KERNEL.equals(action)) {
            builder.setContentTitle(getString(R.string.setting_kernel));
        }

        builder.setContentIntent(pendingIntent);
        builder.setProgress(0, 0, true);

        NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        nm.notify(1, builder.build());
    }

    private void removeNotification() {
        NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        nm.cancel(1);
    }

    private void chooseRom(Bundle data) {
        setupNotification(ACTION_CHOOSE_ROM);

        String kernelId = data.getString(PARAM_KERNEL_ID);
        boolean failed = !SwitcherUtils.chooseRom(kernelId);

        onChoseRom(failed, kernelId);

        removeNotification();
    }

    private void setKernel(Bundle data) {
        setupNotification(ACTION_SET_KERNEL);

        String kernelId = data.getString(PARAM_KERNEL_ID);
        boolean failed = !SwitcherUtils.setKernel(kernelId);

        onSetKernel(failed, kernelId);

        removeNotification();
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_CHOOSE_ROM.equals(action)) {
            chooseRom(intent.getExtras());
        } else if (ACTION_SET_KERNEL.equals(action)) {
            setKernel(intent.getExtras());
        }
    }
}
