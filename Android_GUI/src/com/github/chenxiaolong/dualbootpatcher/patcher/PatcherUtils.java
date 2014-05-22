package com.github.chenxiaolong.dualbootpatcher.patcher;

import java.io.File;
import java.util.ArrayList;

import org.json.JSONException;

import android.content.Context;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandListener;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandRunner;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.LiveOutputFilter;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation.FileInfo;

public class PatcherUtils {
    public static final String TAG = "PatcherUtils";
    private static final String FILENAME = "DualBootPatcherAndroid-%s.tar.xz";
    private static final String DIRNAME = "DualBootPatcherAndroid-%s";

    public static final String PARAM_FILENAME = "filename";
    public static final String PARAM_SUPPORTED = "supported";
    public static final String PARAM_DEVICE = "device";
    public static final String PARAM_PARTCONFIG = "partconfig";
    public static final String PARAM_PRESET = "preset";
    public static final String PARAM_USE_AUTOPATCHER = "use_autopatcher";
    public static final String PARAM_AUTOPATCHER = "autopatcher";
    public static final String PARAM_USE_PATCH = "use_patch";
    public static final String PARAM_PATCH = "patch";
    public static final String PARAM_DEVICE_CHECK = "device_check";
    public static final String PARAM_HAS_BOOT_IMAGE = "has_boot_image";
    public static final String PARAM_LOKI = "loki";
    public static final String PARAM_RAMDISK = "ramdisk";
    public static final String PARAM_PATCHED_INIT = "patched_init";
    public static final String PARAM_BOOT_IMAGE = "boot_image";

    public static final String RESULT_PATCH_FILE_NEW_FILE = "new_file";
    public static final String RESULT_PATCH_FILE_MESSAGE = "message";
    public static final String RESULT_PATCH_FILE_FAILED = "failed";

    private static String mVersion;

    private static PatcherInformation mInfo;

    /** Filename of the tar.xz archive */
    private static String mFileName;

    /** Tar binary in the cache directory */
    private static File mTar;

    /** Patcher's tar.xz archive in the cache directory */
    private static File mTargetFile;

    /** Directory of extracted patcher */
    private static File mTargetDir;

    private static String getVersion(Context context) {
        if (mVersion == null) {
            try {
                mVersion = context.getPackageManager().getPackageInfo(
                        context.getPackageName(), 0).versionName;
                mVersion = mVersion.replace("-DEBUG", "");
                mVersion = mVersion.replace("-SNAPSHOT", "");
            } catch (NameNotFoundException e) {
                e.printStackTrace();
            }
        }

        return mVersion;
    }

    private static String getFilename(Context context) {
        if (mFileName == null) {
            mFileName = String.format(FILENAME, getVersion(context));
        }

        return mFileName;
    }

    private static File getTarBinary(Context context) {
        if (mTar == null) {
            mTar = new File(context.getCacheDir() + "/tar");
        }

        return mTar;
    }

    private static File getTargetFile(Context context) {
        if (mTargetFile == null) {
            String fileName = getFilename(context);
            mTargetFile = new File(context.getCacheDir() + "/" + fileName);
        }

        return mTargetFile;
    }

    static File getTargetDirectory(Context context) {
        if (mTargetDir == null) {
            String dirName = String.format(DIRNAME, getVersion(context));
            mTargetDir = new File(context.getFilesDir() + "/" + dirName);
        }

        return mTargetDir;
    }

    private static class FullOutputListener implements CommandListener {
        private final StringBuilder mOutput = new StringBuilder();

        @Override
        public void onNewOutputLine(String line, String stream) {
            if (stream.equals(CommandUtils.STREAM_STDOUT)) {
                mOutput.append(line);
                mOutput.append(System.getProperty("line.separator"));
            }
        }

        @Override
        public void onCommandCompletion(CommandResult result) {
        }

        public String getOutput() {
            return mOutput.toString();
        }
    }

    private static class PatcherOutputFilter implements LiveOutputFilter {
        public static final String RESULT_MESSAGE = "message";
        public static final String RESULT_FAILED = "failed";

        @Override
        public void onStdoutLine(CommandParams params, CommandResult result,
                String line) {
        }

        @Override
        public void onStderrLine(CommandParams params, CommandResult result,
                String line) {
            if (line.contains("EXITFAIL:")) {
                String message = line.replace("EXITFAIL:", "");

                result.data.putString(RESULT_MESSAGE, message);
                result.data.putBoolean(RESULT_FAILED, true);
            } else if (line.contains("EXITSUCCESS:")) {
                String message = line.replace("EXITSUCCESS:", "");

                result.data.putString(RESULT_MESSAGE, message);
                result.data.putBoolean(RESULT_FAILED, false);
            }
        }
    }

    public synchronized static Bundle patchFile(Context context,
            CommandListener listener, Bundle data) {
        // Make sure patcher is extracted first
        extractPatcher(context);

        // Setup command
        ArrayList<String> args = new ArrayList<String>();
        args.add("pythonportable/bin/python3");
        args.add("-B");
        args.add("scripts/patchfile.py");
        args.add("--noquestions");

        // Filename
        String filename = data.getString(PARAM_FILENAME);
        args.add(filename);

        // Device
        String device = data.getString(PARAM_DEVICE);
        args.add("--device");
        args.add(device);

        // Partition configuration
        String partConfig = data.getString(PARAM_PARTCONFIG);
        args.add("--partconfig");
        args.add(partConfig);

        // Whether the file is supported
        boolean supported = data.getBoolean(PARAM_SUPPORTED);
        if (!supported) {
            args.add("--unsupported");

            FileInfo preset = data.getParcelable(PARAM_PRESET);
            if (preset != null) {
                args.add("--preset");
                args.add(preset.mPath);
            } else {
                boolean useAutopatcher = data.getBoolean(PARAM_USE_AUTOPATCHER);
                if (useAutopatcher) {
                    String autopatcher = data.getString(PARAM_AUTOPATCHER);

                    args.add("--autopatcher");
                    args.add(autopatcher);
                }

                boolean usePatch = data.getBoolean(PARAM_USE_PATCH);
                if (usePatch) {
                    String patch = data.getString(PARAM_PATCH);

                    args.add("--patch");
                    args.add(patch);
                }

                boolean hasBootImage = data.getBoolean(PARAM_HAS_BOOT_IMAGE);
                if (!hasBootImage) {
                    args.add("--nobootimage");
                } else {
                    args.add("--hasbootimage");

                    boolean loki = data.getBoolean(PARAM_LOKI);
                    if (loki) {
                        args.add("--loki");
                    } else {
                        args.add("--no-loki");
                    }

                    String ramdisk = data.getString(PARAM_RAMDISK);
                    args.add("--ramdisk");
                    args.add(ramdisk);

                    String init = data.getString(PARAM_PATCHED_INIT);
                    if (init != null) {
                        args.add("--patchedinit");
                        args.add(init);
                    }
                }
            }

            boolean deviceCheck = data.getBoolean(PARAM_DEVICE_CHECK);
            if (!deviceCheck) {
                args.add("--nodevicecheck");
            }
        }

        File targetDir = getTargetDirectory(context);

        CommandParams params = new CommandParams();
        params.listener = listener;
        params.filter = new PatcherOutputFilter();
        CommandRunner cmd;

        params.command = args.toArray(new String[0]);
        params.environment = new String[] { "PYTHONUNBUFFERED=true",
                "TMPDIR=" + context.getCacheDir() };
        params.cwd = targetDir;

        cmd = new CommandRunner(params);
        cmd.start();
        final CommandResult result;

        try {
            cmd.join();
            result = cmd.getResult();

            // TODO: Fix support for .img and .lok files
            final String newFile = filename.replace(".zip", "_" + partConfig
                    + ".zip");

            String message = result.data.getString(
                    PatcherOutputFilter.RESULT_MESSAGE, "");
            boolean failed = result.data.getBoolean(
                    PatcherOutputFilter.RESULT_FAILED, true);

            Bundle ret = new Bundle();
            ret.putString(RESULT_PATCH_FILE_NEW_FILE, newFile);
            ret.putString(RESULT_PATCH_FILE_MESSAGE, message);
            ret.putBoolean(RESULT_PATCH_FILE_FAILED, failed);
            return ret;
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        return null;
    }

    public synchronized static boolean isFileSupported(Context context,
            Bundle data) {
        // Make sure patcher is extracted first
        // extractPatcher(context);

        // Setup command
        ArrayList<String> args = new ArrayList<String>();
        args.add("pythonportable/bin/python3");
        args.add("-B");
        args.add("scripts/patchfile.py");
        args.add("--is-supported");

        // Filename
        String filename = data.getString(PARAM_FILENAME);
        args.add(filename);

        // Device
        String device = data.getString(PARAM_DEVICE);
        args.add("--device");
        args.add(device);

        // Partition configuration
        String partConfig = data.getString(PARAM_PARTCONFIG);
        args.add("--partconfig");
        args.add(partConfig);

        File targetDir = getTargetDirectory(context);

        FullOutputListener listener = new FullOutputListener();

        CommandParams params = new CommandParams();
        params.listener = listener;
        CommandRunner cmd;

        params.command = args.toArray(new String[0]);
        params.environment = new String[] { "PYTHONUNBUFFERED=true" };
        params.cwd = targetDir;

        cmd = new CommandRunner(params);
        cmd.start();

        try {
            cmd.join();

            String output = listener.getOutput();

            return output.startsWith("supported");
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        // Uh-oh
        return false;
    }

    public synchronized static PatcherInformation getPatcherInformation(
            Context context) {
        if (mInfo == null) {
            // Make sure patcher is extracted first
            extractPatcher(context);

            File targetDir = getTargetDirectory(context);

            FullOutputListener listener = new FullOutputListener();

            CommandParams params = new CommandParams();
            params.listener = listener;
            CommandRunner cmd;

            params.command = new String[] { "pythonportable/bin/python3", "-B",
                    "scripts/jsondump.py" };
            params.environment = new String[] { "PYTHONUNBUFFERED=true" };
            params.cwd = targetDir;
            params.logStdout = false;

            cmd = new CommandRunner(params);
            cmd.start();

            try {
                cmd.join();
                PatcherInformation info = new PatcherInformation();
                info.loadJson(listener.getOutput());
                mInfo = info;
                return mInfo;
            } catch (InterruptedException e) {
                e.printStackTrace();
            } catch (JSONException e) {
                e.printStackTrace();
            }

            // Hopefully we never hit this
            return null;
        }

        return mInfo;
    }

    private synchronized static void extractPatcher(Context context) {
        /*
         * Remove temporary files in case the script crashes and doesn't clean
         * itself up properly - cacheDir|* - filesDir|*|tmp*
         */
        for (File d : context.getCacheDir().listFiles()) {
            FileUtils.recursiveDelete(d);
        }
        for (File d : context.getFilesDir().listFiles()) {
            if (d.isDirectory()) {
                for (File t : d.listFiles()) {
                    if (t.getName().contains("tmp")) {
                        FileUtils.recursiveDelete(t);
                    }
                }
            }
        }

        File tar = getTarBinary(context);
        File targetFile = getTargetFile(context);
        File targetDir = getTargetDirectory(context);

        if (!targetDir.exists()) {
            FileUtils.extractAsset(context, "tar", tar);
            FileUtils.extractAsset(context, mFileName, targetFile);

            // Remove all previous files
            for (File d : context.getFilesDir().listFiles()) {
                FileUtils.recursiveDelete(d);
            }

            CommandParams params = new CommandParams();
            CommandRunner cmd;

            // Make tar executable
            params.command = new String[] { "chmod", "755", tar.getPath() };
            params.environment = null;
            params.cwd = null;

            cmd = new CommandRunner(params);
            cmd.start();
            CommandUtils.waitForCommand(cmd);

            // Extract patcher
            params.command = new String[] { tar.getPath(), "-J", "-x", "-v",
                    "-f", targetFile.getPath() };
            params.environment = null;
            params.cwd = context.getFilesDir();

            cmd = new CommandRunner(params);
            cmd.start();
            CommandUtils.waitForCommand(cmd);

            // Make Python executable
            params.command = new String[] { "chmod", "755",
                    "pythonportable/bin/python3" };
            params.environment = null;
            params.cwd = targetDir;

            cmd = new CommandRunner(params);
            cmd.start();
            CommandUtils.waitForCommand(cmd);

            // Delete archive and tar binary
            targetFile.delete();
            tar.delete();
        }
    }
}
