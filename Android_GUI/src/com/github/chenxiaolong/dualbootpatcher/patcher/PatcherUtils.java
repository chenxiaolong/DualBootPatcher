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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.content.Context;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.BuildConfig;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.ErrorCode;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.FileInfo;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Patcher;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Patcher.ProgressListener;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.PatcherConfig;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMiscStuff;

import java.io.File;
import java.util.ArrayList;

public class PatcherUtils {
    public static final String TAG = PatcherUtils.class.getSimpleName();
    private static final String FILENAME = "data-%s.tar.xz";
    private static final String DIRNAME = "data-%s";

    public static final String PARAM_PATCHER = "patcher";
    public static final String PARAM_FILEINFO = "fileinfo";

    public static final String RESULT_PATCH_FILE_NEW_FILE = "new_file";
    public static final String RESULT_PATCH_FILE_MESSAGE = "message";
    public static final String RESULT_PATCH_FILE_FAILED = "failed";

    public static PatcherConfig sPC;

    private static String sTargetFile;
    private static String sTargetDir;

    private static InstallLocation[] sInstallLocations;

    static {
        String version = BuildConfig.VERSION_NAME.split("-")[0];
        sTargetFile = String.format(FILENAME, version);
        sTargetDir = String.format(DIRNAME, version);
    }

    private static File getTargetFile(Context context) {
        return new File(context.getCacheDir() + File.separator + sTargetFile);
    }

    public static File getTargetDirectory(Context context) {
        return new File(context.getFilesDir() + File.separator + sTargetDir);
    }

    public synchronized static void initializePatcher(Context context) {
        extractPatcher(context);

        if (sPC == null) {
            sPC = new PatcherConfig();
            sPC.setDataDirectory(getTargetDirectory(context).getAbsolutePath());
            sPC.setTempDirectory(context.getCacheDir().getAbsolutePath());
        }
    }

    public synchronized static Device getCurrentDevice(Context context, PatcherConfig pc) {
        String realCodename = RomUtils.getDeviceCodename(context);

        Device device = null;
        for (Device d : pc.getDevices()) {
            for (String codename : d.getCodenames()) {
                if (realCodename.equals(codename)) {
                    device = d;
                    break;
                }
            }
            if (device != null) {
                break;
            }
        }

        return device;
    }

    public synchronized static Bundle patchFile(Context context, Bundle data,
                                                ProgressListener listener) {
        // Make sure patcher is extracted first
        extractPatcher(context);

        Patcher patcher = data.getParcelable(PARAM_PATCHER);
        FileInfo fileInfo = data.getParcelable(PARAM_FILEINFO);

        patcher.setFileInfo(fileInfo);
        boolean ret = patcher.patchFile(listener);

        Bundle bundle = new Bundle();
        bundle.putString(RESULT_PATCH_FILE_NEW_FILE, patcher.newFilePath());
        bundle.putString(RESULT_PATCH_FILE_MESSAGE, getErrorMessage(context, patcher.getError()));
        bundle.putBoolean(RESULT_PATCH_FILE_FAILED, !ret);
        return bundle;
    }

    public static String getErrorMessage(Context context, int error) {
        switch (error) {
        case ErrorCode.NO_ERROR:
            return context.getString(R.string.no_error);
        case ErrorCode.UNKNOWN_ERROR:
            return context.getString(R.string.unknown_error);
        case ErrorCode.PATCHER_CREATE_ERROR:
            return context.getString(R.string.patcher_create_error);
        case ErrorCode.AUTOPATCHER_CREATE_ERROR:
            return context.getString(R.string.autopatcher_create_error);
        case ErrorCode.RAMDISK_PATCHER_CREATE_ERROR:
            return context.getString(R.string.ramdisk_patcher_create_error);
        case ErrorCode.FILE_OPEN_ERROR:
            return context.getString(R.string.file_open_error);
        case ErrorCode.FILE_READ_ERROR:
            return context.getString(R.string.file_read_error);
        case ErrorCode.FILE_WRITE_ERROR:
            return context.getString(R.string.file_write_error);
        case ErrorCode.DIRECTORY_NOT_EXIST_ERROR:
            return context.getString(R.string.directory_not_exist_error);
        case ErrorCode.BOOT_IMAGE_PARSE_ERROR:
            return context.getString(R.string.boot_image_parse_error);
        case ErrorCode.BOOT_IMAGE_APPLY_BUMP_ERROR:
            return context.getString(R.string.boot_image_apply_bump_error);
        case ErrorCode.BOOT_IMAGE_APPLY_LOKI_ERROR:
            return context.getString(R.string.boot_image_apply_loki_error);
        case ErrorCode.CPIO_FILE_ALREADY_EXIST_ERROR:
            return context.getString(R.string.cpio_file_already_exists_error);
        case ErrorCode.CPIO_FILE_NOT_EXIST_ERROR:
            return context.getString(R.string.cpio_file_not_exist_error);
        case ErrorCode.ARCHIVE_READ_OPEN_ERROR:
            return context.getString(R.string.archive_read_open_error);
        case ErrorCode.ARCHIVE_READ_DATA_ERROR:
            return context.getString(R.string.archive_read_data_error);
        case ErrorCode.ARCHIVE_READ_HEADER_ERROR:
            return context.getString(R.string.archive_read_header_error);
        case ErrorCode.ARCHIVE_WRITE_OPEN_ERROR:
            return context.getString(R.string.archive_write_open_error);
        case ErrorCode.ARCHIVE_WRITE_DATA_ERROR:
            return context.getString(R.string.archive_write_data_error);
        case ErrorCode.ARCHIVE_WRITE_HEADER_ERROR:
            return context.getString(R.string.archive_write_header_error);
        case ErrorCode.ARCHIVE_CLOSE_ERROR:
            return context.getString(R.string.archive_close_error);
        case ErrorCode.ARCHIVE_FREE_ERROR:
            return context.getString(R.string.archive_free_error);
        case ErrorCode.XML_PARSE_FILE_ERROR:
            return context.getString(R.string.xml_parse_file_error);
        case ErrorCode.ONLY_ZIP_SUPPORTED:
            return context.getString(R.string.only_zip_supported);
        case ErrorCode.ONLY_BOOT_IMAGE_SUPPORTED:
            return context.getString(R.string.only_boot_image_supported);
        case ErrorCode.PATCHING_CANCELLED:
            return context.getString(R.string.patching_cancelled);
        case ErrorCode.SYSTEM_CACHE_FORMAT_LINES_NOT_FOUND:
            return context.getString(R.string.system_cache_format_lines_not_found);
        case ErrorCode.APPLY_PATCH_FILE_ERROR:
            return context.getString(R.string.apply_patch_file_error);
        default:
            throw new IllegalStateException("Unknown error code!");
        }
    }

    public synchronized static void extractPatcher(Context context) {
        for (File d : context.getCacheDir().listFiles()) {
            if (d.getName().startsWith("DualBootPatcherAndroid")
                    || d.getName().startsWith("tmp")
                    || d.getName().startsWith("data-")) {
                org.apache.commons.io.FileUtils.deleteQuietly(d);
            }
        }
        for (File d : context.getFilesDir().listFiles()) {
            if (d.isDirectory()) {
                for (File t : d.listFiles()) {
                    if (t.getName().contains("tmp")) {
                        org.apache.commons.io.FileUtils.deleteQuietly(t);
                    }
                }
            }
        }

        File targetFile = getTargetFile(context);
        File targetDir = getTargetDirectory(context);

        if (!targetDir.exists()) {
            FileUtils.extractAsset(context, sTargetFile, targetFile);

            // Remove all previous files
            for (File d : context.getFilesDir().listFiles()) {
                org.apache.commons.io.FileUtils.deleteQuietly(d);
            }

            LibMiscStuff.INSTANCE.extract_archive(targetFile.getAbsolutePath(),
                    context.getFilesDir().getAbsolutePath());

            // Delete archive
            targetFile.delete();
        }
    }

    public static class InstallLocation {
        public String id;
        public String name;
        public String description;
    }

    public static InstallLocation[] getInstallLocations(Context context) {
        if (sInstallLocations == null) {
            ArrayList<InstallLocation> locations = new ArrayList<>();

            InstallLocation location = new InstallLocation();
            location.id = "primary";
            location.name = context.getString(R.string.install_location_primary_upgrade);
            location.description = context.getString(R.string.install_location_primary_upgrade_desc);

            locations.add(location);

            location = new InstallLocation();
            location.id = "dual";
            location.name = context.getString(R.string.secondary);
            location.description = String.format(context.getString(R.string.install_location_desc),
                    "/system/multiboot/dual");
            locations.add(location);

            for (int i = 1; i <= 3; i++) {
                location = new InstallLocation();
                location.id = "multi-slot-" + i;
                location.name = String.format(context.getString(R.string.multislot), i);
                location.description = String.format(context.getString(R.string.install_location_desc),
                        "/cache/multiboot/multi-slot-" + i);
                locations.add(location);
            }

            sInstallLocations = locations.toArray(new InstallLocation[locations.size()]);
        }

        return sInstallLocations;
    }

    public static InstallLocation getDataSlotInstallLocation(Context context, String dataSlotId) {
        InstallLocation location = new InstallLocation();
        location.id = getDataSlotRomId(dataSlotId);
        location.name = String.format(context.getString(R.string.dataslot), dataSlotId);
        location.description = String.format(context.getString(R.string.install_location_desc),
                "/data/multiboot/data-slot-" + dataSlotId);
        return location;
    }

    public static String getDataSlotRomId(String dataSlotId) {
        return "data-slot-" + dataSlotId;
    }
}
