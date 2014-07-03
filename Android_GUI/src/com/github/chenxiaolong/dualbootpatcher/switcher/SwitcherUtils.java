/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;

import java.io.File;

public class SwitcherUtils {
    public static final String TAG = "SwitcherUtils";
    public static final String BOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/boot";
    // Can't use Environment.getExternalStorageDirectory() because the path is
    // different in the root environment
    public static final String KERNEL_PATH_ROOT = "/raw-data/media/0/MultiKernels";

    public static void writeKernel(String kernelId) throws Exception {
        String[] paths = new String[] { KERNEL_PATH_ROOT,
                KERNEL_PATH_ROOT.replace("raw-data", "data"),
                "/raw-system/dual-kernels", "/system/dual-kernels" };

        String kernel = null;
        for (String path : paths) {
            String temp = path + File.separator + kernelId + ".img";
            if (!FileUtils.isExistsFile(temp)) {
                Log.e(TAG, temp + " not found");
                continue;
            }
            kernel = temp;
            break;
        }

        if (kernel == null) {
            throw new Exception("The kernel for " + kernelId + " was not found");
        }

        int exitCode = CommandUtils.runRootCommand("dd if=" + kernel + " of=" + BOOT_PARTITION);
        if (exitCode != 0) {
            Log.e(TAG, "Failed to write " + kernel + " with dd");
            throw new Exception("Failed to write " + kernel + " with dd");
        }
    }

    public static void backupKernel(String kernelId) throws Exception {
        String kernel_path = KERNEL_PATH_ROOT;
        if (!FileUtils.isExistsDirectory("/raw-data")) {
            kernel_path = kernel_path.replace("raw-data", "data");
        }

        FileUtils.makedirs(kernel_path);
        String kernel = kernel_path + File.separator + kernelId + ".img";

        Log.v(TAG, "Backing up " + kernelId + " kernel");

        int exitCode = CommandUtils.runRootCommand("dd if=" + BOOT_PARTITION + " of=" + kernel);

        if (exitCode != 0) {
            Log.e(TAG, "Failed to backup to " + kernel + " with dd");
            throw new Exception("Failed to backup to " + kernel + " with dd");
        }

        Log.v(TAG, "Fixing permissions");
        CommandUtils.runRootCommand("chmod -R 775 " + kernel_path);
        CommandUtils.runRootCommand("chown -R media_rw:media_rw " + kernel_path);
    }

    public static void reboot() {
        CommandUtils.runRootCommand("reboot");
    }
}
