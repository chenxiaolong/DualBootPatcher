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
import android.os.Parcelable;

import com.github.chenxiaolong.dualbootpatcher.AnsiStuff;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Attribute;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Color;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandListener;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandRunner;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SwitchRomResult;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.WipeResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction;

import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.util.EnumSet;

public class SwitcherService extends IntentService {
    private static final String TAG = SwitcherService.class.getSimpleName();
    public static final String BROADCAST_INTENT =
            "com.chenxiaolong.github.dualbootpatcher.BROADCAST_SWITCHER_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_FLASH_ZIPS = "flash_zips";

    public static final String PARAM_ZIP_FILE = "zip_file";
    public static final String PARAM_PENDING_ACTIONS = "pending_actions";

    public static final String STATE = "state";
    public static final String STATE_FLASHED_ZIPS = "flashed_zips";
    public static final String STATE_COMMAND_OUTPUT_DATA = "command_output_line";

    public static final String RESULT_TOTAL_ACTIONS = "total_actions";
    public static final String RESULT_FAILED_ACTIONS = "failed_actions";
    public static final String RESULT_OUTPUT_DATA = "output_line";

    // Switch ROM and set kernel
    public static final String ACTION_SWITCH_ROM = "switch_rom";
    public static final String ACTION_SET_KERNEL = "set_kernel";
    public static final String PARAM_KERNEL_ID = "kernel_id";
    public static final String STATE_SWITCHED_ROM = "switched_rom";
    public static final String STATE_SET_KERNEL = "set_kernel";
    public static final String RESULT_KERNEL_ID = "kernel_id";
    public static final String RESULT_SWITCH_ROM = "switch_rom";
    public static final String RESULT_SET_KERNEL = "set_kernel";

    // Wipe ROM
    public static final String ACTION_WIPE_ROM = "wipe_rom";
    public static final String PARAM_ROM_ID = "rom_id";
    public static final String PARAM_WIPE_TARGETS = "wipe_targets";
    public static final String STATE_WIPED_ROM = "wiped_rom";
    public static final String RESULT_TARGETS_SUCCEEDED = "targets_succeeded";
    public static final String RESULT_TARGETS_FAILED = "targets_failed";

    // Verify ZIP for in-app flashing
    public static final String ACTION_VERIFY_ZIP = "verify_zip";
    public static final String STATE_VERIFIED_ZIP = "verified_zip";
    public static final String RESULT_VERIFY_ZIP = "verify_zip";
    public static final String RESULT_ROM_ID = "rom_id";

    private static final String UPDATE_BINARY = "META-INF/com/google/android/update-binary";

    public SwitcherService() {
        super(TAG);
    }

    private void onSwitchedRom(String kernelId, SwitchRomResult result) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SWITCHED_ROM);
        i.putExtra(RESULT_KERNEL_ID, kernelId);
        i.putExtra(RESULT_SWITCH_ROM, result);
        sendBroadcast(i);
    }

    private void onSetKernel(String kernelId, SetKernelResult result) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SET_KERNEL);
        i.putExtra(RESULT_KERNEL_ID, kernelId);
        i.putExtra(RESULT_SET_KERNEL, result);
        sendBroadcast(i);
    }

    private void onVerifiedZip(VerificationResult result, String romId) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_VERIFIED_ZIP);
        i.putExtra(RESULT_VERIFY_ZIP, result);
        i.putExtra(RESULT_ROM_ID, romId);
        sendBroadcast(i);
    }

    private void onFlashedZips(int totalActions, int failedActions) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_FLASHED_ZIPS);
        i.putExtra(RESULT_TOTAL_ACTIONS, totalActions);
        i.putExtra(RESULT_FAILED_ACTIONS, failedActions);
        sendBroadcast(i);
    }

    private void onNewOutputData(String line) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_COMMAND_OUTPUT_DATA);
        i.putExtra(RESULT_OUTPUT_DATA, line);
        sendBroadcast(i);
    }

    private void onWipedRom(short[] succeeded, short[] failed) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_WIPED_ROM);
        i.putExtra(RESULT_TARGETS_SUCCEEDED, succeeded);
        i.putExtra(RESULT_TARGETS_FAILED, failed);
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

        if (ACTION_SWITCH_ROM.equals(action)) {
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

    private void switchRom(Bundle data) {
        setupNotification(ACTION_SWITCH_ROM);

        String kernelId = data.getString(PARAM_KERNEL_ID);
        SwitchRomResult result = MbtoolSocket.getInstance().chooseRom(this, kernelId);

        onSwitchedRom(kernelId, result);

        removeNotification();
    }

    private void setKernel(Bundle data) {
        setupNotification(ACTION_SET_KERNEL);

        String kernelId = data.getString(PARAM_KERNEL_ID);
        SetKernelResult result = MbtoolSocket.getInstance().setKernel(this, kernelId);

        onSetKernel(kernelId, result);

        removeNotification();
    }

    private void verifyZip(Bundle data) {
        String zipFile = data.getString(PARAM_ZIP_FILE);
        VerificationResult result = SwitcherUtils.verifyZipMbtoolVersion(zipFile);
        String romId = SwitcherUtils.getTargetInstallLocation(zipFile);
        onVerifiedZip(result, romId);
    }

    private int runRootCommand(String command) {
        printBoldText(Color.YELLOW, "Running command: " + command + "\n");

        RootCommandParams params = new RootCommandParams();
        params.command = command;
        params.listener = new RootCommandListener() {
            @Override
            public void onNewOutputLine(String line) {
                SwitcherService.this.onNewOutputData(line + "\n");
            }

            @Override
            public void onCommandCompletion(CommandResult result) {
            }
        };

        RootCommandRunner cmd = new RootCommandRunner(params);
        cmd.start();
        CommandUtils.waitForRootCommand(cmd);
        CommandResult result = cmd.getResult();

        if (result == null) {
            return -1;
        } else {
            return result.exitCode;
        }
    }

    private boolean remountFs(String mountpoint, boolean rw) {
        printBoldText(Color.YELLOW, "Mounting " + mountpoint + " as " +
                (rw ? "writable" : "read-only") + "\n");

        if (rw) {
            return runRootCommand("mount -o remount,rw " + mountpoint) == 0;
        } else {
            return runRootCommand("mount -o remount,ro " + mountpoint) == 0;
        }
    }

    private void printSeparator() {
        printBoldText(Color.WHITE, StringUtils.repeat('-', 16) + "\n");
    }

    private void printBoldText(Color color, String text) {
        onNewOutputData(AnsiStuff.format(text,
                EnumSet.of(color), null, EnumSet.of(Attribute.BOLD)));
    }

    private static String quoteArg(String arg) {
        return "'" + arg.replace("'", "'\"'\"'") + "'";
    }

    private void flashZips(Bundle data) {
        Parcelable[] parcelables = data.getParcelableArray(PARAM_PENDING_ACTIONS);
        PendingAction[] actions = new PendingAction[parcelables.length];
        System.arraycopy(parcelables, 0, actions, 0, parcelables.length);

        boolean remountedRoot = false;
        boolean remountedSystem = false;

        int succeeded = 0;

        try {
            if (!CommandUtils.requestRootAccess()) {
                printBoldText(Color.RED, "Failed to obtain root privileges\n");
                return;
            }

            if (!remountFs("/", true)) {
                printBoldText(Color.RED, "Failed to remount / as rw\n");
                return;
            }
            remountedRoot = true;

            if (!remountFs("/system", true)) {
                printBoldText(Color.RED, "Failed to remount /system as rw\n");
                return;
            }
            remountedSystem = true;

            for (PendingAction pa : actions) {
                if (pa.type != PendingAction.Type.INSTALL_ZIP) {
                    throw new IllegalStateException("Only INSTALL_ZIP is supported right now");
                }

                printSeparator();

                printBoldText(Color.MAGENTA, "Processing action: Flash zip\n");
                printBoldText(Color.MAGENTA, "- ZIP file: " + pa.zipFile + "\n");
                printBoldText(Color.MAGENTA, "- Destination: " + pa.romId + "\n");

                // Extract mbtool from the zip file
                File zipInstaller = new File(getCacheDir() + File.separator + "rom-installer");
                zipInstaller.delete();

                printBoldText(Color.YELLOW, "Extracting mbtool ROM installer from the zip file\n");
                if (!FileUtils.zipExtractFile(pa.zipFile, UPDATE_BINARY, zipInstaller.getPath())) {
                    printBoldText(Color.RED, "Failed to extract update-binary\n");
                    return;
                }

                // Copy to /
                if (runRootCommand("rm -f /rom-installer") != 0) {
                    printBoldText(Color.RED, "Failed to remove old /rom-installer\n");
                    return;
                }
                if (runRootCommand("cp " + zipInstaller.getPath() + " /rom-installer") != 0) {
                    printBoldText(Color.RED, "Failed to copy new /rom-installer\n");
                    return;
                }
                if (runRootCommand("chmod 755 /rom-installer") != 0) {
                    printBoldText(Color.RED, "Failed to chmod /rom-installer\n");
                    return;
                }

                int ret = runRootCommand("/rom-installer --romid " + quoteArg(pa.romId) + " " +
                        quoteArg(pa.zipFile));

                zipInstaller.delete();

                if (ret < 0) {
                    printBoldText(Color.RED, "\nFailed to run command\n");
                    return;
                } else {
                    printBoldText(ret == 0 ? Color.GREEN : Color.RED,
                            "\nCommand returned: " + ret + "\n");

                    if (ret == 0) {
                        succeeded++;
                    } else {
                        return;
                    }
                }
            }
        } finally {
            if (remountedRoot || remountedSystem) {
                printSeparator();
            }

            if (remountedRoot && !remountFs("/", false)) {
                printBoldText(Color.RED, "Failed to remount / as ro\n");
            }
            if (remountedSystem && !remountFs("/system", false)) {
                printBoldText(Color.RED, "Failed to remount /system as ro\n");
            }

            printSeparator();

            String frac = succeeded + "/" + actions.length;

            printBoldText(Color.CYAN, "Successfully completed " + frac + " actions\n");

            onFlashedZips(actions.length, actions.length - succeeded);
        }
    }

    private void wipeRom(Bundle data) {
        String romId = data.getString(PARAM_ROM_ID);
        short[] targets = data.getShortArray(PARAM_WIPE_TARGETS);

        WipeResult result = MbtoolSocket.getInstance().wipeRom(this, romId, targets);

        if (result == null) {
            onWipedRom(null, null);
        } else {
            onWipedRom(result.succeeded, result.failed);
        }
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_SWITCH_ROM.equals(action)) {
            switchRom(intent.getExtras());
        } else if (ACTION_SET_KERNEL.equals(action)) {
            setKernel(intent.getExtras());
        } else if (ACTION_VERIFY_ZIP.equals(action)) {
            verifyZip(intent.getExtras());
        } else if (ACTION_FLASH_ZIPS.equals(action)) {
            flashZips(intent.getExtras());
        } else if (ACTION_WIPE_ROM.equals(action)) {
            wipeRom(intent.getExtras());
        }
    }
}
