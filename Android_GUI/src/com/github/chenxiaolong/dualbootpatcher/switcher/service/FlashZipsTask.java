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
import android.text.TextUtils;

import com.github.chenxiaolong.dualbootpatcher.AnsiStuff;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Attribute;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Color;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SignedExecCompletion;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SignedExecOutputCallback;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction;

import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.EnumSet;

import mbtool.daemon.v3.SignedExecResult;

public final class FlashZipsTask extends BaseServiceTask implements SignedExecOutputCallback {
    private static final boolean DEBUG_USE_ALTERNATE_MBTOOL = false;
    private static final String ALTERNATE_MBTOOL_PATH = "/data/local/tmp/mbtool_recovery";
    private static final String ALTERNATE_MBTOOL_SIG_PATH = "/data/local/tmp/mbtool_recovery.sig";

    private static final String UPDATE_BINARY = "META-INF/com/google/android/update-binary";
    private static final String UPDATE_BINARY_SIG = UPDATE_BINARY + ".sig";

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
        int succeeded = 0;

        try {
            printBoldText(Color.YELLOW, "Connecting to mbtool...\n");

            MbtoolSocket socket = MbtoolSocket.getInstance();
            socket.connect(getContext());

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
                File zipInstallerSig = new File(
                        getContext().getCacheDir() + File.separator + "rom-installer.sig");
                zipInstaller.delete();
                zipInstallerSig.delete();

                if (DEBUG_USE_ALTERNATE_MBTOOL) {
                    printBoldText(Color.YELLOW, "[DEBUG] Copying alternate mbtool binary: "
                            + ALTERNATE_MBTOOL_PATH + "\n");
                    try {
                        org.apache.commons.io.FileUtils.copyFile(
                                new File(ALTERNATE_MBTOOL_PATH), zipInstaller);
                    } catch (IOException e) {
                        printBoldText(Color.RED, "Failed to copy alternate mbtool binary\n");
                        return;
                    }
                    printBoldText(Color.YELLOW, "[DEBUG] Copying alternate mbtool signature: "
                            + ALTERNATE_MBTOOL_SIG_PATH + "\n");
                    try {
                        org.apache.commons.io.FileUtils.copyFile(
                                new File(ALTERNATE_MBTOOL_SIG_PATH), zipInstallerSig);
                    } catch (IOException e) {
                        printBoldText(Color.RED, "Failed to copy alternate mbtool signature\n");
                        return;
                    }
                } else {
                    printBoldText(Color.YELLOW, "Extracting mbtool ROM installer from the zip file\n");

                    if (!FileUtils.zipExtractFile(pa.zipFile, UPDATE_BINARY,
                            zipInstaller.getPath())) {
                        printBoldText(Color.RED, "Failed to extract update-binary\n");
                        return;
                    }
                    printBoldText(Color.YELLOW, "Extracting mbtool signature from the zip file\n");
                    if (!FileUtils.zipExtractFile(pa.zipFile, UPDATE_BINARY_SIG,
                            zipInstallerSig.getPath())) {
                        printBoldText(Color.RED, "Failed to extract update-binary.sig\n");
                        return;
                    }
                }

                String[] args = new String[] { "--romid", pa.romId, pa.zipFile };

                printBoldText(Color.YELLOW, "Running rom-installer with arguments: ["
                        + TextUtils.join(", ", args) + "]\n");

                SignedExecCompletion completion = socket.signedExec(getContext(),
                        zipInstaller.getAbsolutePath(), zipInstallerSig.getAbsolutePath(),
                        "rom-installer", args, this);

                switch (completion.result) {
                case SignedExecResult.PROCESS_EXITED:
                    printBoldText(completion.exitStatus == 0 ? Color.GREEN : Color.RED,
                            "\nCommand returned: " + completion.exitStatus + "\n");
                    if (completion.exitStatus == 0) {
                        succeeded++;
                    } else {
                        return;
                    }
                    break;
                case SignedExecResult.PROCESS_KILLED_BY_SIGNAL:
                    printBoldText(Color.RED, "\nProcess killed by signal: "
                            + completion.termSig + "\n");
                    return;
                case SignedExecResult.INVALID_SIGNATURE:
                    printBoldText(Color.RED, "\nThe mbtool binary has an invalid signature. This"
                            + " can happen if an unofficial app was used to patch the file or if"
                            + " the zip was maliciously modified. The file was NOT flashed.\n");
                    return;
                case SignedExecResult.OTHER_ERROR:
                default:
                    printBoldText(Color.RED, "\nError: " + completion.errorMsg + "\n");
                    return;
                }

                zipInstaller.delete();
                zipInstallerSig.delete();
            }
        } catch (IOException e) {
            e.printStackTrace();
            printBoldText(Color.RED, "mbtool connection error: " + e.getMessage() + "\n");
        } finally {
            printSeparator();

            String frac = succeeded + "/" + mPendingActions.length;

            printBoldText(Color.CYAN, "Successfully completed " + frac + " actions\n");

            mTotal = mPendingActions.length;
            mFailed = mPendingActions.length - succeeded;
            mListener.onFlashedZips(getTaskId(), mTotal, mFailed);
        }
    }

    @Override
    public void onOutputLine(String line) {
        onCommandOutput(line);
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
}
