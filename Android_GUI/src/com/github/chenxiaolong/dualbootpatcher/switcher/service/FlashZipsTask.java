/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.AnsiStuff;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Attribute;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Color;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SignedExecCompletion;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SignedExecOutputListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction;

import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.EnumSet;

import mbtool.daemon.v3.SignedExecResult;

public final class FlashZipsTask extends BaseServiceTask implements SignedExecOutputListener {
    private static final String TAG = FlashZipsTask.class.getSimpleName();

    private static final boolean DEBUG_USE_ALTERNATE_MBTOOL = false;
    private static final String ALTERNATE_MBTOOL_PATH = "/data/local/tmp/mbtool_recovery";
    private static final String ALTERNATE_MBTOOL_SIG_PATH = "/data/local/tmp/mbtool_recovery.sig";

    private static final String UPDATE_BINARY = "META-INF/com/google/android/update-binary";
    private static final String UPDATE_BINARY_SIG = UPDATE_BINARY + ".sig";

    private final PendingAction[] mPendingActions;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private ArrayList<String> mLines = new ArrayList<>();
    private int mTotal = -1;
    private int mFailed = -1;

    public interface FlashZipsTaskListener extends BaseServiceTaskListener, MbtoolErrorListener {
        void onFlashedZips(int taskId, int totalActions, int failedActions);

        void onCommandOutput(int taskId, String line);
    }

    public FlashZipsTask(int taskId, Context context, PendingAction[] pendingActions) {
        super(taskId, context);
        mPendingActions = pendingActions;
    }

    @Override
    public void execute() {
        int succeeded = 0;

        MbtoolConnection conn = null;

        try {
            printBoldText(Color.YELLOW, "Connecting to mbtool...");

            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            printBoldText(Color.YELLOW, " connected\n");

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

                SignedExecCompletion completion = iface.signedExec(
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
            Log.e(TAG, "mbtool communication error", e);
            printBoldText(Color.RED, "mbtool connection error: " + e.getMessage() + "\n");
        } catch (MbtoolException e) {
            Log.e(TAG, "mbtool error", e);
            printBoldText(Color.RED, "mbtool error: " + e.getMessage() + "\n");
            sendOnMbtoolError(e.getReason());
        } catch (MbtoolCommandException e) {
            Log.w(TAG, "mbtool command error", e);
            printBoldText(Color.RED, "mbtool command error: " + e.getMessage() + "\n");
        } finally {
            IOUtils.closeQuietly(conn);

            printSeparator();

            String frac = succeeded + "/" + mPendingActions.length;

            printBoldText(Color.CYAN, "Successfully completed " + frac + " actions\n");

            synchronized (mStateLock) {
                mTotal = mPendingActions.length;
                mFailed = mPendingActions.length - succeeded;
                sendOnFlashedZips();
                mFinished = true;
            }
        }
    }

    @Override
    public void onOutputLine(String line) {
        onCommandOutput(line);
    }

    private void onCommandOutput(String line) {
        synchronized (mStateLock) {
            mLines.add(line);
            sendOnCommandOutput(line);
        }
    }

    private void printBoldText(Color color, String text) {
        onCommandOutput(
                AnsiStuff.format(text, EnumSet.of(color), null, EnumSet.of(Attribute.BOLD)));
    }

    private void printSeparator() {
        printBoldText(Color.WHITE, StringUtils.repeat('-', 16) + "\n");
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            for (String line : mLines) {
                sendOnCommandOutput(line);
            }
            if (mFinished) {
                sendOnFlashedZips();
            }
        }
    }

    private void sendOnCommandOutput(final String line) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((FlashZipsTaskListener) listener).onCommandOutput(getTaskId(), line);
            }
        });
    }

    private void sendOnFlashedZips() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((FlashZipsTaskListener) listener).onFlashedZips(getTaskId(), mTotal, mFailed);
            }
        });
    }

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((FlashZipsTaskListener) listener).onMbtoolConnectionFailed(getTaskId(), reason);
            }
        });
    }
}
