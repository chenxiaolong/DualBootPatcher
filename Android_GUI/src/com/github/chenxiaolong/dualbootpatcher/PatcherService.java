package com.github.chenxiaolong.dualbootpatcher;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.StringReader;
import java.util.Arrays;
import java.util.Properties;

import android.app.IntentService;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.ConditionVariable;
import android.util.Log;

public class PatcherService extends IntentService {
    private static final String TAG = "PatcherService";
    public static final String BROADCAST_INTENT = "com.chenxiaolong.github.dualbootpatcher.BROADCAST_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_PATCH_FILE = "patch_file";
    public static final String ACTION_FETCH_PART_CONFIGS = "fetch_part_configs";
    public static final String ACTION_UPDATE_PATCHER = "update_patcher";

    public static final String STATE = "state";
    public static final String STATE_ADD_TASK = "add_task";
    public static final String STATE_UPDATE_TASK = "update_task";
    public static final String STATE_UPDATE_DETAILS = "update_details";
    public static final String STATE_NEW_OUTPUT_LINE = "new_output_line";
    public static final String STATE_SET_PROGRESS_MAX = "set_progress_max";
    public static final String STATE_SET_PROGRESS = "set_progress";
    public static final String STATE_EXTRACTED_PATCHER = "extracted_patcher";
    public static final String STATE_FETCHED_PART_CONFIGS = "fetched_part_configs";
    public static final String STATE_PATCHED_FILE = "patched_file";

    public PatcherService() {
        super(TAG);
    }

    private void addTask(String task) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_ADD_TASK);
        i.putExtra("task", task);
        sendBroadcast(i);
    }

    private void updateTask(String task) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_TASK);
        i.putExtra("task", task);
        sendBroadcast(i);
    }

    private void updateDetails(String details) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_DETAILS);
        i.putExtra("details", details);
        sendBroadcast(i);
    }

    private void newOutputLine(String line, String stream) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_NEW_OUTPUT_LINE);
        i.putExtra("line", line);
        i.putExtra("stream", stream);
        sendBroadcast(i);
    }

    private void setProgressMax(int max) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SET_PROGRESS_MAX);
        i.putExtra("value", max);
        sendBroadcast(i);
    }

    private void setProgress(int value) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SET_PROGRESS);
        i.putExtra("value", value);
        sendBroadcast(i);
    }

    private void onExtractedPatcher() {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_EXTRACTED_PATCHER);
        sendBroadcast(i);
    }

    private void onFetchedPartConfigs(PartitionConfigInfo info) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_FETCHED_PART_CONFIGS);
        i.putExtra("configs", info.mPartConfigs);
        i.putExtra("names", info.mPartConfigNames);
        i.putExtra("descs", info.mPartConfigDescs);
        sendBroadcast(i);
    }

    private void onPatchedFile(boolean failed, String exitMsg, String newFile) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_PATCHED_FILE);
        i.putExtra("failed", failed);
        i.putExtra("exitMsg", exitMsg);
        i.putExtra("newFile", newFile);
        sendBroadcast(i);
    }

    private void log(String msg) {
        Log.e(TAG, msg);
    }

    private ConditionVariable mExtractPatcherDone = new ConditionVariable(false);

    private static final String mFileBase = "DualBootPatcherAndroid-";
    private static final String mFileExt = ".tar.xz";
    private String mVersion = "";
    private String mDirName = "";
    private String mFileName = "";

    /* Some paths */
    private File tar; // tar binary
    private File target; // patcher's tar.xz in cache directory
    private File targetDir; // directory of extracted patcher

    private boolean mExtractPatcherOnce = false;
    private boolean mGetPathsOnce = false;

    @Override
    public void onCreate() {
        super.onCreate();

        if (!mGetPathsOnce) {
            try {
                mVersion = getPackageManager().getPackageInfo(getPackageName(),
                        0).versionName;
                mFileName = mFileBase + mVersion.replace("-DEBUG", "")
                        + mFileExt;
                mDirName = mFileBase + mVersion.replace("-DEBUG", "");
            } catch (NameNotFoundException e) {
                e.printStackTrace();
            }

            tar = new File(getCacheDir() + "/tar");
            target = new File(getCacheDir() + "/" + mFileName);
            targetDir = new File(getFilesDir() + "/" + mDirName);

            mGetPathsOnce = true;
        }
    }

    private void extractPatcher() {
        if (!mExtractPatcherOnce) {
            mExtractPatcherOnce = true;
            mExtractPatcherDone.close();

            try {
                ExtractPatcher task = new ExtractPatcher();
                task.start();
                task.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private void fetchPartConfigs() {
        try {
            GetPartitionConfigs task = new GetPartitionConfigs();
            task.start();
            task.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void patchFile(String zipFile, String partConfig) {
        try {
            ExtractPatcher extractTask = new ExtractPatcher();
            extractTask.start();
            extractTask.join();

            PatchFile task = new PatchFile(zipFile, partConfig);
            task.start();
            task.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getExtras().getString(ACTION);

        if (action.equals(ACTION_UPDATE_PATCHER)) {
            extractPatcher();
        } else if (action.equals(ACTION_FETCH_PART_CONFIGS)) {
            extractPatcher();
            fetchPartConfigs();
        } else if (action.equals(ACTION_PATCH_FILE)) {
            String zipFile = intent.getExtras().getString("zipFile");
            String partConfig = intent.getExtras().getString("partConfig");

            extractPatcher();
            patchFile(zipFile, partConfig);
        }
    }

    public class PartitionConfigInfo {
        public String[] mPartConfigs;
        public String[] mPartConfigNames;
        public String[] mPartConfigDescs;
    }

    private class GetPartitionConfigs extends Thread implements CommandListener {
        private PartitionConfigInfo mInfo = new PartitionConfigInfo();
        private StringBuilder mOutput = new StringBuilder();

        @Override
        public void run() {
            // Patcher should have been extracted already

            mExtractPatcherDone.block();

            CommandParams params = new CommandParams();
            params.listener = this;
            CommandRunner cmd;

            params.command = new String[] { "pythonportable/bin/python3", "-B",
                    "scripts/listpartitionconfigs.py" };
            params.environment = new String[] {
                    "PYTHONUNBUFFERED=true" };
            params.cwd = targetDir;

            cmd = new CommandRunner(params);
            cmd.start();

            try {
                cmd.join();

                Properties prop = new Properties();

                prop.load(new StringReader(mOutput.toString()));

                Log.i("DualBootPatcher", "Got from listpartitionconfigs.py: "
                        + prop.stringPropertyNames().toString());

                mInfo.mPartConfigs = prop.getProperty("partitionconfigs")
                        .split(",");
                mInfo.mPartConfigNames = new String[mInfo.mPartConfigs.length];
                mInfo.mPartConfigDescs = new String[mInfo.mPartConfigs.length];
                for (int i = 0; i < mInfo.mPartConfigs.length; i++) {
                    mInfo.mPartConfigNames[i] = prop
                            .getProperty(mInfo.mPartConfigs[i] + ".name");
                    mInfo.mPartConfigDescs[i] = prop
                            .getProperty(mInfo.mPartConfigs[i] + ".description");

                    Log.i("DualBootPatcher", mInfo.mPartConfigs[i]);
                    Log.i("DualBootPatcher", " --> "
                            + mInfo.mPartConfigNames[i]);
                    Log.i("DualBootPatcher", " --> "
                            + mInfo.mPartConfigDescs[i]);
                }

                onFetchedPartConfigs(mInfo);
            } catch (InterruptedException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onNewOutputLine(String line, String stream) {
            if (stream.equals("stdout")) {
                mOutput.append(line);
                mOutput.append(System.getProperty("line.separator"));
            }
        }

        @Override
        public void onCommandCompletion(CommandResult result) {
        }
    }

    private class PatchFile extends Thread implements CommandListener {
        private String mZipFile = "";
        private String mPartConfig = "";

        public PatchFile(String zipFile, String partConfig) {
            super();
            mZipFile = zipFile;
            mPartConfig = partConfig;
        }

        @Override
        public void run() {
            mExtractPatcherDone.block();

            updateTask(getString(R.string.task_patching_files));

            CommandParams params = new CommandParams();
            params.listener = this;
            CommandRunner cmd;

            // Make tar binary executable
            params.command = new String[] { "pythonportable/bin/python3", "-B",
                    "scripts/patchfile.py", mZipFile, mPartConfig };
            params.environment = new String[] {
                    "PYTHONUNBUFFERED=true", "TMPDIR=" + getCacheDir() };
            params.cwd = targetDir;

            cmd = new CommandRunner(params);
            cmd.start();
            final CommandResult result;

            try {
                cmd.join();
                result = cmd.getResult();

                final String newFile = mZipFile.replace(".zip", "_"
                        + mPartConfig + ".zip");

                onPatchedFile(result.failed, result.exitMsg, newFile);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        @Override
        public void onNewOutputLine(String line, String stream) {
            if (line.contains("ADDTASK:")) {
                addTask(line.replace("ADDTASK:", ""));
            } else if (line.contains("SETTASK:")) {
                updateTask(line.replace("SETTASK:", ""));
            } else if (line.contains("SETDETAILS:")) {
                updateDetails(line.replace("SETDETAILS:", ""));
            } else if (line.contains("SETMAXPROGRESS:")) {
                setProgressMax(Integer.parseInt(line.replace("SETMAXPROGRESS:",
                        "")));
            } else if (line.contains("SETPROGRESS:")) {
                setProgress(Integer.parseInt(line.replace("SETPROGRESS:", "")));
            } else {
                newOutputLine(line, stream);
            }
        }

        @Override
        public void onCommandCompletion(CommandResult result) {
        }
    }

    private class ExtractPatcher extends Thread implements CommandListener {
        @Override
        public void run() {
            updateTask(getString(R.string.task_updating_patcher));

            /*
             * Remove temporary files in case the script crashes and doesn't
             * clean itself up properly - cacheDir|* - filesDir|*|tmp*
             */
            for (File d : getCacheDir().listFiles()) {
                delete(d);
            }
            for (File d : getFilesDir().listFiles()) {
                if (d.isDirectory()) {
                    for (File t : d.listFiles()) {
                        if (t.getName().contains("tmp")) {
                            delete(t);
                        }
                    }
                }
            }

            if (!targetDir.exists()) {
                extractAsset("tar", tar);
                extractAsset(mFileName, target);

                // Remove all previous files
                for (File d : getFilesDir().listFiles()) {
                    delete(d);
                }

                CommandParams params = new CommandParams();
                params.listener = this;
                CommandRunner cmd;

                // Make tar executable
                params.command = new String[] { "chmod", "755", tar.getPath() };
                params.environment = null;
                params.cwd = null;

                cmd = new CommandRunner(params);
                cmd.start();
                waitForCommand(cmd);

                // Extract patcher
                params.command = new String[] { tar.getPath(), "-J", "-x",
                        "-v", "-f", target.getPath() };
                params.environment = null;
                params.cwd = getFilesDir();

                cmd = new CommandRunner(params);
                cmd.start();
                waitForCommand(cmd);

                // Make Python executable
                params.command = new String[] { "chmod", "755",
                        "pythonportable/bin/python3" };
                params.environment = null;
                params.cwd = targetDir;

                cmd = new CommandRunner(params);
                cmd.start();
                waitForCommand(cmd);

                // Delete archive and tar binary
                target.delete();
                tar.delete();
            }

            onExtractedPatcher();

            mExtractPatcherDone.open();
        }

        @Override
        public void onNewOutputLine(String line, String stream) {
            newOutputLine(line, stream);
        }

        @Override
        public void onCommandCompletion(CommandResult result) {
        }

        public void delete(File f) {
            if (f.isFile()) {
                updateDetails(String.format(
                        getString(R.string.details_deteting), f.getName()));
                f.delete();
            } else if (f.isDirectory()) {
                for (File f2 : f.listFiles()) {
                    delete(f2);
                }
                updateDetails(String.format(
                        getString(R.string.details_deteting), f.getName()));
                f.delete();
            }
        }

        public void extractAsset(String src, File dest) {
            try {
                InputStream i = getAssets().open(src);
                FileOutputStream o = new FileOutputStream(dest);

                // byte[] buffer = new byte[4096];
                // int length;
                // while ((length = i.read(buffer)) > 0) {
                // o.write(buffer, 0, length);
                // }

                int length = i.available();
                byte[] buffer = new byte[length];
                i.read(buffer);
                o.write(buffer);

                o.flush();
                o.close();
                i.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        private void waitForCommand(CommandRunner cmd) {
            try {
                cmd.join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    private interface CommandListener {
        public void onNewOutputLine(String line, String stream);

        public void onCommandCompletion(CommandResult result);
    }

    private class CommandParams {
        public String[] command;
        public String[] environment;
        public File cwd;
        public CommandListener listener;
    }

    private class CommandResult {
        public int exitCode;
        public String exitMsg;
        public boolean failed;
    }

    private class CommandRunner extends Thread {
        private BufferedReader stdout = null;
        private BufferedReader stderr = null;

        private CommandParams mParams;
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
                log("Command: " + Arrays.toString(mParams.command));

                ProcessBuilder pb = new ProcessBuilder(
                        Arrays.asList(mParams.command));

                if (mParams.environment != null) {
                    log("Environment: " + Arrays.toString(mParams.environment));

                    for (String s : mParams.environment) {
                        String[] split = s.split("=");
                        pb.environment().put(split[0], split[1]);
                    }
                } else {
                    log("Environment: Inherited");
                }

                if (mParams.cwd != null) {
                    log("Working directory: " + mParams.cwd);

                    pb.directory(mParams.cwd);
                } else {
                    log("Working directory: Inherited");
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
                                log("Standard output: " + s);

                                if (mParams.listener != null) {
                                    mParams.listener.onNewOutputLine(s,
                                            "stdout");
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
                        mResult.failed = false;
                        mResult.exitMsg = "";

                        String s;
                        try {
                            while ((s = stderr.readLine()) != null) {
                                log("Standard error: " + s);

                                if (s.contains("EXITFAIL:")) {
                                    mResult.exitMsg = s
                                            .replace("EXITFAIL:", "");
                                    mResult.failed = true;
                                } else if (s.contains("EXITSUCCESS:")) {
                                    mResult.exitMsg = s.replace("EXITSUCCESS:",
                                            "");
                                    mResult.failed = false;
                                }

                                if (mParams.listener != null) {
                                    mParams.listener.onNewOutputLine(s,
                                            "stderr");
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
}
