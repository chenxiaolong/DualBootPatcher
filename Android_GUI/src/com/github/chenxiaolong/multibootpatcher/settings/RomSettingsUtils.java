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

package com.github.chenxiaolong.multibootpatcher.settings;

import android.content.Context;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.BootImage;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.FileInfo;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Patcher;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatcherError;
import com.github.chenxiaolong.multibootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolSocket.SetKernelResult;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolSocket.SwitchRomResult;

import java.io.File;
import java.io.IOException;

public class RomSettingsUtils {
    private static final String TAG = RomSettingsUtils.class.getSimpleName();

    private static final String BOOT_IMAGE_PATH =
            Environment.getExternalStorageDirectory() + "/MultiBoot/%s/boot.img";

    /**
     * Update mbtool using libmbp and return the path to the new boot image
     *
     * @return New file path on success or null on error
     */
    private static String updateMbtool(Context context, String path) {
        PatcherUtils.initializePatcher(context);

        Patcher patcher = PatcherUtils.sPC.createPatcher("MbtoolUpdater");
        if (patcher == null) {
            Log.e(TAG, "Bundled libmbp does not support MbtoolUpdater!");
            return null;
        }

        FileInfo fi = new FileInfo();
        fi.setFilename(path);

        Device device = PatcherUtils.getCurrentDevice(PatcherUtils.sPC);
        if (device == null) {
            Log.e(TAG, "Current device " + Build.DEVICE + " does not appear to be supported");
            return null;
        }
        fi.setDevice(device);

        patcher.setFileInfo(fi);

        if (!patcher.patchFile(null)) {
            logLibMbpError(context, patcher.getError());
            PatcherUtils.sPC.destroyPatcher(patcher);
            return null;
        }

        String newPath = patcher.newFilePath();
        PatcherUtils.sPC.destroyPatcher(patcher);
        return newPath;
    }

    /**
     * Log the libmbp error and destoy the PatcherError object
     *
     * @param context Context
     * @param error PatcherError
     */
    private static void logLibMbpError(Context context, PatcherError error) {
        Log.e(TAG, "libmbp error: " + PatcherUtils.getErrorMessage(context, error));
        error.destroy();
    }

    public static boolean updateRamdisk(Context context) {
        // libmbp's MbtoolUpdater needs to grab a copy of the latest mbtool from the data archive
        PatcherUtils.extractPatcher(context);

        // We'll need to add the ROM ID to the kernel command line
        RomInformation romInfo = RomUtils.getCurrentRom(context);
        if (romInfo == null) {
            Log.e(TAG, "Could not determine current ROM");
            return false;
        }

        String bootImage = String.format(BOOT_IMAGE_PATH, romInfo.getId());
        File bootImageFile = new File(bootImage);

        // Make sure the kernel was backed up
        if (!bootImageFile.exists()) {
            SetKernelResult result = MbtoolSocket.getInstance().setKernel(context, romInfo.getId());
            if (result != SetKernelResult.SUCCEEDED) {
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

        // Run MbtoolUpdater on the boot image
        String newFile = updateMbtool(context, tmpKernel);
        if (newFile == null) {
            Log.e(TAG, "Failed to patch file!");
            return false;
        }

        // Check if original boot image was lokid
        BootImage bi = new BootImage();
        if (!bi.load(tmpKernel)) {
            logLibMbpError(context, bi.getError());
            return false;
        }
        boolean wasLoki = bi.isLoki();
        boolean hasRomId = bi.getKernelCmdline().contains("romid=");
        bi.destroy();

        // Overwrite old boot image
        try {
            tmpKernelFile.delete();
            org.apache.commons.io.FileUtils.moveFile(new File(newFile), tmpKernelFile);
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }

        // Add the ROM ID to the kernel command line if it wasn't added already
        if (!hasRomId) {
            bi = new BootImage();
            if (!bi.load(tmpKernel)) {
                logLibMbpError(context, bi.getError());
                return false;
            }
            bi.setKernelCmdline(bi.getKernelCmdline() + " romid=" + romInfo.getId());
            if (!bi.createFile(tmpKernel)) {
                logLibMbpError(context, bi.getError());
                return false;
            }
            bi.destroy();
        }

        // Repatch with loki if needed
        if (wasLoki) {
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

        SwitchRomResult result = MbtoolSocket.getInstance().chooseRom(context, romInfo.getId());
        if (result != SwitchRomResult.SUCCEEDED) {
            Log.e(TAG, "Failed to reflash boot image");
            return false;
        }

        Log.v(TAG, "Successfully updated ramdisk!");

        return true;
    }
}
