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

import android.os.Build;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RootFile;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatcherConfig;

import java.io.File;

public class SwitcherUtils {
    public static final String TAG = SwitcherUtils.class.getSimpleName();
    public static final String BOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/boot";
    public static final String ABOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/aboot";
    // Can't use Environment.getExternalStorageDirectory() because the path is
    // different in the root environment
    public static final String KERNEL_PATH_ROOT = "/data/media/0/MultiBoot/%s/boot.img";

    public static int dd(String source, String dest) {
        return CommandUtils.runRootCommand("dd if=" + source + " of=" + dest);
    }

    private static String getBootPartition() {
        String bootBlockDev = null;

        PatcherConfig pc = new PatcherConfig();
        for (Device d : pc.getDevices()) {
            if (Build.DEVICE.contains(d.getCodename())) {
                String[] bootBlockDevs = d.getBootBlockDevs();
                if (bootBlockDevs.length > 0) {
                    bootBlockDev = bootBlockDevs[0];
                }
                break;
            }
        }
        pc.destroy();

        return bootBlockDev;
    }

    public static void writeKernel(String id) throws Exception {
        String kernel = String.format(KERNEL_PATH_ROOT, id);
        if (!new RootFile(kernel).isFile()) {
            Log.e(TAG, kernel + " not found");
            throw new Exception("The kernel for " + id + " was not found");
        }

        if (dd(kernel, getBootPartition()) != 0) {
            throw new Exception("Failed to write " + kernel + " with dd");
        }
    }

    public static void backupKernel(String id) throws Exception {
        String kernel = String.format(KERNEL_PATH_ROOT, id);

        new RootFile(new File(kernel).getParentFile()).mkdirs();

        if (dd(getBootPartition(), kernel) != 0) {
            throw new Exception("Failed to backup to " + kernel + " with dd");
        }

        RootFile f = new RootFile(kernel);

        Log.v(TAG, "Fixing permissions");
        f.chmod(0775);
        f.chown("media_rw", "media_rw");
    }

    public static void reboot() {
        new Thread() {
            @Override
            public void run() {
                runReboot();
            }
        }.start();
    }

    private static int runReboot() {
        return CommandUtils.runRootCommand("reboot");
    }
}
