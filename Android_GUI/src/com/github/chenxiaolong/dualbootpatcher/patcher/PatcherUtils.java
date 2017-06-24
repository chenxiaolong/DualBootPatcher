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
import android.os.Environment;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.BuildConfig;
import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.PatcherConfig;
import com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff.LibMiscStuff;

import org.apache.commons.io.Charsets;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

public class PatcherUtils {
    private static final String TAG = PatcherUtils.class.getSimpleName();
    private static final String FILENAME = "data-%s.tar.xz";
    private static final String DIRNAME = "data-%s";

    private static final String PREFIX_DATA_SLOT = "data-slot-";
    private static final String PREFIX_EXTSD_SLOT = "extsd-slot-";

    public static final String PATCHER_ID_MULTIBOOTPATCHER = "MultiBootPatcher";
    public static final String PATCHER_ID_ODINPATCHER = "OdinPatcher";

    private static boolean sInitialized;

    private static Device[] sDevices;
    private static Device sCurrentDevice;

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
        if (!sInitialized) {
            extractPatcher(context);
            sInitialized = true;
        }
    }

    public static PatcherConfig newPatcherConfig(Context context) {
        PatcherConfig pc = new PatcherConfig();
        pc.setDataDirectory(getTargetDirectory(context).getAbsolutePath());
        pc.setTempDirectory(context.getCacheDir().getAbsolutePath());
        return pc;
    }

    public synchronized static Device[] getDevices(Context context) {
        ThreadUtils.enforceExecutionOnNonMainThread();
        initializePatcher(context);

        if (sDevices == null) {
            File path = new File(getTargetDirectory(context) + File.separator + "devices.json");
            try {
                String json = org.apache.commons.io.FileUtils.readFileToString(
                        path, Charsets.UTF_8);

                Device[] devices = Device.newListFromJson(json);
                ArrayList<Device> validDevices = new ArrayList<>();

                if (devices != null) {
                    for (Device d : devices) {
                        if (d.validate() == 0) {
                            validDevices.add(d);
                        }
                    }
                }

                if (!validDevices.isEmpty()) {
                    sDevices = validDevices.toArray(new Device[validDevices.size()]);
                }
            } catch (IOException e) {
                Log.w(TAG, "Failed to read " + path, e);
            }
        }

        return sDevices;
    }

    public synchronized static Device getCurrentDevice(Context context) {
        ThreadUtils.enforceExecutionOnNonMainThread();

        if (sCurrentDevice == null) {
            String realCodename = RomUtils.getDeviceCodename(context);
            Device[] devices = getDevices(context);

            if (devices != null) {
                outer:
                for (Device d : devices) {
                    for (String codename : d.getCodenames()) {
                        if (realCodename.equals(codename)) {
                            sCurrentDevice = d;
                            break outer;
                        }
                    }
                }
            }
        }

        return sCurrentDevice;
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

        if (BuildConfig.BUILD_TYPE.equals("debug") || !targetDir.exists()) {
            try {
                FileUtils.extractAsset(context, sTargetFile, targetFile);
            } catch (IOException e) {
                throw new IllegalStateException("Failed to extract data archive from assets", e);
            }

            // Remove all previous files
            for (File d : context.getFilesDir().listFiles()) {
                org.apache.commons.io.FileUtils.deleteQuietly(d);
            }

            try {
                LibMiscStuff.extractArchive(targetFile.getAbsolutePath(),
                        context.getFilesDir().getAbsolutePath());
            } catch (IOException e) {
                throw new IllegalStateException("Failed to extract data archive", e);
            }

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

    public static InstallLocation[] getNamedInstallLocations(Context context) {
        ThreadUtils.enforceExecutionOnNonMainThread();

        Log.d(TAG, "Looking for named ROMs");

        File dir = new File(Environment.getExternalStorageDirectory()
                + File.separator + "MultiBoot");

        ArrayList<InstallLocation> locations = new ArrayList<>();

        File[] files = dir.listFiles();
        if (files != null) {
            for (File f : files) {
                String name = f.getName();

                if (name.startsWith("data-slot-") && !name.equals("data-slot-")) {
                    Log.d(TAG, "- Found data-slot: " + name.substring(10));
                    locations.add(getDataSlotInstallLocation(context, name.substring(10)));
                } else if (name.startsWith("extsd-slot-") && !name.equals("extsd-slot-")) {
                    Log.d(TAG, "- Found extsd-slot: " + name.substring(11));
                    locations.add(getExtsdSlotInstallLocation(context, name.substring(11)));
                }
            }
        } else {
            Log.e(TAG, "Failed to list files in: " + dir);
        }

        return locations.toArray(new InstallLocation[locations.size()]);
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
        return PREFIX_DATA_SLOT + dataSlotId;
    }

    public static String getExtsdSlotRomId(String extsdSlotId) {
        return PREFIX_EXTSD_SLOT + extsdSlotId;
    }

    public static boolean isDataSlotRomId(String romId) {
        return romId.startsWith(PREFIX_DATA_SLOT);
    }

    public static boolean isExtsdSlotRomId(String romId) {
        return romId.startsWith(PREFIX_EXTSD_SLOT);
    }

    public static String getDataSlotIdFromRomId(String romId) {
        if (isDataSlotRomId(romId)) {
            return romId.substring(PREFIX_DATA_SLOT.length());
        } else {
            return null;
        }
    }

    public static String getExtsdSlotIdFromRomId(String romId) {
        if (isExtsdSlotRomId(romId)) {
            return romId.substring(PREFIX_EXTSD_SLOT.length());
        } else {
            return null;
        }
    }
}
