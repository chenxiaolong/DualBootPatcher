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

import android.os.Bundle;
import android.util.Log;

import com.stericson.RootShell.exceptions.RootDeniedException;
import com.stericson.RootShell.execution.Command;
import com.stericson.RootTools.RootTools;

import java.io.IOException;
import java.util.concurrent.TimeoutException;

public final class CommandUtils {
    private static final String TAG = CommandUtils.class.getSimpleName();

    public static class CommandResult {
        public int exitCode;

        // The output filter can put things here
        public Bundle data = new Bundle();
    }

    public interface RootCommandListener {
        void onNewOutputLine(String line);

        void onCommandCompletion(CommandResult result);
    }

    public static class RootCommandParams {
        public String command;
        public RootCommandListener listener;
        public RootLiveOutputFilter filter;
        public boolean logOutput = true;
    }

    public interface RootLiveOutputFilter {
        void onOutputLine(RootCommandParams params, CommandResult result, String line);
    }

    public static boolean requestRootAccess() {
        return RootTools.isAccessGiven();
    }

    public static class RootCommandRunner extends Thread {
        private final RootCommandParams mParams;
        private CommandResult mResult;

        public RootCommandRunner(RootCommandParams params) {
            super();
            mParams = params;
        }

        public CommandResult getResult() {
            return mResult;
        }

        @Override
        public void run() {
            if (!requestRootAccess()) {
                return;
                // throw new RootDeniedException("Root access denied");
            }

            mResult = new CommandResult();

            try {
                Log.v(TAG, "Command: " + mParams.command);

                Command command = new Command(0, 0, mParams.command) {
                    @Override
                    public void commandOutput(int id, String line) {
                        if (mParams.logOutput) {
                            Log.d(TAG, "Root command output: " + line);
                        }

                        if (mParams.filter != null) {
                            mParams.filter.onOutputLine(mParams, mResult, line);
                        }

                        if (mParams.listener != null) {
                            mParams.listener.onNewOutputLine(line);
                        }

                        super.commandOutput(id, line);
                    }
                };

                RootTools.getShell(true).add(command);

                while (!command.isFinished()) {
                    synchronized (command) {
                        command.wait(2000);
                    }
                }

                mResult.exitCode = command.getExitCode();

                // TODO: Should this be done on another thread?
                if (mParams.listener != null) {
                    mParams.listener.onCommandCompletion(mResult);
                }
            } catch (IOException e) {
                e.printStackTrace();
            } catch (InterruptedException e) {
                Log.e(TAG, "Process was interrupted", e);
            } catch (TimeoutException e) {
                Log.e(TAG, "Process timed out", e);
            } catch (RootDeniedException e) {
                Log.e(TAG, "Root access was denied", e);
            }
        }
    }

    public static int runRootCommand(String command) {
        RootCommandParams params = new RootCommandParams();
        params.command = command;

        RootCommandRunner cmd = new RootCommandRunner(params);
        cmd.start();

        try {
            cmd.join();
            CommandResult result = cmd.getResult();

            if (result != null) {
                return result.exitCode;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return -1;
    }

    public static int runRootCommand(String... args) {
        StringBuilder command = new StringBuilder();
        for (int i = 0; i < args.length; i++) {
            if (i > 0) {
                command.append(' ');
            }
            command.append(shellQuote(args[i]));
        }
        return runRootCommand(command.toString());
    }

    public static String shellQuote(String text) {
        return "'" + text.replace("'", "'\"'\"'") + "'";
    }
}
