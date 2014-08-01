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
import android.content.pm.PackageManager.NameNotFoundException;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RootFile;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;

import java.io.File;

public class SwitcherUtils {
    public static final String TAG = SwitcherUtils.class.getSimpleName();
    public static final String BOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/boot";
    // Can't use Environment.getExternalStorageDirectory() because the path is
    // different in the root environment
    public static final String KERNEL_PATH_ROOT = "/media/0/MultiKernels";
    private static final String MOUNT_POINT = "/data/local/tmp/busybox";
    private static final String TMP_KERNEL = "/data/local/tmp/tmp.dbp";

    private static int dd(String source, String dest) {
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
            Log.e(TAG, "Failed to write " + kernel + " with dd");
            throw new Exception("Failed to write " + kernel + " with dd");
        }
    }

    public static void backupKernel(Context context, String kernelId) throws Exception {
        String kernel_path = RomUtils.RAW_DATA + KERNEL_PATH_ROOT;
        if (!new RootFile(RomUtils.RAW_DATA).isDirectory()) {
            kernel_path = RomUtils.DATA + KERNEL_PATH_ROOT;
        }

        // Copy kernel to a temporary location
        RootFile tmpdir = new RootFile(TMP_KERNEL);
        tmpdir.mkdirs();

        String tmpKernel = TMP_KERNEL + File.separator + kernelId + ".img";
        int ret = dd(BOOT_PARTITION, tmpKernel);
        if (ret != 0) {
            String msg = "Failed to backup to " + tmpKernel + " with dd";
            Log.e(TAG, msg);
            throw new Exception(msg);
        }
        tmpdir.recursiveChmod(0777);

        // Update syncdaemon in the boot image
        if (!PatcherUtils.updateSyncdaemon(context, tmpKernel)) {
            String msg = "Failed to update syncdaemon";
            Log.e(TAG, msg);
            throw new Exception(msg);
        }

        // Copy to target
        String updatedKernel = tmpKernel.replace(".img", "_syncdaemon.img");
        String targetKernel = kernel_path + File.separator + kernelId + ".img";

        RootFile f = new RootFile(kernel_path);
        f.mkdirs();

        new RootFile(updatedKernel).copyTo(new RootFile(targetKernel));

        Log.v(TAG, "Fixing permissions");
        f.recursiveChmod(0775);
        f.recursiveChown("media_rw", "media_rw");

        // Write updated image back to boot partition
        ret = dd(targetKernel, BOOT_PARTITION);
        if (ret != 0) {
            String msg = "Failed to backup to " + targetKernel + " with dd";
            Log.e(TAG, msg);
            throw new Exception(msg);
        }

        // Remove temporary directory
        tmpdir.recursiveDelete();
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
        int uid = 0;

        try {
            uid = context.getPackageManager().getApplicationInfo(
                    context.getPackageName(), 0).uid;
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }

        if (uid == 0) {
            Log.e(TAG, "Couldn't determine UID");
            return -1;
        }

        // Delete old versions
        FileUtils.deleteOldCachedAsset(context, "busybox-static");
        String busybox = FileUtils.extractVersionedAssetToCache(context, "busybox-static");

        // Clean up in case something crashed before
        final String busyboxBinary = MOUNT_POINT + File.separator + "busybox";

        RootFile mountPoint = new RootFile(MOUNT_POINT);
        mountPoint.mountTmpFs();
        mountPoint.chown(uid, uid);

        RootFile busyboxFile = new RootFile(busyboxBinary);
        if (busyboxFile.isFile()) {
            busyboxFile.delete();
        }

        CommandUtils.runRootCommand("cp " + busybox + " " + busyboxBinary);
        new RootFile(busyboxBinary).chmod(0755);
        CommandUtils.runRootCommand("chcon u:object_r:system_file:s0 " + busyboxBinary);

        int exitCode = CommandUtils.runRootCommand(busyboxBinary + " reboot");

        mountPoint.unmountTmpFs();
        mountPoint.recursiveDelete();

        return exitCode;
    }
}
