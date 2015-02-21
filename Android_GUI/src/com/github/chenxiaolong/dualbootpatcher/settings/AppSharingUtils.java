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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.content.Context;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.BootImage;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CpioFile;
import com.github.chenxiaolong.multibootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolSocket;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;

public class AppSharingUtils {
    public static final String TAG = AppSharingUtils.class.getSimpleName();

    private static final String BOOT_IMAGE_PATH =
            Environment.getExternalStorageDirectory() + "/MultiBoot/%s/boot.img";
    private static final String SHARE_APPS_PATH =
            Environment.getExternalStorageDirectory() + "/MultiBoot/%s/share-app";
    private static final String SHARE_PAID_APPS_PATH =
            Environment.getExternalStorageDirectory() + "/MultiBoot/%s/share-app-asec";

    public static boolean setShareAppsEnabled(boolean enable) {
        String path = String.format(SHARE_APPS_PATH, "ID");
        File f = new File(path);
        if (enable) {
            try {
                f.createNewFile();
            } catch (IOException e) {
                e.printStackTrace();
            }
            return isShareAppsEnabled();
        } else {
            f.delete();
            return !isShareAppsEnabled();
        }
    }

    public static boolean setSharePaidAppsEnabled(boolean enable) {
        String path = String.format(SHARE_PAID_APPS_PATH);
        File f = new File(path);
        if (enable) {
            try {
                f.createNewFile();
            } catch (IOException e) {
                e.printStackTrace();
            }
            return isSharePaidAppsEnabled();
        } else {
            f.delete();
            return !isSharePaidAppsEnabled();
        }
    }

    public static boolean isShareAppsEnabled() {
        return new File(SHARE_APPS_PATH).isFile();
    }

    public static boolean isSharePaidAppsEnabled() {
        return new File(SHARE_PAID_APPS_PATH).isFile();
    }

    public static HashMap<RomInformation, ArrayList<String>> getAllApks(Context context) {
        RomInformation[] roms = RomUtils.getRoms(context);

        HashMap<RomInformation, ArrayList<String>> apksMap = new HashMap<>();

        for (RomInformation rom : roms) {
            ArrayList<String> apks = new ArrayList<>();
            String[] filenames = new File(rom.getDataPath() + File.separator + "app").list();

            if (filenames == null || filenames.length == 0) {
                continue;
            }

            for (String filename : filenames) {
                if (filename.endsWith(".apk")) {
                    apks.add(rom.getDataPath() + File.separator + "app" + File.separator + filename);
                }
            }

            if (apks.size() > 0) {
                apksMap.put(rom, apks);
            }
        }

        return apksMap;
    }

    public static boolean updateRamdisk(Context context) {
        PatcherUtils.extractPatcher(context);

        RomInformation romInfo = RomUtils.getCurrentRom(context);
        if (romInfo == null) {
            Log.e(TAG, "Could not determine current ROM");
            return false;
        }

        String bootImage = String.format(BOOT_IMAGE_PATH, romInfo.getId());
        File bootImageFile = new File(bootImage);

        // Make sure the kernel was backed up
        if (!bootImageFile.exists()) {
            if (!SwitcherUtils.setKernel(context, romInfo.getId())) {
                Log.e(TAG, "Failed to backup boot image before modification");
                return false;
            }
        }

        // Create temporary copy of the boot image
        String tmpKernel = context.getCacheDir() + File.separator + "boot.img";
        File tmpKernelFile = new File(tmpKernel);
        try {
            org.apache.commons.io.FileUtils.copyFile(bootImageFile, tmpKernelFile);
        } catch (IOException e) {
            Log.e(TAG, "Failed to copy boot image to temporary file", e);
            return false;
        }

        // Update mbtool in boot image
        BootImage bi = new BootImage();
        if (!bi.load(tmpKernel)) {
            Log.e(TAG, "libmbp error: " + PatcherUtils.getErrorMessage(context, bi.getError()));
            return false;
        }

        CpioFile cpio = new CpioFile();
        if (!cpio.load(bi.getRamdiskImage())) {
            Log.e(TAG, "libmbp error: " + PatcherUtils.getErrorMessage(context, cpio.getError()));
            return false;
        }

        // Remove old mbtool
        if (cpio.isExists("mbtool")) {
            cpio.remove("mbtool");
        }

        // Add new copy of mbtool
        String abi;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            abi = Build.SUPPORTED_ABIS[0];
        } else {
            abi = Build.CPU_ABI;
        }
        String mbtool = PatcherUtils.getTargetDirectory(context)
                + "/binaries/android/" + abi + "/mbtool";

        if (!cpio.addFile(mbtool, "mbtool", 0755)) {
            Log.e(TAG, "libmbp error: " + PatcherUtils.getErrorMessage(context, cpio.getError()));
            return false;
        }

        // Create new ramdisk
        bi.setRamdiskImage(cpio.createData());

        // Create new boot image
        if (!bi.createFile(tmpKernel)) {
            Log.e(TAG, "libmbp error: " + PatcherUtils.getErrorMessage(context, bi.getError()));
            return false;
        }

        // Repatch with loki if needed
        if (bi.isLoki()) {
            String lokiKernel = context.getCacheDir() + File.separator + "boot.lok";

            if (!MbtoolSocket.getInstance().lokiPatch(context, tmpKernel, lokiKernel)) {
                Log.e(TAG, "Failed to patch boot image with loki");
                return false;
            }

            if (!MbtoolSocket.getInstance().chmod(context, lokiKernel, 0666)) {
                Log.e(TAG, "Failed to chmod temporary aboot image");
                return false;
            }

            try {
                org.apache.commons.io.FileUtils.moveFile(new File(lokiKernel), tmpKernelFile);
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }
        }

        try {
            org.apache.commons.io.FileUtils.copyFile(tmpKernelFile, bootImageFile);
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }

        tmpKernelFile.delete();

        if (!SwitcherUtils.chooseRom(context, romInfo.getId())) {
            Log.e(TAG, "Failed to reflash boot image");
            return false;
        }

        Log.v(TAG, "Successfully updated ramdisk!");

        return true;
    }
}
