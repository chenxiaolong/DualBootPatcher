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
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.AnsiStuff;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Attribute;
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Color;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SignedExecCompletion;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SignedExecOutputListener;

import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.EnumSet;

import mbtool.daemon.v3.SignedExecResult;

public final class BackupRestoreRomTask extends BaseServiceTask implements SignedExecOutputListener {
    private static final String TAG = BackupRestoreRomTask.class.getSimpleName();

    public static final String ACTION_BACKUP_ROM = "backup_rom";
    public static final String ACTION_RESTORE_ROM = "restore_rom";

    private final String mAction;
    private final String mRomId;
    private final String[] mTargets;
    private final String mName;
    private final String mBackupDir;
    private final boolean mForce;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    public ArrayList<String> mLines = new ArrayList<>();
    private boolean mSuccess = false;

    public interface BackupRestoreRomTaskListener extends BaseServiceTaskListener,
            MbtoolErrorListener {
        void onBackupRestoreFinished(int taskId, String action, boolean success);

        void onCommandOutput(int taskId, String line);
    }

    public BackupRestoreRomTask(int taskId, Context context, String action, String romId,
                                String[] targets, String name, String backupDir, boolean force) {
        super(taskId, context);
        if (!ACTION_BACKUP_ROM.equals(action) && !ACTION_RESTORE_ROM.equals(action)) {
            throw new IllegalArgumentException("Invalid action: " + action);
        }
        mAction = action;
        mRomId = romId;
        mTargets = targets;
        mName = name;
        mBackupDir = backupDir;
        mForce = force;
    }

    @Override
    public void execute() {
        boolean success = false;

        MbtoolConnection conn = null;

        try {
            printBoldText(Color.YELLOW, "Connecting to mbtool...");

            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            printBoldText(Color.YELLOW, " connected\n");

            printSeparator();

            if (ACTION_BACKUP_ROM.equals(mAction)) {
                printBoldText(Color.MAGENTA, "Backup:\n");
            } else if (ACTION_RESTORE_ROM.equals(mAction)) {
                printBoldText(Color.MAGENTA, "Restore:\n");
            }

            printBoldText(Color.MAGENTA, "- ROM ID: " + mRomId + "\n");
            printBoldText(Color.MAGENTA, "- Targets: " + Arrays.toString(mTargets) + "\n");
            printBoldText(Color.MAGENTA, "- Name: " + mName + "\n");
            printBoldText(Color.MAGENTA, "- Backup dir: " + mBackupDir + "\n");
            printBoldText(Color.MAGENTA, "- Force/overwrite: " + mForce + "\n");

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

            String argv0 = null;
            if (ACTION_BACKUP_ROM.equals(mAction)) {
                argv0 = "backup";
            } else if (ACTION_RESTORE_ROM.equals(mAction)) {
                argv0 = "restore";
            }

            ArrayList<String> args = new ArrayList<>();
            args.add("-r");
            args.add(mRomId);
            args.add("-t");
            args.add(StringUtils.join(mTargets, ','));
            if (mName != null) {
                args.add("-n");
                args.add(mName);
            }
            if (mBackupDir != null) {
                args.add("-d");
                args.add(mBackupDir);
            }
            if (mForce) {
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
                if (completion.exitStatus == 0) {
                    success = true;
                }
                break;
            case SignedExecResult.PROCESS_KILLED_BY_SIGNAL:
                printBoldText(Color.RED, "\nProcess killed by signal: "
                        + completion.termSig + "\n");
                break;
            case SignedExecResult.INVALID_SIGNATURE:
                printBoldText(Color.RED, "\nThe mbtool binary has an invalid signature. This"
                        + " means DualBootPatcher's data files have been maliciously modified"
                        + " by somebody or another app. The backup/restore process has been"
                        + " cancelled.\n");
                break;
            case SignedExecResult.OTHER_ERROR:
            default:
                printBoldText(Color.RED, "\nError: " + completion.errorMsg + "\n");
                break;
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

            synchronized (mStateLock) {
                mSuccess = success;
                sendOnBackupRestoreFinished();
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
                sendOnBackupRestoreFinished();
            }
        }
    }

    private void sendOnCommandOutput(final String line) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((BackupRestoreRomTaskListener) listener).onCommandOutput(getTaskId(), line);
            }
        });
    }

    private void sendOnBackupRestoreFinished() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((BackupRestoreRomTaskListener) listener).onBackupRestoreFinished(
                        getTaskId(), mAction, mSuccess);
            }
        });
    }

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((BackupRestoreRomTaskListener) listener).onMbtoolConnectionFailed(
                        getTaskId(), reason);
            }
        });
    }
}
