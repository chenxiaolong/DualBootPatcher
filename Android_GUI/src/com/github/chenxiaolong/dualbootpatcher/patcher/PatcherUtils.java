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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.content.Context;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.BuildConfig;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandParams;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandRunner;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RootFile;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.FileInfo;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Patcher;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Patcher.ProgressListener;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatcherConfig;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatcherError;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatcherError.ErrorCode;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

public class PatcherUtils {
    public static final String TAG = PatcherUtils.class.getSimpleName();
    private static final String FILENAME = "data-%s.zip";
    private static final String DIRNAME = "data-%s";

    public static final String PARAM_PATCHER = "patcher";
    public static final String PARAM_FILEINFO = "fileinfo";

    public static final String RESULT_PATCH_FILE_NEW_FILE = "new_file";
    public static final String RESULT_PATCH_FILE_MESSAGE = "message";
    public static final String RESULT_PATCH_FILE_FAILED = "failed";

    static PatcherConfig sPC;

    private static String sTargetFile;
    private static String sTargetDir;

    static {
        String version = BuildConfig.VERSION_NAME.split("-")[0];
        sTargetFile = String.format(FILENAME, version);
        sTargetDir = String.format(DIRNAME, version);
    }

    private static File getTargetFile(Context context) {
        return new File(context.getCacheDir() + File.separator + sTargetFile);
    }

    static File getTargetDirectory(Context context) {
        return new File(context.getFilesDir() + File.separator + sTargetDir);
    }

    public synchronized static void initializePatcher(Context context) {
        extractPatcher(context);

        sPC = new PatcherConfig();
        sPC.setDataDirectory(getTargetDirectory(context).getAbsolutePath());
        sPC.setTempDirectory(context.getCacheDir().getAbsolutePath());
        sPC.loadPatchInfos();
        // TODO: Throw exception if patchinfo loading fails
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

    private static String getErrorMessage(Context context, PatcherError error) {
        switch (error.getErrorCode()) {
        case ErrorCode.NO_ERROR:
            return context.getString(R.string.no_error);
        case ErrorCode.UNKNOWN_ERROR:
            return context.getString(R.string.unknown_error);
        case ErrorCode.PATCHER_CREATE_ERROR:
            return String.format(context.getString(R.string.patcher_create_error),
                    error.getPatcherName());
        case ErrorCode.AUTOPATCHER_CREATE_ERROR:
            return String.format(context.getString(R.string.autopatcher_create_error),
                    error.getPatcherName());
        case ErrorCode.RAMDISK_PATCHER_CREATE_ERROR:
            return String.format(context.getString(R.string.ramdisk_patcher_create_error),
                    error.getPatcherName());
        case ErrorCode.FILE_OPEN_ERROR:
            return String.format(context.getString(R.string.file_open_error), error.getFilename());
        case ErrorCode.FILE_READ_ERROR:
            return String.format(context.getString(R.string.file_read_error), error.getFilename());
        case ErrorCode.FILE_WRITE_ERROR:
            return String.format(context.getString(R.string.file_write_error), error.getFilename());
        case ErrorCode.DIRECTORY_NOT_EXIST_ERROR:
            return String.format(context.getString(R.string.directory_not_exist_error),
                    error.getFilename());
        case ErrorCode.BOOT_IMAGE_SMALLER_THAN_HEADER_ERROR:
            return context.getString(R.string.boot_image_smaller_than_header_error);
        case ErrorCode.BOOT_IMAGE_NO_ANDROID_HEADER_ERROR:
            return context.getString(R.string.boot_image_no_android_header_error);
        case ErrorCode.BOOT_IMAGE_NO_RAMDISK_GZIP_HEADER_ERROR:
            return context.getString(R.string.boot_image_no_ramdisk_gzip_header_error);
        case ErrorCode.BOOT_IMAGE_NO_RAMDISK_SIZE_ERROR:
            return context.getString(R.string.boot_image_no_ramdisk_size_error);
        case ErrorCode.BOOT_IMAGE_NO_KERNEL_SIZE_ERROR:
            return context.getString(R.string.boot_image_no_kernel_size_error);
        case ErrorCode.BOOT_IMAGE_NO_RAMDISK_ADDRESS_ERROR:
            return context.getString(R.string.boot_image_no_ramdisk_address_error);
        case ErrorCode.CPIO_FILE_ALREADY_EXIST_ERROR:
            return String.format(context.getString(R.string.cpio_file_already_exists_error),
                    error.getFilename());
        case ErrorCode.CPIO_FILE_NOT_EXIST_ERROR:
            return String.format(context.getString(R.string.cpio_file_not_exist_error),
                    error.getFilename());
        case ErrorCode.ARCHIVE_READ_OPEN_ERROR:
            return context.getString(R.string.archive_read_open_error);
        case ErrorCode.ARCHIVE_READ_DATA_ERROR:
            return String.format(context.getString(R.string.archive_read_data_error),
                    error.getFilename());
        case ErrorCode.ARCHIVE_READ_HEADER_ERROR:
            return context.getString(R.string.archive_read_header_error);
        case ErrorCode.ARCHIVE_WRITE_OPEN_ERROR:
            return context.getString(R.string.archive_write_open_error);
        case ErrorCode.ARCHIVE_WRITE_DATA_ERROR:
            return String.format(context.getString(R.string.archive_write_data_error),
                    error.getFilename());
        case ErrorCode.ARCHIVE_WRITE_HEADER_ERROR:
            return String.format(context.getString(R.string.archive_write_header_error),
                    error.getFilename());
        case ErrorCode.ARCHIVE_CLOSE_ERROR:
            return context.getString(R.string.archive_close_error);
        case ErrorCode.ARCHIVE_FREE_ERROR:
            return context.getString(R.string.archive_free_error);
        case ErrorCode.XML_PARSE_FILE_ERROR:
            return String.format(context.getString(R.string.xml_parse_file_error),
                    error.getFilename());
        case ErrorCode.ONLY_ZIP_SUPPORTED:
            return String.format(context.getString(R.string.only_zip_supported),
                    error.getPatcherName());
        case ErrorCode.ONLY_BOOT_IMAGE_SUPPORTED:
            return String.format(context.getString(R.string.only_boot_image_supported),
                    error.getPatcherName());
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

    public synchronized static boolean updateSyncdaemon(Context context, String bootimg) {
        // TODO: Need to update for new API
        if (Math.sin(0) < 1) {
            return false;
        }

        // Make sure patcher is extracted first
        extractPatcher(context);

        ArrayList<String> args = new ArrayList<String>();
        args.add("pythonportable/bin/python3");
        args.add("-B");
        args.add("scripts/updatesyncdaemon.py");
        args.add(bootimg);

        CommandParams params = new CommandParams();
        params.command = args.toArray(new String[args.size()]);
        params.environment = new String[] { "PYTHONUNBUFFERED=true" };
        params.cwd = getTargetDirectory(context);

        CommandRunner cmd = new CommandRunner(params);
        cmd.start();
        CommandUtils.waitForCommand(cmd);

        if (cmd.getResult() != null) {
            return cmd.getResult().exitCode == 0;
        } else {
            return false;
        }
    }

    public synchronized static boolean isLokiBootImage(Context context, String bootimg) {
        // TODO: Need to fix still
        if (Math.sin(0) < 1) {
            return false;
        }

        // Make sure patcher is extracted first
        extractPatcher(context);

        ArrayList<String> args = new ArrayList<String>();
        args.add("pythonportable/bin/python3");
        args.add("-B");
        args.add("scripts/multiboot/standalone/unlokibootimg.py");
        args.add("-i");
        args.add(bootimg);
        args.add("--isloki");

        CommandParams params = new CommandParams();
        params.command = args.toArray(new String[args.size()]);
        params.environment = new String[] { "PYTHONUNBUFFERED=true" };
        params.cwd = getTargetDirectory(context);

        CommandRunner cmd = new CommandRunner(params);
        cmd.start();
        CommandUtils.waitForCommand(cmd);

        if (cmd.getResult() != null) {
            return cmd.getResult().exitCode == 0;
        } else {
            return false;
        }
    }

    private synchronized static void extractPatcher(Context context) {
        for (File d : context.getCacheDir().listFiles()) {
            if (d.getName().startsWith("DualBootPatcherAndroid")
                    || d.getName().startsWith("tmp")
                    || d.getName().startsWith("data-")) {
                new RootFile(d.getAbsolutePath()).recursiveDelete();
            }
        }
        for (File d : context.getFilesDir().listFiles()) {
            if (d.isDirectory()) {
                for (File t : d.listFiles()) {
                    if (t.getName().contains("tmp")) {
                        new RootFile(t.getAbsolutePath()).recursiveDelete();
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
                new RootFile(d.getAbsolutePath()).recursiveDelete();
            }

            //extractZip(targetFile.getAbsolutePath(), targetDir.getAbsolutePath());
            extractZip(targetFile.getAbsolutePath(), context.getFilesDir().getAbsolutePath());

            // Delete archive
            targetFile.delete();
        }
    }

    /* http://stackoverflow.com/questions/3382996/how-to-unzip-files-programmatically-in-android */
    private static boolean extractZip(String zipPath, String outputDir) {
        FileInputStream fis = null;
        ZipInputStream zis = null;

        try {
            fis = new FileInputStream(zipPath);
            zis = new ZipInputStream(new BufferedInputStream(fis));
            ZipEntry ze;

            byte[] buffer = new byte[1024];
            int length;

            while ((ze = zis.getNextEntry()) != null) {
                File target = new File(outputDir + File.separator + ze.getName());

                if (ze.isDirectory()) {
                    target.mkdirs();
                } else {
                    File parent = target.getParentFile();
                    if (parent != null) {
                        parent.mkdirs();
                    }

                    FileOutputStream fos = null;
                    try {
                        fos = new FileOutputStream(target);

                        while ((length = zis.read(buffer)) >= 0) {
                            fos.write(buffer, 0, length);
                        }

                        fos.close();
                        fos = null;
                    } finally {
                        if (fos != null) {
                            try {
                                fos.close();
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }

                zis.closeEntry();

                if (target.getName().equals("patch")) {
                    new RootFile(target, false).chmod(0700);
                }
            }

            zis.close();

            return true;
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (zis != null) {
                try {
                    zis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            } else if (fis != null) {
                try {
                    fis.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }

        return false;
    }
}
