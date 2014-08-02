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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.content.Context;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.RootFile;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;

public class AppSharingUtils {
    public static final String TAG = AppSharingUtils.class.getSimpleName();

    private static final String SHARE_APPS_PATH = "/data/patcher.share-app";
    private static final String SHARE_PAID_APPS_PATH = "/data/patcher.share-app-asec";

    public static boolean setShareAppsEnabled(boolean enable) {
        if (enable) {
            CommandUtils.runRootCommand("touch " + SHARE_APPS_PATH);
            return isShareAppsEnabled();
        } else {
            CommandUtils.runRootCommand("rm -f " + SHARE_APPS_PATH);
            return !isShareAppsEnabled();
        }
    }

    public static boolean setSharePaidAppsEnabled(boolean enable) {
        if (enable) {
            CommandUtils.runRootCommand("touch " + SHARE_PAID_APPS_PATH);
            return isSharePaidAppsEnabled();
        } else {
            CommandUtils.runRootCommand("rm -f " + SHARE_PAID_APPS_PATH);
            return !isSharePaidAppsEnabled();
        }
    }

    public static boolean isShareAppsEnabled() {
        return new RootFile(SHARE_APPS_PATH).isFile();
    }

    public static boolean isSharePaidAppsEnabled() {
        return new RootFile(SHARE_PAID_APPS_PATH).isFile();
    }

    public static HashMap<RomInformation, ArrayList<String>> getAllApks(Context context) {
        RomInformation[] roms = RomUtils.getRoms(context);

        HashMap<RomInformation, ArrayList<String>> apksMap =
                new HashMap<RomInformation, ArrayList<String>>();

        for (RomInformation rom : roms) {
            ArrayList<String> apks = new ArrayList<String>();
            String[] filenames = new RootFile(rom.data + File.separator + "app").list();

            if (filenames == null || filenames.length == 0) {
                continue;
            }

            for (String filename : filenames) {
                if (filename.endsWith(".apk")) {
                    apks.add(rom.data + File.separator + "app" + File.separator + filename);
                }
            }

            if (apks.size() > 0) {
                apksMap.put(rom, apks);
            }
        }

        return apksMap;
    }

    public static void updateRamdisk(Context context) throws Exception {
        RomInformation romInfo = RomUtils.getCurrentRom(context);
        if (romInfo == null) {
            throw new Exception("Failed to get current ROM");
        }

        String kernelPath = RomUtils.RAW_DATA + SwitcherUtils.KERNEL_PATH_ROOT;
        if (!new RootFile(RomUtils.RAW_DATA).isDirectory()) {
            kernelPath = RomUtils.DATA + SwitcherUtils.KERNEL_PATH_ROOT;
        }
        new RootFile(kernelPath).chmod(0755);

        // Copy the current kernel to a temporary directory
        String bootImage = kernelPath + File.separator + romInfo.kernelId + ".img";
        RootFile bootImageFile = new RootFile(bootImage);

        if (!bootImageFile.isFile()) {
            SwitcherUtils.backupKernel(romInfo.kernelId);
        }

        String tmpKernel = context.getCacheDir() + File.separator + "kernel.img";
        RootFile tmpKernelFile = new RootFile(tmpKernel);
        bootImageFile.copyTo(tmpKernelFile);
        tmpKernelFile.chmod(0666);

        boolean wasLokid = PatcherUtils.isLokiBootImage(context, tmpKernel);

        // Update syncdaemon in the boot image
        if (!PatcherUtils.updateSyncdaemon(context, tmpKernel)) {
            throw new Exception("Failed to update syncdaemon");
        }

        if (wasLokid) {
            String aboot = context.getCacheDir() + File.separator + "aboot.img";
            SwitcherUtils.dd(SwitcherUtils.ABOOT_PARTITION, aboot);
            new RootFile(aboot).chmod(0666);

            String lokiKernel = context.getCacheDir() + File.separator + "kernel.lok";

            if (lokiPatch("boot", aboot, tmpKernel, lokiKernel) != 0) {
                throw new Exception("Failed to loki patch new boot image");
            }

            tmpKernelFile.delete();

            org.apache.commons.io.FileUtils.moveFile(
                    new File(lokiKernel), new File(tmpKernel));
        }

        // Copy to target
        tmpKernelFile.copyTo(bootImageFile);
        bootImageFile.chmod(0755);

        SwitcherUtils.writeKernel(romInfo.kernelId);
    }


    // libloki-jni native library

    static {
        System.loadLibrary("loki-jni");
    }

    private static native int lokiPatch(String partitionLabel, String abootImage,
                                        String inImage, String outImage);

    private static native int lokiFlash(String partitionLabel, String lokiImage);

    private static native int lokiFind(String abootImage);

    private static native int lokiUnlok(String inImage, String outImage);
}
