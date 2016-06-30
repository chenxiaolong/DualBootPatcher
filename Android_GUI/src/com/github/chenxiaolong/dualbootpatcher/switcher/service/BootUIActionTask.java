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
import android.os.Build;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;

import org.apache.commons.io.IOUtils;

import java.io.File;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Properties;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

import mbtool.daemon.v3.FileOpenFlag;
import mbtool.daemon.v3.PathDeleteFlag;

public final class BootUIActionTask extends BaseServiceTask {
    private static final String TAG = BootUIActionTask.class.getSimpleName();

    private static final String CACHE_MOUNT_POINT = "/cache";
    private static final String RAW_CACHE_MOUNT_POINT = "/raw/cache";

    private static final String BOOTUI_ZIP_PATH = "/multiboot/bootui.zip";
    private static final String BOOTUI_SIG_PATH = "/multiboot/bootui.zip.sig";

    private static final String PROPERTIES_FILE = "info.prop";
    private static final String PROP_VERSION = "bootui.version";
    private static final String PROP_GIT_VERSION = "bootui.git-version";

    private final BootUIAction mAction;

    private final Object mStateLock = new Object();
    private boolean mFinished;

    private Version mVersion;
    private boolean mSuccess;

    public enum BootUIAction {
        GET_VERSION,
        INSTALL,
        UNINSTALL
    }

    public interface BootUIActionTaskListener extends BaseServiceTaskListener,
            MbtoolErrorListener {
        void onBootUIHaveVersion(int taskId, Version version);

        void onBootUIInstalled(int taskId, boolean success);

        void onBootUIUninstalled(int taskId, boolean success);
    }

    public BootUIActionTask(int taskId, Context context, BootUIAction action) {
        super(taskId, context);
        mAction = action;
    }

    private static boolean pathExists(MbtoolInterface iface, String path)
            throws IOException, MbtoolException {
        int id = -1;

        try {
            id = iface.fileOpen(path, new short[]{FileOpenFlag.RDONLY}, 0);
            return true;
        } catch (MbtoolCommandException e) {
            // Ignore
        } finally {
            if (id >= 0) {
                try {
                    iface.fileClose(id);
                } catch (MbtoolCommandException e) {
                    // Ignore
                }
            }
        }

        return false;
    }

    /**
     * Get path to the cache partition's mount point
     *
     * @param iface Active mbtool interface
     * @return Either "/cache" or "/raw/cache"
     */
    private String getCacheMountPoint(MbtoolInterface iface) throws IOException, MbtoolException {
        String mountPoint = CACHE_MOUNT_POINT;

        if (pathExists(iface, RAW_CACHE_MOUNT_POINT)) {
            mountPoint = RAW_CACHE_MOUNT_POINT;
        }

        return mountPoint;
    }

    /**
     * Get currently installed version of boot UI
     *
     * @param iface Mbtool interface
     * @return The {@link Version} installed or null if an error occurs or the version number is
     *         invalid
     * @throws IOException
     * @throws MbtoolException
     */
    private Version getCurrentVersion(MbtoolInterface iface) throws IOException, MbtoolException {
        String mountPoint = getCacheMountPoint(iface);
        String zipPath = mountPoint + BOOTUI_ZIP_PATH;

        File tempZip = new File(getContext().getCacheDir() + "/bootui.zip");
        tempZip.delete();

        try {
            iface.pathCopy(zipPath, tempZip.getAbsolutePath());
            iface.pathChmod(tempZip.getAbsolutePath(), 0644);

            // Set SELinux label
            try {
                String label = iface.pathSelinuxGetLabel(tempZip.getParent(), false);
                iface.pathSelinuxSetLabel(tempZip.getAbsolutePath(), label, false);
            } catch (MbtoolCommandException e) {
                Log.w(TAG, tempZip + ": Failed to set SELinux label", e);
            }
        } catch (MbtoolCommandException e) {
            return null;
        }

        ZipFile zf = null;
        String versionStr = null;
        String gitVersionStr = null;

        try {
            zf = new ZipFile(tempZip);

            final Enumeration<? extends ZipEntry> entries = zf.entries();
            while (entries.hasMoreElements()) {
                final ZipEntry ze = entries.nextElement();

                if (ze.getName().equals(PROPERTIES_FILE)) {
                    Properties prop = new Properties();
                    prop.load(zf.getInputStream(ze));
                    versionStr = prop.getProperty(PROP_VERSION);
                    gitVersionStr = prop.getProperty(PROP_GIT_VERSION);
                    break;
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed to read bootui.zip", e);
        } finally {
            if (zf != null) {
                try {
                    zf.close();
                } catch (IOException e) {
                    // Ignore
                }
            }

            tempZip.delete();
        }

        Log.d(TAG, "Boot UI version: " + versionStr);
        Log.d(TAG, "Boot UI git version: " + gitVersionStr);

        if (versionStr != null) {
            return Version.from(versionStr);
        } else {
            return null;
        }
    }

    /**
     * Install or update boot UI
     *
     * @param iface Mbtool interface
     * @return Whether the installation was successful
     * @throws IOException
     * @throws MbtoolException
     */
    private boolean install(MbtoolInterface iface) throws IOException, MbtoolException {
        // Need to grab latest boot UI from the data archive
        PatcherUtils.initializePatcher(getContext());

        String abi;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            abi = Build.SUPPORTED_ABIS[0];
        } else {
            abi = Build.CPU_ABI;
        }

        String zipPath = PatcherUtils.getTargetDirectory(getContext())
                + File.separator + "bootui"
                + File.separator + abi
                + File.separator + "bootui.zip";
        String sigPath = PatcherUtils.getTargetDirectory(getContext())
                + File.separator + "bootui"
                + File.separator + abi
                + File.separator + "bootui.zip.sig";

        String mountPoint = getCacheMountPoint(iface);
        String targetZipPath = mountPoint + BOOTUI_ZIP_PATH;
        String targetSigPath = mountPoint + BOOTUI_SIG_PATH;
        File parent = new File(targetZipPath).getParentFile();

        try {
            iface.pathMkdir(parent.getAbsolutePath(), 0755, true);
            iface.pathCopy(zipPath, targetZipPath);
            iface.pathCopy(sigPath, targetSigPath);
            iface.pathChmod(targetZipPath, 0644);
            iface.pathChmod(targetSigPath, 0644);
            return true;
        } catch (MbtoolCommandException e) {
            Log.e(TAG, "Failed to install/update boot UI", e);
            return false;
        }
    }

    /**
     * Uninstall boot UI
     *
     * @param iface Mbtool interface
     * @return Whether the uninstallation was successful
     * @throws IOException
     * @throws MbtoolException
     */
    private boolean uninstall(MbtoolInterface iface) throws IOException, MbtoolException {
        String mountPoint = getCacheMountPoint(iface);
        String zipPath = mountPoint + BOOTUI_ZIP_PATH;
        String sigPath = mountPoint + BOOTUI_SIG_PATH;

        // Ignore errors for now since mbtool doesn't expose the errno value for us to check for
        // ENOENT
        try {
            iface.pathDelete(zipPath, PathDeleteFlag.UNLINK);
        } catch (MbtoolCommandException e) {
            // Ignore
        }
        try {
            iface.pathDelete(sigPath, PathDeleteFlag.UNLINK);
        } catch (MbtoolCommandException e) {
            // Ignore
        }

        return true;
    }

    @Override
    public void execute() {
        boolean success = false;
        Version version = null;

        MbtoolConnection conn = null;

        try {
            conn = new MbtoolConnection(getContext());
            MbtoolInterface iface = conn.getInterface();

            synchronized (BootUIAction.class) {
                switch (mAction) {
                case GET_VERSION:
                    version = getCurrentVersion(iface);
                    break;
                case INSTALL:
                    success = install(iface);
                    break;
                case UNINSTALL:
                    success = uninstall(iface);
                    break;
                }
            }
        } catch (IOException e) {
            Log.e(TAG, "mbtool communication error", e);
        } catch (MbtoolException e) {
            Log.e(TAG, "mbtool error", e);
            sendOnMbtoolError(e.getReason());
        } finally {
            IOUtils.closeQuietly(conn);
        }

        synchronized (mStateLock) {
            mSuccess = success;
            mVersion = version;
            sendResult();
            mFinished = true;
        }
    }

    @Override
    protected void onListenerAdded(BaseServiceTaskListener listener) {
        super.onListenerAdded(listener);

        synchronized (mStateLock) {
            if (mFinished) {
                sendResult();
            }
        }
    }

    private void sendResult() {
        switch (mAction) {
        case GET_VERSION:
            sendOnHaveVersion();
            break;
        case INSTALL:
            sendOnInstalled();
            break;
        case UNINSTALL:
            sendOnUninstalled();
            break;
        }
    }

    private void sendOnHaveVersion() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((BootUIActionTaskListener) listener).onBootUIHaveVersion(
                        getTaskId(), mVersion);
            }
        });
    }

    private void sendOnInstalled() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((BootUIActionTaskListener) listener).onBootUIInstalled(
                        getTaskId(), mSuccess);
            }
        });
    }

    private void sendOnUninstalled() {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((BootUIActionTaskListener) listener).onBootUIUninstalled(
                        getTaskId(), mSuccess);
            }
        });
    }

    private void sendOnMbtoolError(final Reason reason) {
        forEachListener(new CallbackRunnable() {
            @Override
            public void call(BaseServiceTaskListener listener) {
                ((BootUIActionTaskListener) listener).onMbtoolConnectionFailed(
                        getTaskId(), reason);
            }
        });
    }
}
