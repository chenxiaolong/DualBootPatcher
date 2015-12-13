/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher.service;

import android.content.Context;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.BuildConfig;
import com.github.chenxiaolong.dualbootpatcher.LogUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.BootImage;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.BootImage.Type;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CpioFile;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.FileInfo;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Patcher;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SwitchRomResult;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;

public final class UpdateRamdiskTask extends BaseServiceTask {
    private static final String TAG = UpdateRamdiskTask.class.getSimpleName();

    /** {@link Patcher} ID for the libmbp mbtool updater patcher */
    private static final String LIBMBP_MBTOOL_UPDATER = "MbtoolUpdater";
    /** aboot partition on the Galaxy S4 (for Loki) */
    private static final String ABOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/aboot";
    /** Suffix for boot image backup */
    private static final String BOOT_IMAGE_BACKUP_SUFFIX = ".before-ramdisk-update.img";

    public final RomInformation mRomInfo;
    private final UpdateRamdiskTaskListener mListener;

    public boolean mSuccess;

    public interface UpdateRamdiskTaskListener extends BaseServiceTaskListener {
        void onUpdatedRamdisk(int taskId, RomInformation romInfo, boolean success);
    }

    public UpdateRamdiskTask(int taskId, Context context, RomInformation romInfo,
                             UpdateRamdiskTaskListener listener) {
        super(taskId, context);
        mRomInfo = romInfo;
        mListener = listener;
    }

    /**
     * Log the libmbp error and destoy the PatcherError object
     *
     * @param error PatcherError
     */
    private static void logLibMbpError(int error) {
        Log.e(TAG, "libmbp error code: " + error);
    }

    /**
     * Update mbtool using libmbp and return the path to the new boot image
     *
     * @return New file path on success or null on error
     */
    private String updateMbtool(String path) {
        Patcher patcher = PatcherUtils.sPC.createPatcher(LIBMBP_MBTOOL_UPDATER);
        if (patcher == null) {
            Log.e(TAG, "Bundled libmbp does not support " + LIBMBP_MBTOOL_UPDATER);
            return null;
        }

        FileInfo fi = new FileInfo();
        try {
            fi.setFilename(path);

            Device device = PatcherUtils.getCurrentDevice(getContext(), PatcherUtils.sPC);
            String codename = RomUtils.getDeviceCodename(getContext());
            if (device == null) {
                Log.e(TAG, "Current device " + codename + " does not appear to be supported");
                return null;
            }
            fi.setDevice(device);

            patcher.setFileInfo(fi);

            if (!patcher.patchFile(null)) {
                logLibMbpError(patcher.getError());
                return null;
            }

            return patcher.newFilePath();
        } finally {
            fi.destroy();
            PatcherUtils.sPC.destroyPatcher(patcher);
        }
    }

    private boolean repatchBootImage(File file, int wasType, boolean hasRomIdFile) {
        if (wasType == Type.LOKI || !hasRomIdFile) {
            BootImage bi = new BootImage();
            CpioFile cpio = new CpioFile();

            try {
                if (!bi.load(file.getAbsolutePath())) {
                    logLibMbpError(bi.getError());
                    return false;
                }

                if (wasType == Type.LOKI) {
                    Log.d(TAG, "Will reapply loki to boot image");
                    bi.setTargetType(Type.LOKI);

                    File abootFile = new File(
                            getContext().getCacheDir() + File.separator + "aboot.img");

                    MbtoolSocket socket = MbtoolSocket.getInstance();

                    // Copy aboot partition to the temporary file
                    if (!socket.pathCopy(getContext(), ABOOT_PARTITION, abootFile.getPath()) ||
                            !socket.pathChmod(getContext(), abootFile.getPath(), 0644)) {
                        Log.e(TAG, "Failed to copy aboot partition to temporary file");
                        return false;
                    }

                    byte[] abootImage =
                            org.apache.commons.io.FileUtils.readFileToByteArray(abootFile);
                    bi.setAbootImage(abootImage);

                    abootFile.delete();
                }
                if (!hasRomIdFile) {
                    if (!cpio.load(bi.getRamdiskImage())) {
                        logLibMbpError(cpio.getError());
                        return false;
                    }
                    cpio.remove("romid");
                    if (!cpio.addFile(mRomInfo.getId().getBytes(
                            Charset.forName("UTF-8")), "romid", 0644)) {
                        logLibMbpError(cpio.getError());
                        return false;
                    }
                    bi.setRamdiskImage(cpio.createData());
                }

                if (!bi.createFile(file.getAbsolutePath())) {
                    logLibMbpError(bi.getError());
                    return false;
                }
            } catch (IOException e) {
                Log.e(TAG, "Failed to make changes to boot image", e);
                return false;
            } finally {
                bi.destroy();
                cpio.destroy();
            }
        }

        return true;
    }

    /**
     * Switch the ROM if we're updating the ramdisk for the currently booted ROM
     *
     * @return True if the operation succeeded or was not needed
     */
    private boolean switchRomIfNeeded() {
        RomInformation currentRom = RomUtils.getCurrentRom(getContext());
        if (currentRom != null && currentRom.getId().equals(mRomInfo.getId())) {
            try {
                SwitchRomResult result =
                        MbtoolSocket.getInstance().switchRom(getContext(), mRomInfo.getId(), true);
                if (result != SwitchRomResult.SUCCEEDED) {
                    Log.e(TAG, "Failed to reflash boot image");
                    return false;
                }
            } catch (IOException e) {
                Log.e(TAG, "mbtool communication error", e);
                return false;
            }
        }
        return true;
    }

    /**
     * Set the kernel if we're updating the ramdisk for the currently booted ROM
     *
     * @return True if the operation succeeded or was not needed
     */
    private boolean setKernelIfNeeded() {
        RomInformation currentRom = RomUtils.getCurrentRom(getContext());
        if (currentRom != null && currentRom.getId().equals(mRomInfo.getId())) {
            try {
                SetKernelResult result =
                        MbtoolSocket.getInstance().setKernel(getContext(), mRomInfo.getId());
                if (result != SetKernelResult.SUCCEEDED) {
                    Log.e(TAG, "Failed to reflash boot image");
                    return false;
                }
            } catch (IOException e) {
                Log.e(TAG, "mbtool communication error", e);
                return false;
            }
        }
        return true;
    }

    private boolean updateRamdisk() {
        synchronized (UpdateRamdiskTask.class) {
            Log.d(TAG, "Starting to update ramdisk for " + mRomInfo.getId() + " to "
                    + BuildConfig.VERSION_NAME);

            try {
                // libmbp's MbtoolUpdater needs to grab a copy of the latest mbtool from the
                // data archive
                PatcherUtils.initializePatcher(getContext());

                // We'll need to add the ROM ID to the /romid file
                String bootImage = RomUtils.getBootImagePath(mRomInfo.getId());
                File bootImageFile = new File(bootImage);

                // Make sure the kernel exists
                if (!bootImageFile.exists() && !setKernelIfNeeded()) {
                    Log.e(TAG, "The kernel has not been backed up");
                    return false;
                }

                // Backup the working kernel
                File bootImageBackupFile = new File(bootImage + BOOT_IMAGE_BACKUP_SUFFIX);
                try {
                    org.apache.commons.io.FileUtils.copyFile(bootImageFile, bootImageBackupFile);
                } catch (IOException e) {
                    Log.w(TAG, "Failed to copy " + bootImage + " to " + bootImageBackupFile, e);
                }

                // Create temporary copy of the boot image
                File tmpKernelFile = new File(
                        getContext().getCacheDir() + File.separator + "boot.img");
                try {
                    org.apache.commons.io.FileUtils.copyFile(bootImageFile, tmpKernelFile);
                } catch (IOException e) {
                    Log.e(TAG, "Failed to copy boot image to temporary file", e);
                    return false;
                }

                // Run libmbp's MbtoolUpdater on the boot image
                String newFile = updateMbtool(tmpKernelFile.getAbsolutePath());
                if (newFile == null) {
                    Log.e(TAG, "Failed to patch file!");
                    return false;
                }

                // Check if original boot image was patched with loki or bump
                int wasType;
                boolean hasRomIdFile;

                BootImage bi = new BootImage();
                CpioFile cpio = new CpioFile();
                try {
                    if (!bi.load(tmpKernelFile.getAbsolutePath())) {
                        logLibMbpError(bi.getError());
                        return false;
                    }

                    wasType = bi.wasType();

                    if (!cpio.load(bi.getRamdiskImage())) {
                        logLibMbpError(cpio.getError());
                        return false;
                    }

                    hasRomIdFile = cpio.isExists("romid");
                } finally {
                    bi.destroy();
                    cpio.destroy();
                }

                Log.d(TAG, "Original boot image type: " + wasType);
                Log.d(TAG, "Original boot image had /romid file in ramdisk: " + hasRomIdFile);

                // Overwrite old boot image
                try {
                    tmpKernelFile.delete();
                    org.apache.commons.io.FileUtils.moveFile(new File(newFile), tmpKernelFile);
                } catch (IOException e) {
                    Log.e(TAG, "Failed to move " + newFile + " to " + tmpKernelFile, e);
                    return false;
                }

                // Make changes to the boot image if necessary
                if (!repatchBootImage(tmpKernelFile, wasType, hasRomIdFile)) {
                    return false;
                }

                // Overwrite saved boot image
                try {
                    org.apache.commons.io.FileUtils.copyFile(tmpKernelFile, bootImageFile);
                } catch (IOException e) {
                    Log.e(TAG, "Failed to copy " + tmpKernelFile + " to " + bootImageFile, e);
                    return false;
                }

                tmpKernelFile.delete();

                // Reflash boot image if we're updating the ramdisk for the current ROM
                if (!switchRomIfNeeded()) {
                    return false;
                }

                Log.v(TAG, "Successfully updated ramdisk!");

                return true;
            } finally {
                // Save whatever is in the logcat buffer to /sdcard/MultiBoot/ramdisk-update.log
                LogUtils.dump("ramdisk-update.log");
            }
        }
    }

    @Override
    public void execute() {
        mSuccess = updateRamdisk();
        mListener.onUpdatedRamdisk(getTaskId(), mRomInfo, mSuccess);
    }
}
