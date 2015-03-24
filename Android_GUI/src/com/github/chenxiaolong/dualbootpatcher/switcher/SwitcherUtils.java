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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.content.Context;
import android.os.Build;

import com.github.chenxiaolong.multibootpatcher.Version;
import com.github.chenxiaolong.multibootpatcher.Version.VersionParseException;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatcherConfig;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolSocket;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolUtils.Feature;

import org.apache.commons.io.IOUtils;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Properties;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class SwitcherUtils {
    public static final String TAG = SwitcherUtils.class.getSimpleName();

    private static final String ZIP_MULTIBOOT_DIR = "multiboot/";
    private static final String ZIP_INFO_PROP = ZIP_MULTIBOOT_DIR + "info.prop";
    private static final String PROP_INSTALLER_VERSION = "mbtool.installer.version";

    public static String getBootPartition() {
        String bootBlockDev = null;

        PatcherConfig pc = new PatcherConfig();
        for (Device d : pc.getDevices()) {
            boolean matches = false;

            for (String codename : d.getCodenames()) {
                if (Build.DEVICE.equals(codename)) {
                    matches = true;
                    break;
                }
            }

            if (matches) {
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

    public static VerificationResult verifyZipMbtoolVersion(String zipFile) {
        ZipFile zf = null;

        try {
            zf = new ZipFile(zipFile);

            boolean isMultiboot = false;

            Properties prop = null;

            final Enumeration<? extends ZipEntry> entries = zf.entries();
            while (entries.hasMoreElements()) {
                final ZipEntry ze = entries.nextElement();

                if (ze.getName().startsWith(ZIP_MULTIBOOT_DIR)) {
                    isMultiboot = true;
                }
                if (ze.getName().equals(ZIP_INFO_PROP)) {
                    prop = new Properties();
                    prop.load(zf.getInputStream(ze));
                    break;
                }
            }

            if (!isMultiboot) {
                return VerificationResult.ERROR_NOT_MULTIBOOT;
            }

            if (prop == null) {
                return VerificationResult.ERROR_VERSION_TOO_OLD;
            }

            Version version = new Version(prop.getProperty(PROP_INSTALLER_VERSION, "0.0.0"));
            Version minVersion = MbtoolUtils.getMinimumRequiredVersion(Feature.IN_APP_INSTALLATION);

            if (version.compareTo(minVersion) < 0) {
                return VerificationResult.ERROR_VERSION_TOO_OLD;
            }

            return VerificationResult.NO_ERROR;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return VerificationResult.ERROR_ZIP_NOT_FOUND;
        } catch (IOException e) {
            e.printStackTrace();
            return VerificationResult.ERROR_ZIP_READ_FAIL;
        } catch (VersionParseException e) {
            e.printStackTrace();
            return VerificationResult.ERROR_VERSION_TOO_OLD;
        } finally {
            IOUtils.closeQuietly(zf);
        }
    }

    public enum VerificationResult {
        NO_ERROR,
        ERROR_ZIP_NOT_FOUND,
        ERROR_ZIP_READ_FAIL,
        ERROR_NOT_MULTIBOOT,
        ERROR_VERSION_TOO_OLD
    }

    public static void reboot(final Context context) {
        new Thread() {
            @Override
            public void run() {
                MbtoolSocket.getInstance().restart(context, "");
            }
        }.start();
    }
}
