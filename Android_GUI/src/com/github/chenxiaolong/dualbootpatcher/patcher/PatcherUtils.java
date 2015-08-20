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
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.BuildConfig;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.LogUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
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
    public static final String RESULT_PATCH_FILE_ERROR_CODE = "error_code";
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
        Log.d(TAG, "Android GUI version: " + BuildConfig.VERSION_NAME);
        Log.d(TAG, "libmbp version: " + PatcherUtils.sPC.getVersion());

        // Make sure patcher is extracted first
        extractPatcher(context);

        Patcher patcher = data.getParcelable(PARAM_PATCHER);
        FileInfo fileInfo = data.getParcelable(PARAM_FILEINFO);

        Log.d(TAG, "Patching file: " + fileInfo.getFilename());

        patcher.setFileInfo(fileInfo);
        boolean ret = patcher.patchFile(listener);
        LogUtils.dump("patch-file.log");

        Bundle bundle = new Bundle();
        bundle.putString(RESULT_PATCH_FILE_NEW_FILE, patcher.newFilePath());
        bundle.putInt(RESULT_PATCH_FILE_ERROR_CODE, patcher.getError());
        bundle.putBoolean(RESULT_PATCH_FILE_FAILED, !ret);
        return bundle;
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
        location.description = context.getString(R.string.install_location_desc,
                "/data/multiboot/data-slot-" + dataSlotId);
        return location;
    }

    public static InstallLocation getExtsdSlotInstallLocation(Context context, String extsdSlotId) {
        InstallLocation location = new InstallLocation();
        location.id = getExtsdSlotRomId(extsdSlotId);
        location.name = String.format(context.getString(R.string.extsdslot), extsdSlotId);
        location.description = context.getString(R.string.install_location_desc,
                "[External SD]/multiboot/extsd-slot-" + extsdSlotId);
        return location;
    }

    public static String getDataSlotRomId(String dataSlotId) {
        return "data-slot-" + dataSlotId;
    }

    public static String getExtsdSlotRomId(String extsdSlotId) {
        return "extsd-slot-" + extsdSlotId;
    }
}
