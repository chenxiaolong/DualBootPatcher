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

package com.github.chenxiaolong.dualbootpatcher.switcher.service;

import android.content.Context;

import com.github.chenxiaolong.dualbootpatcher.AnsiStuff;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Attribute;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Color;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandListener;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandRunner;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction;

import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.EnumSet;

public final class FlashZipsTask extends BaseServiceTask {
    private static final String UPDATE_BINARY = "META-INF/com/google/android/update-binary";

    private final PendingAction[] mPendingActions;
    private final FlashZipsTaskListener mListener;

    public final Object mLinesLock = new Object();
    public ArrayList<String> mLines = new ArrayList<>();
    public int mTotal = -1;
    public int mFailed = -1;

    public interface FlashZipsTaskListener extends BaseServiceTaskListener {
        void onFlashedZips(int taskId, int totalActions, int failedActions);

        void onCommandOutput(int taskId, String line);
    }

    public FlashZipsTask(int taskId, Context context, PendingAction[] pendingActions,
                         FlashZipsTaskListener listener) {
        super(taskId, context);
        mPendingActions = pendingActions;
        mListener = listener;
    }

    public String[] getLines() {
        synchronized (mLinesLock) {
            return mLines.toArray(new String[mLines.size()]);
        }
    }

    @Override
    public void execute() {
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

            for (PendingAction pa : mPendingActions) {
                if (pa.type != PendingAction.Type.INSTALL_ZIP) {
                    throw new IllegalStateException("Only INSTALL_ZIP is supported right now");
                }

                printSeparator();

                printBoldText(Color.MAGENTA, "Processing action: Flash zip\n");
                printBoldText(Color.MAGENTA, "- ZIP file: " + pa.zipFile + "\n");
                printBoldText(Color.MAGENTA, "- Destination: " + pa.romId + "\n");

                // Extract mbtool from the zip file
                File zipInstaller = new File(
                        getContext().getCacheDir() + File.separator + "rom-installer");
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

                int ret = runRootCommand("/rom-installer --romid " +
                        CommandUtils.quoteArg(pa.romId) + " " +
                        CommandUtils.quoteArg(pa.zipFile));

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

            String frac = succeeded + "/" + mPendingActions.length;

            printBoldText(Color.CYAN, "Successfully completed " + frac + " actions\n");

            mTotal = mPendingActions.length;
            mFailed = mPendingActions.length - succeeded;
            mListener.onFlashedZips(getTaskId(), mTotal, mFailed);
        }
    }

    private void onCommandOutput(String line) {
        synchronized (mLinesLock) {
            mLines.add(line);
        }
        mListener.onCommandOutput(getTaskId(), line);
    }

    private void printBoldText(Color color, String text) {
        onCommandOutput(
                AnsiStuff.format(text, EnumSet.of(color), null, EnumSet.of(Attribute.BOLD)));
    }

    private void printSeparator() {
        printBoldText(Color.WHITE, StringUtils.repeat('-', 16) + "\n");
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

    private int runRootCommand(String command) {
        printBoldText(Color.YELLOW, "Running command: " + command + "\n");

        RootCommandParams params = new RootCommandParams();
        params.command = command;
        params.listener = new RootCommandListener() {
            @Override
            public void onNewOutputLine(String line) {
                onCommandOutput(line + "\n");
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
}
