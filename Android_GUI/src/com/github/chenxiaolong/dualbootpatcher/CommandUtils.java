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

package com.github.chenxiaolong.dualbootpatcher;

import android.util.Log;

import com.stericson.RootShell.RootShell;
import com.stericson.RootShell.exceptions.RootDeniedException;
import com.stericson.RootShell.execution.Command;

import java.io.IOException;
import java.util.concurrent.TimeoutException;

public final class CommandUtils {
    private static final String TAG = CommandUtils.class.getSimpleName();

    // Global lock because RootTools' design only supports a single root session
    private static final Object LOCK = new Object();

    public interface RootOutputListener {
        void onNewOutputLine(String line);
    }

    private static class RootCommandResult {
        int exitCode;
        String terminationMessage;
    }

    public static class RootExecutionException extends Exception {
        public RootExecutionException(String detailMessage) {
            super(detailMessage);
        }

        public RootExecutionException(String message, Throwable cause) {
            super(message, cause);
        }

        public RootExecutionException(Throwable cause) {
            super(cause);
        }
    }

    public static int runRootCommand(final RootOutputListener listener, String commandStr)
            throws RootExecutionException, RootDeniedException {
        synchronized (LOCK) {
            final RootCommandResult result = new RootCommandResult();

            try {
                Log.v(TAG, "Command: " + commandStr);

                Command command = new Command(0, 0, commandStr) {
                    @Override
                    public void commandOutput(int id, String line) {
                        Log.d(TAG, "Root command output: " + line);

                        if (listener != null) {
                            listener.onNewOutputLine(line);
                        }

                        super.commandOutput(id, line);
                    }

                    @Override
                    public void commandTerminated(int id, String reason) {
                        synchronized (result) {
                            result.exitCode = -1;
                            result.terminationMessage = reason;
                            result.notify();
                        }

                        super.commandTerminated(id, reason);
                    }

                    @Override
                    public void commandCompleted(int id, int exitcode) {
                        synchronized (result) {
                            result.exitCode = exitcode;
                            result.notify();
                        }

                        super.commandCompleted(id, exitcode);
                    }
                };

                RootShell.getShell(true).add(command);

                synchronized (result) {
                    result.wait();
                }

                if (result.exitCode == -1) {
                    throw new RootExecutionException(
                            "Process was terminated: " + result.terminationMessage);
                }

                return result.exitCode;
            } catch (IOException e) {
                throw new RootExecutionException("I/O error", e);
            } catch (InterruptedException e) {
                throw new RootExecutionException("Process was interrupted", e);
            } catch (TimeoutException e) {
                throw new RootExecutionException("Process timed out", e);
            }
        }
    }

    public static int runRootCommand(RootOutputListener listener, String... commandArgs)
            throws RootDeniedException, RootExecutionException {
        return runRootCommand(listener, shellQuote(commandArgs));
    }

    public static int runRootCommand(String commandStr)
            throws RootDeniedException, RootExecutionException {
        return runRootCommand(null, commandStr);
    }

    public static int runRootCommand(String... commandArgs)
            throws RootDeniedException, RootExecutionException {
        return runRootCommand(null, commandArgs);
    }

    public static String shellQuote(String... args) {
        StringBuilder command = new StringBuilder();
        for (int i = 0; i < args.length; i++) {
            if (i > 0) {
                command.append(' ');
            }
            command.append(shellQuote(args[i]));
        }
        return command.toString();
    }

    public static String shellQuote(String text) {
        return "'" + text.replace("'", "'\"'\"'") + "'";
    }
}
