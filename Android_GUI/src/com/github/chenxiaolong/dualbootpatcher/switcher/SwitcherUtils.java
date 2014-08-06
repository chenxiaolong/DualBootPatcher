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

import android.content.Context;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RootFile;

import java.io.File;

public class SwitcherUtils {
    public static final String TAG = SwitcherUtils.class.getSimpleName();
    public static final String BOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/boot";
    public static final String ABOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/aboot";
    // Can't use Environment.getExternalStorageDirectory() because the path is
    // different in the root environment
    public static final String KERNEL_PATH_ROOT = "/media/0/MultiKernels";

    public static int dd(String source, String dest) {
        return CommandUtils.runRootCommand("dd if=" + source + " of=" + dest);
    }

    public static void writeKernel(String kernelId) throws Exception {
        String[] paths = new String[] { RomUtils.RAW_DATA + KERNEL_PATH_ROOT,
                RomUtils.DATA + KERNEL_PATH_ROOT,
                "/raw-system/dual-kernels", "/system/dual-kernels" };

        String kernel = null;
        for (String path : paths) {
            String temp = path + File.separator + kernelId + ".img";
            if (!new RootFile(temp).isFile()) {
                Log.e(TAG, temp + " not found");
                continue;
            }
            kernel = temp;
            break;
        }

        if (kernel == null) {
            throw new Exception("The kernel for " + kernelId + " was not found");
        }

        int ret = dd(kernel, BOOT_PARTITION);
        if (ret != 0) {
            throw new Exception("Failed to write " + kernel + " with dd");
        }
    }

    public static void backupKernel(String kernelId) throws Exception {
        String kernelPath = RomUtils.RAW_DATA + KERNEL_PATH_ROOT;
        if (!new RootFile(RomUtils.RAW_DATA).isDirectory()) {
            kernelPath = RomUtils.DATA + KERNEL_PATH_ROOT;
        }

        RootFile f = new RootFile(kernelPath);
        f.mkdirs();

        String targetKernel = kernelPath + File.separator + kernelId + ".img";

        if (dd(BOOT_PARTITION, targetKernel) != 0) {
            throw new Exception("Failed to backup to " + targetKernel + " with dd");
        }

        Log.v(TAG, "Fixing permissions");
        f.recursiveChmod(0775);
        f.recursiveChown("media_rw", "media_rw");
    }

    public static void reboot(final Context context) {
        new Thread() {
            @Override
            public void run() {
                runReboot(context);
            }
        }.start();
    }

    private static int runReboot(Context context) {
        String busybox = CommandUtils.mountBusyboxTmpfs(context);

        int ret = CommandUtils.runRootCommand(busybox + " reboot");

        CommandUtils.unmountBusyboxTmpfs();

        return ret;
    }
}
