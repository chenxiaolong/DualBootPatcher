package com.github.chenxiaolong.dualbootpatcher.switcher;

import java.io.File;

import android.app.FragmentManager;
import android.content.Context;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootCommandRunner;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;

public class SwitcherUtils {
    public static final String TAG = "SwitcherUtils";
    public static final String BOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/boot";
    // Can't use Environment.getExternalStorageDirectory() because the path is
    // different in the root environment
    public static final String KERNEL_PATH_ROOT = "/raw-data/media/0/MultiKernels";

    private static int runRootCommand(String command) {
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

    public static void writeKernel(String rom) throws Exception {
        String rom_id = RomDetector.getId(rom);

        String[] paths = new String[] { KERNEL_PATH_ROOT,
                KERNEL_PATH_ROOT.replace("raw-data", "data"),
                "/raw-system/dual-kernels", "/system/dual-kernels" };

        String kernel = null;
        for (String path : paths) {
            String temp = path + File.separator + rom_id + ".img";
            if (!FileUtils.isExistsDirectory(temp)) {
                Log.e(TAG, temp + " not found");
                continue;
            }
            kernel = temp;
            break;
        }

        if (kernel == null) {
            throw new Exception("The kernel for " + rom + " was not found");
        }

        int exitCode = runRootCommand("dd if=" + kernel + " of=" + BOOT_PARTITION);
        if (exitCode != 0) {
            Log.e(TAG, "Failed to write " + kernel + " with dd");
            throw new Exception("Failed to write " + kernel + " with dd");
        }
    }

    public static void backupKernel(String rom) throws Exception {
        String rom_id = RomDetector.getId(rom);

        String kernel_path = KERNEL_PATH_ROOT;
        if (!FileUtils.isExistsDirectory("/raw-data")) {
            kernel_path = kernel_path.replace("raw-data", "data");
        }

        FileUtils.makedirs(kernel_path);
        String kernel = kernel_path + File.separator + rom_id + ".img";

        Log.v(TAG, "Backing up " + rom_id + " kernel");

        int exitCode = runRootCommand("dd if=" + BOOT_PARTITION + " of=" + kernel);

        if (exitCode != 0) {
            Log.e(TAG, "Failed to backup to " + kernel + " with dd");
            throw new Exception("Failed to backup to " + kernel + " with dd");
        }

        Log.v(TAG, "Fixing permissions");
        runRootCommand("chmod -R 775 " + kernel_path);
        runRootCommand("chown -R media_rw:media_rw " + kernel_path);
    }

    public static void reboot(Context context, FragmentManager fm) {
        runRootCommand("reboot");
    }
}
