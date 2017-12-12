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
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.FileInfo;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.Patcher;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.PatcherConfig;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SetKernelResult;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.IOException;

public final class CreateRamdiskUpdaterTask extends BaseServiceTask {
    private static final String TAG = CreateRamdiskUpdaterTask.class.getSimpleName();

    /** {@link Patcher} ID for the libmbpatcher patcher */
    private static final String LIBMBPATCHER_RAMDISK_UPDATER = "RamdiskUpdater";
    /** Suffix for boot image backup */
    private static final String BOOT_IMAGE_BACKUP_SUFFIX = ".before-ramdisk-update.img";

    private final RomInformation mRomInfo;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private String mPath;

    public interface CreateRamdiskUpdaterTaskListener extends BaseServiceTaskListener,
            MbtoolErrorListener {
        void onCreatedRamdiskUpdater(int taskId, RomInformation romInfo, String path);
    }

    public CreateRamdiskUpdaterTask(int taskId, Context context, RomInformation romInfo) {
        super(taskId, context);
        mRomInfo = romInfo;
    }

    /**
     * Log the libmbpatcher error and destoy the PatcherError object
     *
     * @param error PatcherError
     */
    private static void logLibMbPatcherError(int error) {
        Log.e(TAG, "libmbpatcher error code: " + error);
    }

    /**
     * Create zip for updating the ramdisk
     *
     * @return Whether the ramdisk was successfully updated
     */
    private boolean createZip(File path) {
        PatcherConfig pc = null;
        Patcher patcher = null;
        FileInfo fi = null;

        try {
            fi = new FileInfo();
            pc = PatcherUtils.newPatcherConfig(getContext());
            patcher = pc.createPatcher(LIBMBPATCHER_RAMDISK_UPDATER);
            if (patcher == null) {
                Log.e(TAG, "Bundled libmbpatcher does not support " + LIBMBPATCHER_RAMDISK_UPDATER);
                return false;
            }

            fi.setOutputPath(path.getAbsolutePath());

            Device device = PatcherUtils.getCurrentDevice(getContext());
            String codename = RomUtils.getDeviceCodename(getContext());
            if (device == null) {
                Log.e(TAG, "Current device " + codename + " does not appear to be supported");
                return false;
            }
            fi.setDevice(device);

            patcher.setFileInfo(fi);

            if (!patcher.patchFile(null)) {
                logLibMbPatcherError(patcher.getError());
                return false;
            }

            return true;
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
        }
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

    private String createRamdiskUpdater(MbtoolInterface iface)
            throws IOException, MbtoolException, MbtoolCommandException {
        synchronized (CreateRamdiskUpdaterTask.class) {
            Log.d(TAG, "Creating ramdisk updater zip for " + mRomInfo.getId() + " (version: "
                    + BuildConfig.VERSION_NAME + ")");

            try {
                PatcherUtils.initializePatcher(getContext());

                File bootImageFile = new File(RomUtils.getBootImagePath(mRomInfo.getId()));

                // Make sure the boot image exists
                if (!bootImageFile.exists() && !setKernelIfNeeded(iface)) {
                    Log.e(TAG, "The kernel has not been backed up");
                    return null;
                }

                // Back up existing boot image
                File bootImageBackupFile = new File(bootImageFile + BOOT_IMAGE_BACKUP_SUFFIX);
                try {
                    org.apache.commons.io.FileUtils.copyFile(bootImageFile, bootImageBackupFile);
                } catch (IOException e) {
                    Log.w(TAG, "Failed to copy " + bootImageFile + " to " + bootImageBackupFile, e);
                }

                // Create ramdisk updater zip
                File zipFile = new File(
                        getContext().getCacheDir() + File.separator + "ramdisk-updater.zip");

                // Run libmbpatcher's RamdiskUpdater on the boot image
                if (!createZip(zipFile)) {
                    Log.e(TAG, "Failed to create ramdisk updater zip");
                    return null;
                }

                Log.v(TAG, "Successfully created ramdisk updater zip");

                return zipFile.getAbsolutePath();
            } finally {
                // Save whatever is in the logcat buffer to /sdcard/MultiBoot/ramdisk-update.log
                LogUtils.dump("ramdisk-update.log");
            }
        }
    }

    @Override
    public void execute() {
        String path = null;

        MbtoolConnection conn = null;

        try {
            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            path = createRamdiskUpdater(iface);
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
            mPath = path;
            sendOnCreatedRamdiskUpdater();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            if (mFinished) {
                sendOnCreatedRamdiskUpdater();
            }
        }
    }

    private void sendOnCreatedRamdiskUpdater() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((CreateRamdiskUpdaterTaskListener) listener).onCreatedRamdiskUpdater(
                        getTaskId(), mRomInfo, mPath);
            }
        });
    }

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((CreateRamdiskUpdaterTaskListener) listener).onMbtoolConnectionFailed(
                        getTaskId(), reason);
            }
        });
    }
}
