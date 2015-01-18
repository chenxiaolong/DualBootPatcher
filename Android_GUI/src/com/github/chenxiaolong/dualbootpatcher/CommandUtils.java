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

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.concurrent.TimeoutException;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.util.Log;

import com.stericson.RootTools.RootTools;
import com.stericson.RootTools.exceptions.RootDeniedException;
import com.stericson.RootTools.execution.Command;

import org.apache.commons.lang3.StringUtils;

public final class CommandUtils {
    private static final String TAG = CommandUtils.class.getSimpleName();

    public static final String STREAM_STDOUT = "stdout";
    public static final String STREAM_STDERR = "stderr";

    public static interface CommandListener {
        public void onNewOutputLine(String line, String stream);

        public void onCommandCompletion(CommandResult result);
    }

    public static class CommandParams {
        public String[] command;
        public String[] environment;
        public File cwd;
        public CommandListener listener;
        public LiveOutputFilter filter;
        public boolean logStdout = true;
        public boolean logStderr = true;
    }

    public static class CommandResult {
        public int exitCode;

        // The output filter can put things here
        public Bundle data = new Bundle();
    }

    public static interface LiveOutputFilter {
        public void onStdoutLine(CommandParams params, CommandResult result,
                String line);

        public void onStderrLine(CommandParams params, CommandResult result,
                String line);
    }

    public static class CommandRunner extends Thread {
        private BufferedReader stdout = null;
        private BufferedReader stderr = null;

        private final CommandParams mParams;
        private CommandResult mResult;

        public CommandRunner(CommandParams params) {
            super();
            mParams = params;
        }

        public CommandResult getResult() {
            return mResult;
        }

        @Override
        public void run() {
            mResult = new CommandResult();

            try {
                Log.v(TAG, "Command: " + Arrays.toString(mParams.command));

                ProcessBuilder pb = new ProcessBuilder(
                        Arrays.asList(mParams.command));

                if (mParams.environment != null) {
                    Log.v(TAG,
                            "Environment: "
                                    + Arrays.toString(mParams.environment));

                    for (String s : mParams.environment) {
                        String[] split = s.split("=");
                        pb.environment().put(split[0], split[1]);
                    }
                } else {
                    Log.v(TAG, "Environment: Inherited");
                }

                if (mParams.cwd != null) {
                    Log.v(TAG, "Working directory: " + mParams.cwd);

                    pb.directory(mParams.cwd);
                } else {
                    Log.v(TAG, "Working directory: Inherited");
                }

                Process p = pb.start();

                stdout = new BufferedReader(new InputStreamReader(
                        p.getInputStream()));
                stderr = new BufferedReader(new InputStreamReader(
                        p.getErrorStream()));

                // Read stdout and stderr at the same time
                Thread stdoutReader = new Thread() {
                    @Override
                    public void run() {
                        String s;
                        try {
                            while ((s = stdout.readLine()) != null) {
                                if (mParams.logStdout) {
                                    Log.d(TAG, "Standard output: " + s);
                                }

                                if (mParams.filter != null) {
                                    mParams.filter.onStdoutLine(mParams,
                                            mResult, s);
                                }

                                if (mParams.listener != null) {
                                    mParams.listener.onNewOutputLine(s,
                                            STREAM_STDOUT);
                                }
                            }
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                };

                Thread stderrReader = new Thread() {
                    @Override
                    public void run() {
                        String s;
                        try {
                            while ((s = stderr.readLine()) != null) {
                                if (mParams.logStderr) {
                                    Log.d(TAG, "Standard error: " + s);
                                }

                                if (mParams.filter != null) {
                                    mParams.filter.onStderrLine(mParams,
                                            mResult, s);
                                }

                                if (mParams.listener != null) {
                                    mParams.listener.onNewOutputLine(s,
                                            STREAM_STDERR);
                                }
                            }
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                };

                stdoutReader.start();
                stderrReader.start();
                stdoutReader.join();
                stderrReader.join();
                p.waitFor();
                mResult.exitCode = p.exitValue();

                // TODO: Should this be done on another thread?
                if (mParams.listener != null) {
                    mParams.listener.onCommandCompletion(mResult);
                }
            } catch (IOException e) {
                e.printStackTrace();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public static interface RootCommandListener {
        public void onNewOutputLine(String line);

        public void onCommandCompletion(CommandResult result);
    }

    public static class RootCommandParams {
        public String command;
        public RootCommandListener listener;
        public RootLiveOutputFilter filter;
        public boolean logOutput = true;
    }

    public static interface RootLiveOutputFilter {
        public void onOutputLine(RootCommandParams params,
                CommandResult result, String line);
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

                Command command = new Command(0, mParams.command) {
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
                    }

                    @Override
                    public void commandCompleted(int id, int exitCode) {
                        mResult.exitCode = exitCode;
                        synchronized (RootCommandRunner.this) {
                            RootCommandRunner.this.notify();
                        }
                    }

                    @Override
                    public void commandTerminated(int id, String reason) {
                        // Hope this never happens
                        mResult.exitCode = -1;
                        synchronized (RootCommandRunner.this) {
                            RootCommandRunner.this.notify();
                        }
                    }
                };

                RootTools.getShell(true).add(command);

                synchronized (this) {
                    wait();
                }

                // TODO: Should this be done on another thread?
                if (mParams.listener != null) {
                    mParams.listener.onCommandCompletion(mResult);
                }
            } catch (IOException e) {
                e.printStackTrace();
            } catch (InterruptedException e) {
                e.printStackTrace();
            } catch (TimeoutException e) {
                e.printStackTrace();
            } catch (RootDeniedException e) {
                e.printStackTrace();
            }
        }
    }

    public static void waitForCommand(CommandRunner cmd) {
        try {
            cmd.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    public static void waitForRootCommand(RootCommandRunner cmd) {
        try {
            cmd.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
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

            return result.exitCode;
        } catch (Exception e) {
            e.printStackTrace();
        }

        return -1;
    }

    public static class FullRootOutputListener implements RootCommandListener {
        private final StringBuilder mOutput = new StringBuilder();

        @Override
        public void onNewOutputLine(String line) {
            mOutput.append(line);
            mOutput.append(System.getProperty("line.separator"));
        }

        @Override
        public void onCommandCompletion(CommandResult result) {
            result.data.putString("output", mOutput.toString());
        }
    }

    public static int getUid(Context context) {
        int uid = 0;

        try {
            uid = context.getPackageManager().getApplicationInfo(context.getPackageName(), 0).uid;
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }

        if (uid == 0) {
            Log.e(TAG, "Couldn't determine UID");
            return -1;
        }

        return uid;
    }

    private static class FirstLineListener implements CommandListener {
        private String mLine;

        @Override
        public void onNewOutputLine(String line, String stream) {
            if (stream.equals(STREAM_STDOUT)) {
                if (mLine == null) {
                    mLine = line;
                }
            }
        }

        @Override
        public void onCommandCompletion(CommandResult result) {
        }

        public String getLine() {
            return mLine;
        }
    }
}
