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
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.text.TextUtils;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.AnsiStuff;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Attribute;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Color;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMiscStuff;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SignedExecCompletion;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SignedExecOutputListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction;
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.RomInstallerParams;

import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.EnumSet;

import mbtool.daemon.v3.PathDeleteFlag;
import mbtool.daemon.v3.SignedExecResult;

public final class MbtoolTask extends BaseServiceTask implements SignedExecOutputListener {
    private static final String TAG = MbtoolTask.class.getSimpleName();

    private static final boolean DEBUG_USE_ALTERNATE_MBTOOL = false;
    private static final String ALTERNATE_MBTOOL_PATH = "/data/local/tmp/mbtool_recovery";
    private static final String ALTERNATE_MBTOOL_SIG_PATH = "/data/local/tmp/mbtool_recovery.sig";

    private static final String UPDATE_BINARY = "META-INF/com/google/android/update-binary";
    private static final String UPDATE_BINARY_SIG = UPDATE_BINARY + ".sig";

    private final MbtoolAction[] mActions;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private ArrayList<String> mLines = new ArrayList<>();
    private int mAttempted = -1;
    private int mSucceeded = -1;

    public interface MbtoolTaskListener extends BaseServiceTaskListener, MbtoolErrorListener {
        void onMbtoolTasksCompleted(int taskId, MbtoolAction[] actions, int attempted,
                                    int succeeded);

        void onCommandOutput(int taskId, String line);
    }

    public MbtoolTask(int taskId, Context context, MbtoolAction[] actions) {
        super(taskId, context);
        mActions = actions;
    }

    private void deleteWithPrejudice(String path, MbtoolInterface iface)
            throws IOException, MbtoolException, MbtoolCommandException {
        iface.pathDelete(path, PathDeleteFlag.REMOVE);
    }

    private String translateEmulatedPath(String path) {
        String emuSourcePath = System.getenv("EMULATED_STORAGE_SOURCE");
        String emuTargetPath = System.getenv("EMULATED_STORAGE_TARGET");

        if (emuSourcePath != null && emuTargetPath != null) {
            if (path.startsWith(emuTargetPath)) {
                printBoldText(Color.YELLOW, "Path uses EMULATED_STORAGE_TARGET\n");
                return emuSourcePath + path.substring(emuTargetPath.length());
            }
        }

        return path;
    }

    private boolean performRomInstallation(RomInstallerParams params, MbtoolInterface iface)
            throws IOException, MbtoolCommandException, MbtoolException {
        printSeparator();

        printBoldText(Color.MAGENTA, "Processing action: Flash file\n");
        printBoldText(Color.MAGENTA, "- File URI: " + params.getUri() + "\n");
        printBoldText(Color.MAGENTA, "- File name: " + params.getDisplayName() + "\n");
        printBoldText(Color.MAGENTA, "- Destination: " + params.getRomId() + "\n");

        // Extract mbtool from the zip file
        File zipInstaller = new File(
                getContext().getCacheDir() + File.separator + "rom-installer");
        File zipInstallerSig = new File(
                getContext().getCacheDir() + File.separator + "rom-installer.sig");

        try {
            // An attacker can't exploit this by creating an undeletable malicious file because
            // mbtool will not execute an untrusted binary
            try {
                deleteWithPrejudice(zipInstaller.getAbsolutePath(), iface);
                deleteWithPrejudice(zipInstallerSig.getAbsolutePath(), iface);
            } catch (MbtoolCommandException e) {
                // Ignore because we can't detect ENOENT yet
            }

            if (DEBUG_USE_ALTERNATE_MBTOOL) {
                printBoldText(Color.YELLOW, "[DEBUG] Copying alternate mbtool binary: "
                        + ALTERNATE_MBTOOL_PATH + "\n");
                try {
                    org.apache.commons.io.FileUtils.copyFile(
                            new File(ALTERNATE_MBTOOL_PATH), zipInstaller);
                } catch (IOException e) {
                    printBoldText(Color.RED, "Failed to copy alternate mbtool binary\n");
                    return false;
                }
                printBoldText(Color.YELLOW, "[DEBUG] Copying alternate mbtool signature: "
                        + ALTERNATE_MBTOOL_SIG_PATH + "\n");
                try {
                    org.apache.commons.io.FileUtils.copyFile(
                            new File(ALTERNATE_MBTOOL_SIG_PATH), zipInstallerSig);
                } catch (IOException e) {
                    printBoldText(Color.RED, "Failed to copy alternate mbtool signature\n");
                    return false;
                }
            } else {
                printBoldText(Color.YELLOW, "Extracting mbtool ROM installer from the zip file\n");

                if (!FileUtils.zipExtractFile(getContext(), params.getUri(), UPDATE_BINARY,
                        zipInstaller.getPath())) {
                    printBoldText(Color.RED, "Failed to extract update-binary\n");
                    return false;
                }
                printBoldText(Color.YELLOW, "Extracting mbtool signature from the zip file\n");
                if (!FileUtils.zipExtractFile(getContext(), params.getUri(), UPDATE_BINARY_SIG,
                        zipInstallerSig.getPath())) {
                    printBoldText(Color.RED, "Failed to extract update-binary.sig\n");
                    return false;
                }
            }

            ParcelFileDescriptor pfd = null;
            SignedExecCompletion completion;
            try {
                pfd = getContext().getContentResolver().openFileDescriptor(params.getUri(), "r");
                if (pfd == null) {
                    printBoldText(Color.RED, "Failed to open: " + params.getUri());
                    return false;
                }

                String fdSource = "/proc/self/fd/" + pfd.getFd();
                String fdTarget = LibMiscStuff.readLink(fdSource);

                if (fdTarget == null) {
                    printBoldText(Color.RED, "Failed to resolve symlink: " + fdSource);
                    return false;
                }

                String[] args = new String[]{"--romid", params.getRomId(),
                        translateEmulatedPath(fdTarget)};

                printBoldText(Color.YELLOW, "Running rom-installer with arguments: ["
                        + TextUtils.join(", ", args) + "]\n");

                completion = iface.signedExec(
                        zipInstaller.getAbsolutePath(), zipInstallerSig.getAbsolutePath(),
                        "rom-installer", args, this);
            } finally {
                IOUtils.closeQuietly(pfd);
            }

            switch (completion.result) {
            case SignedExecResult.PROCESS_EXITED:
                printBoldText(completion.exitStatus == 0 ? Color.GREEN : Color.RED,
                        "\nCommand returned: " + completion.exitStatus + "\n");
                return completion.exitStatus == 0;
            case SignedExecResult.PROCESS_KILLED_BY_SIGNAL:
                printBoldText(Color.RED, "\nProcess killed by signal: "
                        + completion.termSig + "\n");
                return false;
            case SignedExecResult.INVALID_SIGNATURE:
                printBoldText(Color.RED, "\nThe mbtool binary has an invalid signature. This"
                        + " can happen if an unofficial app was used to patch the file or if"
                        + " the zip was maliciously modified. The file was NOT flashed.\n");
                return false;
            case SignedExecResult.OTHER_ERROR:
            default:
                printBoldText(Color.RED, "\nError: " + completion.errorMsg + "\n");
                return false;
            }
        } finally {
             // Clean up installer files
            try {
                deleteWithPrejudice(zipInstaller.getAbsolutePath(), iface);
                deleteWithPrejudice(zipInstallerSig.getAbsolutePath(), iface);
            } catch (Exception e) {
                // Ignore because we can't detect ENOENT yet
            }
        }
    }

    private boolean performBackupRestore(BackupRestoreParams params, MbtoolInterface iface)
            throws IOException, MbtoolCommandException, MbtoolException {
        printSeparator();

        switch (params.getAction()) {
        case BACKUP:
            printBoldText(Color.MAGENTA, "Backup:\n");
            break;
        case RESTORE:
            printBoldText(Color.MAGENTA, "Restore:\n");
            break;
        }

        printBoldText(Color.MAGENTA, "- ROM ID: " + params.getRomId() + "\n");
        printBoldText(Color.MAGENTA, "- Targets: " + Arrays.toString(params.getTargets()) + "\n");
        printBoldText(Color.MAGENTA, "- Backup name: " + params.getBackupName() + "\n");
        printBoldText(Color.MAGENTA, "- Backup dir: " + params.getBackupDir() + "\n");
        printBoldText(Color.MAGENTA, "- Force/overwrite: " + params.getForce() + "\n");

        printSeparator();

        printBoldText(Color.YELLOW, "Extracting patcher data archive\n");
        PatcherUtils.extractPatcher(getContext());

        printSeparator();

        String abi;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            abi = Build.SUPPORTED_ABIS[0];
        } else {
            abi = Build.CPU_ABI;
        }

        File mbtoolRecovery = new File(PatcherUtils.getTargetDirectory(getContext()) +
                File.separator + "binaries" +
                File.separator + "android" +
                File.separator + abi +
                File.separator + "mbtool_recovery");
        File mbtoolRecoverySig = new File(PatcherUtils.getTargetDirectory(getContext()) +
                File.separator + "binaries" +
                File.separator + "android" +
                File.separator + abi +
                File.separator + "mbtool_recovery.sig");

        String argv0;
        switch (params.getAction()) {
        case BACKUP:
            argv0 = "backup";
            break;
        case RESTORE:
            argv0 = "restore";
            break;
        default:
            throw new IllegalStateException("Invalid action: " + params.getAction());
        }

        ArrayList<String> args = new ArrayList<>();
        args.add("-r");
        args.add(params.getRomId());
        args.add("-t");
        args.add(StringUtils.join(params.getTargets(), ','));
        if (params.getBackupName() != null) {
            args.add("-n");
            args.add(params.getBackupName());
        }
        if (params.getBackupDir() != null) {
            args.add("-d");
            args.add(translateEmulatedPath(params.getBackupDir()));
        }
        if (params.getForce()) {
            args.add("-f");
        }

        String[] argsArray = args.toArray(new String[args.size()]);

        SignedExecCompletion completion = iface.signedExec(
                mbtoolRecovery.getAbsolutePath(), mbtoolRecoverySig.getAbsolutePath(),
                argv0, argsArray, this);

        switch (completion.result) {
        case SignedExecResult.PROCESS_EXITED:
            printBoldText(completion.exitStatus == 0 ? Color.GREEN : Color.RED,
                    "\nCommand returned: " + completion.exitStatus + "\n");
            return completion.exitStatus == 0;
        case SignedExecResult.PROCESS_KILLED_BY_SIGNAL:
            printBoldText(Color.RED, "\nProcess killed by signal: "
                    + completion.termSig + "\n");
            return false;
        case SignedExecResult.INVALID_SIGNATURE:
            printBoldText(Color.RED, "\nThe mbtool binary has an invalid signature. This"
                    + " means DualBootPatcher's data files have been maliciously modified"
                    + " by somebody or another app. The backup/restore process has been"
                    + " cancelled.\n");
            return false;
        case SignedExecResult.OTHER_ERROR:
        default:
            printBoldText(Color.RED, "\nError: " + completion.errorMsg + "\n");
            return false;
        }
    }

    @Override
    public void execute() {
        MbtoolConnection conn = null;

        int succeeded = 0;
        int attempted = 0;

        try {
            printBoldText(Color.YELLOW, "Connecting to mbtool...");

            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            printBoldText(Color.YELLOW, " connected\n");

            for (MbtoolAction action : mActions) {
                attempted++;

                boolean result;

                switch (action.getType()) {
                case ROM_INSTALLER:
                    result = performRomInstallation(action.getRomInstallerParams(), iface);
                    break;
                case BACKUP_RESTORE:
                    result = performBackupRestore(action.getBackupRestoreParams(), iface);
                    break;
                default:
                    throw new IllegalStateException("Invalid action type: " + action.getType());
                }

                if (result) {
                    succeeded++;
                } else {
                    break;
                }
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

            Color color;
            if (attempted == mActions.length && succeeded == mActions.length) {
                color = Color.GREEN;
            } else {
                color = Color.RED;
            }

            printBoldText(color, "Completed:\n");
            printBoldText(color, attempted + " actions attempted\n");
            printBoldText(color, succeeded + " actions succeeded\n");
            printBoldText(color, (mActions.length - attempted) + " actions skipped\n");

            synchronized (mStateLock) {
                mAttempted = attempted;
                mSucceeded = succeeded;
                sendTasksCompleted();
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
        synchronized (mStateLock) {
            super.onListenerAdded(listener);

            for (String line : mLines) {
                sendOnCommandOutput(line);
            }
            if (mFinished) {
                sendTasksCompleted();
            }
        }
    }

    private void sendOnCommandOutput(final String line) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((MbtoolTaskListener) listener).onCommandOutput(getTaskId(), line);
            }
        });
    }

    private void sendTasksCompleted() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((MbtoolTaskListener) listener).onMbtoolTasksCompleted(
                        getTaskId(), mActions, mAttempted, mSucceeded);
            }
        });
    }

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((MbtoolTaskListener) listener).onMbtoolConnectionFailed(getTaskId(), reason);
            }
        });
    }
}
