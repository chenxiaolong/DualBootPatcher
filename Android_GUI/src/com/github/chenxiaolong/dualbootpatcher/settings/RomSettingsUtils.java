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
import android.os.Environment;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.BootImage;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.BootImage.Type;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CpioFile;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.FileInfo;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Patcher;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.PatcherError;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SwitchRomResult;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;

public class RomSettingsUtils {
    private static final String TAG = RomSettingsUtils.class.getSimpleName();

    private static final String ABOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/aboot";

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

        Device device = PatcherUtils.getCurrentDevice(context, PatcherUtils.sPC);
        String codename = RomUtils.getDeviceCodename(context);
        if (device == null) {
            Log.e(TAG, "Current device " + codename + " does not appear to be supported");
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

    private static Process startLogcat() throws IOException {
        final File path = new File(Environment.getExternalStorageDirectory()
                + File.separator + "MultiBoot" + File.separator + "ramdisk-update.log");
        path.getParentFile().mkdirs();
        return Runtime.getRuntime().exec("logcat -v threadtime -f " + path + " *");
    }

    public synchronized static boolean updateRamdisk(Context context) {
        // Start logging to /sdcard/MultiBoot/ramdisk-update.log
        Process logProcess = null;
        try {
            logProcess = startLogcat();
        } catch (IOException e) {
            Log.e(TAG, "Failed to start logging to ramdisk-update.log", e);
        }

        try {
            // libmbp's MbtoolUpdater needs to grab a copy of the latest mbtool from the
            // data archive

            PatcherUtils.extractPatcher(context);

            // We'll need to add the ROM ID to the kernel command line
            RomInformation romInfo = RomUtils.getCurrentRom(context);
            if (romInfo == null) {
                Log.e(TAG, "Could not determine current ROM");
                return false;
            }

            String bootImage = RomUtils.getBootImagePath(romInfo.getId());
            File bootImageFile = new File(bootImage);

            // Make sure the kernel was backed up
            if (!bootImageFile.exists()) {
                try {
                    SetKernelResult result =
                            MbtoolSocket.getInstance().setKernel(context, romInfo.getId());
                    if (result != SetKernelResult.SUCCEEDED) {
                        Log.e(TAG, "Failed to backup boot image before modification");
                        return false;
                    }
                } catch (IOException e) {
                    Log.e(TAG, "mbtool communication error", e);
                    return false;
                }
            }

            String bootImageBackup = bootImage + ".before-ramdisk-update.img";
            File bootImageBackupFile = new File(bootImageBackup);

            try {
                org.apache.commons.io.FileUtils.copyFile(bootImageFile, bootImageBackupFile);
            } catch (IOException e) {
                Log.w(TAG, "Failed to copy " + bootImage + " to " + bootImageBackupFile, e);
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

            // Check if original boot image was patched with loki or bump
            BootImage bi = new BootImage();
            if (!bi.load(tmpKernel)) {
                logLibMbpError(context, bi.getError());
                bi.destroy();
                return false;
            }
            int wasType = bi.wasType();
            boolean hasRomIdFile;

            CpioFile cpio = new CpioFile();
            if (!cpio.load(bi.getRamdiskImage())) {
                logLibMbpError(context, cpio.getError());
                return false;
            }
            hasRomIdFile = cpio.isExists("romid");
            cpio.destroy();

            bi.destroy();

            Log.d(TAG, "Original boot image type: " + wasType);
            Log.d(TAG, "Original boot image had /romid file in ramdisk: " + hasRomIdFile);

            // Overwrite old boot image
            try {
                tmpKernelFile.delete();
                org.apache.commons.io.FileUtils.moveFile(new File(newFile), tmpKernelFile);
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }

            // Make changes to the boot image if necessary
            if (wasType == Type.LOKI || !hasRomIdFile) {
                bi = new BootImage();
                cpio = new CpioFile();

                try {
                    if (!bi.load(tmpKernel)) {
                        logLibMbpError(context, bi.getError());
                        return false;
                    }

                    if (wasType == Type.LOKI) {
                        Log.d(TAG, "Will reapply loki to boot image");
                        bi.setTargetType(Type.LOKI);

                        File aboot = new File(context.getCacheDir() + File.separator + "aboot.img");

                        MbtoolSocket socket = MbtoolSocket.getInstance();

                        // Copy aboot partition to the temporary file
                        if (!socket.copy(context, ABOOT_PARTITION, aboot.getPath()) ||
                                !socket.chmod(context, aboot.getPath(), 0644)) {
                            Log.e(TAG, "Failed to copy aboot partition to temporary file");
                            return false;
                        }

                        byte[] abootImage =
                                org.apache.commons.io.FileUtils.readFileToByteArray(aboot);
                        bi.setAbootImage(abootImage);

                        aboot.delete();
                    }
                    if (!hasRomIdFile) {
                        if (!cpio.load(bi.getRamdiskImage())) {
                            logLibMbpError(context, cpio.getError());
                            return false;
                        }
                        cpio.remove("romid");
                        if (!cpio.addFile(romInfo.getId().getBytes(
                                Charset.forName("UTF-8")), "romid", 0644)) {
                            logLibMbpError(context, cpio.getError());
                            return false;
                        }
                        bi.setRamdiskImage(cpio.createData());
                    }

                    if (!bi.createFile(tmpKernel)) {
                        logLibMbpError(context, bi.getError());
                        return false;
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                    return false;
                } finally {
                    bi.destroy();
                    cpio.destroy();
                }
            }

            try {
                org.apache.commons.io.FileUtils.copyFile(tmpKernelFile, bootImageFile);
            } catch (IOException e) {
                e.printStackTrace();
                return false;
            }

            tmpKernelFile.delete();

            try {
                SwitchRomResult result =
                        MbtoolSocket.getInstance().chooseRom(context, romInfo.getId());
                if (result != SwitchRomResult.SUCCEEDED) {
                    Log.e(TAG, "Failed to reflash boot image");
                    return false;
                }
            } catch (IOException e) {
                Log.e(TAG, "mbtool communication error", e);
                return false;
            }

            Log.v(TAG, "Successfully updated ramdisk!");

            return true;
        } finally {
            if (logProcess != null) {
                logProcess.destroy();
            }
        }
    }
}
