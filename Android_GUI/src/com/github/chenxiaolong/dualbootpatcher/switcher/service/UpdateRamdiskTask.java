/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.BootImage;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.BootImage.Type;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CpioFile;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.FileInfo;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Patcher;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.PatcherConfig;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMiscStuff;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SwitchRomResult;
import com.google.common.base.Charsets;
import com.sun.jna.ptr.IntByReference;

import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.StringUtils;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.ArrayList;

public final class UpdateRamdiskTask extends BaseServiceTask {
    private static final String TAG = UpdateRamdiskTask.class.getSimpleName();

    /** {@link Patcher} ID for the libmbp mbtool updater patcher */
    private static final String LIBMBP_MBTOOL_UPDATER = "MbtoolUpdater";
    /** aboot partition on the Galaxy S4 (for Loki) */
    private static final String ABOOT_PARTITION = "/dev/block/platform/msm_sdcc.1/by-name/aboot";
    /** Suffix for boot image backup */
    private static final String BOOT_IMAGE_BACKUP_SUFFIX = ".before-ramdisk-update.img";

    private final RomInformation mRomInfo;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private boolean mSuccess;

    public interface UpdateRamdiskTaskListener extends BaseServiceTaskListener,
            MbtoolErrorListener {
        void onUpdatedRamdisk(int taskId, RomInformation romInfo, boolean success);
    }

    public UpdateRamdiskTask(int taskId, Context context, RomInformation romInfo) {
        super(taskId, context);
        mRomInfo = romInfo;
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
     * @return Whether the ramdisk was successfully updated
     */
    private boolean updateMbtool(File path) {
        PatcherConfig pc = null;
        Patcher patcher = null;
        FileInfo fi = null;

        File outPath = new File(path + ".new");

        try {
            fi = new FileInfo();
            pc = PatcherUtils.newPatcherConfig(getContext());
            patcher = pc.createPatcher(LIBMBP_MBTOOL_UPDATER);
            if (patcher == null) {
                Log.e(TAG, "Bundled libmbp does not support " + LIBMBP_MBTOOL_UPDATER);
                return false;
            }

            fi.setInputPath(path.getAbsolutePath());
            fi.setOutputPath(outPath.getAbsolutePath());

            Device device = PatcherUtils.getCurrentDevice(getContext());
            String codename = RomUtils.getDeviceCodename(getContext());
            if (device == null) {
                Log.e(TAG, "Current device " + codename + " does not appear to be supported");
                return false;
            }
            fi.setDevice(device);

            patcher.setFileInfo(fi);

            if (!patcher.patchFile(null)) {
                logLibMbpError(patcher.getError());
                return false;
            }

            // Overwrite old boot image
            path.delete();
            org.apache.commons.io.FileUtils.moveFile(outPath, path);

            return true;
        } catch (IOException e) {
            Log.e(TAG, "Failed to update ramdisk", e);
            return false;
        } finally {
            if (patcher != null) {
                pc.destroyPatcher(patcher);
            }
            if (pc != null) {
                pc.destroy();
            }
            if (fi != null) {
                fi.destroy();
            }
            outPath.delete();
        }
    }

    private enum UsesExfat {
        YES, NO, UNKNOWN
    }

    private UsesExfat voldUsesFuseExfat(MbtoolInterface iface)
            throws IOException, MbtoolException {
        // Only do this for the current ROM since we might not have access to the files of other
        // ROMs (eg. if the ROM uses a system image)
        File tempVold = new File(getContext().getCacheDir() + "/vold");
        tempVold.delete();

        try {
            RomInformation currentRom = RomUtils.getCurrentRom(getContext(), iface);
            if (currentRom != null && currentRom.getId().equals(mRomInfo.getId())) {
                iface.pathCopy("/system/bin/vold", tempVold.getAbsolutePath());
                iface.pathChmod(tempVold.getAbsolutePath(), 0644);

                // Set SELinux context
                try {
                    String label = iface.pathSelinuxGetLabel(tempVold.getParent(), false);
                    iface.pathSelinuxSetLabel(tempVold.getAbsolutePath(), label, false);
                } catch (MbtoolCommandException e) {
                    Log.w(TAG, "Failed to set SELinux label of temp vold copy", e);
                }

                IntByReference result = new IntByReference();
                if (LibMiscStuff.INSTANCE.find_string_in_file(
                        tempVold.getAbsolutePath(), "/system/bin/mount.exfat", result)) {
                    return result.getValue() != 0 ? UsesExfat.YES : UsesExfat.NO;
                }
            }
        } catch (MbtoolCommandException e) {
            Log.e(TAG, "Failed to determine if vold uses fuse-exfat", e);
        } finally {
            tempVold.delete();
        }

        return UsesExfat.UNKNOWN;
    }

    private boolean repatchBootImage(MbtoolInterface iface, File file, int wasType,
                                     boolean hasRomIdFile, UsesExfat usesExfat)
            throws IOException, MbtoolException {
        if (wasType == Type.LOKI || !hasRomIdFile || usesExfat != UsesExfat.UNKNOWN) {
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

                    // Copy aboot partition to the temporary file
                    try {
                        iface.pathCopy(ABOOT_PARTITION, abootFile.getAbsolutePath());
                        iface.pathChmod(abootFile.getAbsolutePath(), 0644);
                    } catch (MbtoolCommandException e) {
                        Log.e(TAG, "Failed to copy aboot partition to temporary file");
                        return false;
                    }

                    byte[] abootImage;
                    try {
                        abootImage = org.apache.commons.io.FileUtils.readFileToByteArray(abootFile);
                    } catch (IOException e) {
                        Log.e(TAG, "Failed to read temporary aboot dump", e);
                        return false;
                    }
                    bi.setAbootImage(abootImage);

                    abootFile.delete();
                }
                if (!hasRomIdFile || usesExfat != UsesExfat.UNKNOWN) {
                    if (!cpio.load(bi.getRamdiskImage())) {
                        logLibMbpError(cpio.getError());
                        return false;
                    }
                    if (!hasRomIdFile) {
                        cpio.remove("romid");
                        if (!cpio.addFile(mRomInfo.getId().getBytes(
                                Charset.forName("UTF-8")), "romid", 0644)) {
                            logLibMbpError(cpio.getError());
                            return false;
                        }
                    }
                    if (usesExfat != UsesExfat.UNKNOWN) {
                        byte[] contents = cpio.getContents("default.prop");
                        if (contents != null) {
                            ArrayList<String> lines = new ArrayList<>();
                            for (String line : new String(contents, Charsets.UTF_8).split("\n")) {
                                if (!line.startsWith("ro.patcher.use_fuse_exfat=")) {
                                    lines.add(line);
                                }
                            }
                            lines.add("ro.patcher.use_fuse_exfat=" +
                                    (usesExfat == UsesExfat.YES ? "true" : "false"));
                            cpio.setContents("default.prop",
                                    StringUtils.join(lines, "\n").getBytes(Charsets.UTF_8));

                        }
                    }
                    bi.setRamdiskImage(cpio.createData());
                }

                if (!bi.createFile(file.getAbsolutePath())) {
                    logLibMbpError(bi.getError());
                    return false;
                }
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
    private boolean switchRomIfNeeded(MbtoolInterface iface)
            throws IOException, MbtoolException, MbtoolCommandException {
        RomInformation currentRom = RomUtils.getCurrentRom(getContext(), iface);
        if (currentRom != null && currentRom.getId().equals(mRomInfo.getId())) {
            SwitchRomResult result = iface.switchRom(getContext(), mRomInfo.getId(), true);
            if (result != SwitchRomResult.SUCCEEDED) {
                Log.e(TAG, "Failed to reflash boot image");
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
    private boolean setKernelIfNeeded(MbtoolInterface iface)
            throws IOException, MbtoolException, MbtoolCommandException {
        RomInformation currentRom = RomUtils.getCurrentRom(getContext(), iface);
        if (currentRom != null && currentRom.getId().equals(mRomInfo.getId())) {
            SetKernelResult result = iface.setKernel(getContext(), mRomInfo.getId());
            if (result != SetKernelResult.SUCCEEDED) {
                Log.e(TAG, "Failed to reflash boot image");
                return false;
            }
        }
        return true;
    }

    private boolean updateRamdisk(MbtoolInterface iface)
            throws IOException, MbtoolException, MbtoolCommandException {
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
                if (!bootImageFile.exists() && !setKernelIfNeeded(iface)) {
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
                if (!updateMbtool(tmpKernelFile)) {
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

                UsesExfat usesExfat = voldUsesFuseExfat(iface);

                Log.d(TAG, "Original boot image type: " + wasType);
                Log.d(TAG, "Original boot image had /romid file in ramdisk: " + hasRomIdFile);
                Log.d(TAG, "vold uses fuse exfat: " + usesExfat.name());

                // Make changes to the boot image if necessary
                if (!repatchBootImage(iface, tmpKernelFile, wasType, hasRomIdFile, usesExfat)) {
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
                if (!switchRomIfNeeded(iface)) {
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
        boolean success = false;

        MbtoolConnection conn = null;

        try {
            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            success = updateRamdisk(iface);
        } catch (IOException e) {
            Log.e(TAG, "mbtool communication error", e);
        } catch (MbtoolException e) {
            Log.e(TAG, "mbtool error", e);
            sendOnMbtoolError(e.getReason());
        } catch (MbtoolCommandException e) {
            Log.w(TAG, "mbtool command error", e);
        } finally {
            IOUtils.closeQuietly(conn);
        }

        synchronized (mStateLock) {
            mSuccess = success;
            sendOnUpdatedRamdisk();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            if (mFinished) {
                sendOnUpdatedRamdisk();
            }
        }
    }

    private void sendOnUpdatedRamdisk() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((UpdateRamdiskTaskListener) listener).onUpdatedRamdisk(
                        getTaskId(), mRomInfo, mSuccess);
            }
        });
    }

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((UpdateRamdiskTaskListener) listener).onMbtoolConnectionFailed(
                        getTaskId(), reason);
            }
        });
    }
}
