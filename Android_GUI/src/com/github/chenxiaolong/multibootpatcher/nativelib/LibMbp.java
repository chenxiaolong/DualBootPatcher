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

package com.github.chenxiaolong.multibootpatcher.nativelib;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CAutoPatcher;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CBootImage;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CCpioFile;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CDevice;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CFileInfo;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CPatchInfo;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CPatcher;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CPatcherConfig;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CPatcherError;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CRamdiskPatcher;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.CWrapper.CStringMap;
import com.sun.jna.Callback;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.StringArray;
import com.sun.jna.ptr.IntByReference;
import com.sun.jna.ptr.PointerByReference;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

// NOTE: Almost no checking of parameters is performed on both the Java and C side of this native
//       wrapper. As a rule of thumb, don't pass null to any function.

@SuppressWarnings("unused")
public class LibMbp {
    /** Log (almost) all native method calls and their parameters */
    private static final boolean DEBUG = false;

    static class CWrapper {
        static {
            Native.register(CWrapper.class, "mbp");
        }

        // BEGIN: ctypes.h
        public static class CBootImage extends PointerType {}
        public static class CCpioFile extends PointerType {}
        public static class CDevice extends PointerType {}
        public static class CFileInfo extends PointerType {}
        public static class CPatcherConfig extends PointerType {}
        public static class CPatcherError extends PointerType {}
        public static class CPatchInfo extends PointerType {}
        public static class CStringMap extends PointerType {}

        public static class CPatcher extends PointerType {}
        public static class CAutoPatcher extends PointerType {}
        public static class CRamdiskPatcher extends PointerType {}
        // END: ctypes.h

        // BEGIN: cbootimage.h
        static native CBootImage mbp_bootimage_create();
        static native void mbp_bootimage_destroy(CBootImage bi);
        static native CPatcherError mbp_bootimage_error(CBootImage bi);
        static native boolean mbp_bootimage_load_data(CBootImage bi, Pointer data, int size);
        static native boolean mbp_bootimage_load_file(CBootImage bi, String filename);
        static native void mbp_bootimage_create_data(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference size);
        static native boolean mbp_bootimage_create_file(CBootImage bi, String filename);
        static native boolean mbp_bootimage_was_loki(CBootImage bi);
        static native boolean mbp_bootimage_was_bump(CBootImage bi);
        static native void mbp_bootimage_set_apply_loki(CBootImage bi, boolean apply);
        static native void mbp_bootimage_set_apply_bump(CBootImage bi, boolean apply);
        static native Pointer mbp_bootimage_boardname(CBootImage bi);
        static native void mbp_bootimage_set_boardname(CBootImage bi, String name);
        static native void mbp_bootimage_reset_boardname(CBootImage bi);
        static native Pointer mbp_bootimage_kernel_cmdline(CBootImage bi);
        static native void mbp_bootimage_set_kernel_cmdline(CBootImage bi, String cmdline);
        static native void mbp_bootimage_reset_kernel_cmdline(CBootImage bi);
        static native int mbp_bootimage_page_size(CBootImage bi);
        static native void mbp_bootimage_set_page_size(CBootImage bi, int pageSize);
        static native void mbp_bootimage_reset_page_size(CBootImage bi);
        static native int mbp_bootimage_kernel_address(CBootImage bi);
        static native void mbp_bootimage_set_kernel_address(CBootImage bi, int address);
        static native void mbp_bootimage_reset_kernel_address(CBootImage bi);
        static native int mbp_bootimage_ramdisk_address(CBootImage bi);
        static native void mbp_bootimage_set_ramdisk_address(CBootImage bi, int address);
        static native void mbp_bootimage_reset_ramdisk_address(CBootImage bi);
        static native int mbp_bootimage_second_bootloader_address(CBootImage bi);
        static native void mbp_bootimage_set_second_bootloader_address(CBootImage bi, int address);
        static native void mbp_bootimage_reset_second_bootloader_address(CBootImage bi);
        static native int mbp_bootimage_kernel_tags_address(CBootImage bi);
        static native void mbp_bootimage_set_kernel_tags_address(CBootImage bi, int address);
        static native void mbp_bootimage_reset_kernel_tags_address(CBootImage bi);
        static native void mbp_bootimage_set_addresses(CBootImage bi, int base, int kernelOffset, int ramdiskOffset, int secondBootloaderOffset, int kernelTagsOffset);
        static native /* size_t */ int mbp_bootimage_kernel_image(CBootImage bi, PointerByReference dataReturn);
        static native void mbp_bootimage_set_kernel_image(CBootImage bi, Pointer data, /* size_t */ int size);
        static native /* size_t */ int mbp_bootimage_ramdisk_image(CBootImage bi, PointerByReference dataReturn);
        static native void mbp_bootimage_set_ramdisk_image(CBootImage bi, Pointer data, /* size_t */ int size);
        static native /* size_t */ int mbp_bootimage_second_bootloader_image(CBootImage bi, PointerByReference dataReturn);
        static native void mbp_bootimage_set_second_bootloader_image(CBootImage bi, Pointer data, /* size_t */ int size);
        static native /* size_t */ int mbp_bootimage_device_tree_image(CBootImage bi, PointerByReference dataReturn);
        static native void mbp_bootimage_set_device_tree_image(CBootImage bi, Pointer data, int size);
        static native /* size_t */ int mbp_bootimage_aboot_image(CBootImage bi, PointerByReference dataReturn);
        static native void mbp_bootimage_set_aboot_image(CBootImage bi, Pointer data, int size);
        // END: cbootimage.h

        // BEGIN: ccommon.h
        static native void mbp_free(Pointer data);
        static native void mbp_free_array(Pointer array);
        // END: ccommon.h

        // BEGIN: ccpiofile.h
        static native CCpioFile mbp_cpiofile_create();
        static native void mbp_cpiofile_destroy(CCpioFile cpio);
        static native CPatcherError mbp_cpiofile_error(CCpioFile cpio);
        static native boolean mbp_cpiofile_load_data(CCpioFile cpio, Pointer data, int size);
        static native boolean mbp_cpiofile_create_data(CCpioFile cpio, PointerByReference dataReturn, /* size_t */ IntByReference size);
        static native boolean mbp_cpiofile_exists(CCpioFile cpio, String filename);
        static native boolean mbp_cpiofile_remove(CCpioFile cpio, String filename);
        static native Pointer mbp_cpiofile_filenames(CCpioFile cpio);
        static native boolean mbp_cpiofile_contents(CCpioFile cpio, String filename, PointerByReference dataReturn, /* size_t */ IntByReference size);
        static native boolean mbp_cpiofile_set_contents(CCpioFile cpio, String filename, Pointer data, /* size_t */ int size);
        static native boolean mbp_cpiofile_add_symlink(CCpioFile cpio, String source, String target);
        static native boolean mbp_cpiofile_add_file(CCpioFile cpio, String path, String name, int perms);
        static native boolean mbp_cpiofile_add_file_from_data(CCpioFile cpio, Pointer data, /* size_t */ int size, String name, int perms);
        // END: ccpiofile.h

        // BEGIN: cdevice.h
        static native CDevice mbp_device_create();
        static native void mbp_device_destroy(CDevice device);
        static native Pointer mbp_device_id(CDevice device);
        static native void mbp_device_set_id(CDevice device, String id);
        static native Pointer mbp_device_codenames(CDevice device);
        static native void mbp_device_set_codenames(CDevice device, StringArray names);
        static native Pointer mbp_device_name(CDevice device);
        static native void mbp_device_set_name(CDevice device, String name);
        static native Pointer mbp_device_architecture(CDevice device);
        static native void mbp_device_set_architecture(CDevice device, String arch);
        static native Pointer mbp_device_block_dev_base_dirs(CDevice device);
        static native void mbp_device_set_block_dev_base_dirs(CDevice device, StringArray dirs);
        static native Pointer mbp_device_system_block_devs(CDevice device);
        static native void mbp_device_set_system_block_devs(CDevice device, StringArray block_devs);
        static native Pointer mbp_device_cache_block_devs(CDevice device);
        static native void mbp_device_set_cache_block_devs(CDevice device, StringArray block_devs);
        static native Pointer mbp_device_data_block_devs(CDevice device);
        static native void mbp_device_set_data_block_devs(CDevice device, StringArray block_devs);
        static native Pointer mbp_device_boot_block_devs(CDevice device);
        static native void mbp_device_set_boot_block_devs(CDevice device, StringArray block_devs);
        static native Pointer mbp_device_recovery_block_devs(CDevice device);
        static native void mbp_device_set_recovery_block_devs(CDevice device, StringArray block_devs);
        static native Pointer mbp_device_extra_block_devs(CDevice device);
        static native void mbp_device_set_extra_block_devs(CDevice device, StringArray block_devs);
        // END: cdevice.h

        // BEGIN: cfileinfo.h
        static native CFileInfo mbp_fileinfo_create();
        static native void mbp_fileinfo_destroy(CFileInfo info);
        static native Pointer mbp_fileinfo_filename(CFileInfo info);
        static native void mbp_fileinfo_set_filename(CFileInfo info, String path);
        static native CPatchInfo mbp_fileinfo_patchinfo(CFileInfo info);
        static native void mbp_fileinfo_set_patchinfo(CFileInfo info, CPatchInfo pInfo);
        static native CDevice mbp_fileinfo_device(CFileInfo info);
        static native void mbp_fileinfo_set_device(CFileInfo info, CDevice device);
        // END: cfileinfo.h

        // BEGIN: cpatcherconfig.h
        static native CPatcherConfig mbp_config_create();
        static native void mbp_config_destroy(CPatcherConfig pc);
        static native CPatcherError mbp_config_error(CPatcherConfig pc);
        static native Pointer mbp_config_data_directory(CPatcherConfig pc);
        static native Pointer mbp_config_temp_directory(CPatcherConfig pc);
        static native void mbp_config_set_data_directory(CPatcherConfig pc, String path);
        static native void mbp_config_set_temp_directory(CPatcherConfig pc, String path);
        static native Pointer mbp_config_version(CPatcherConfig pc);
        static native Pointer mbp_config_devices(CPatcherConfig pc);
        static native Pointer mbp_config_patchinfos(CPatcherConfig pc);
        static native Pointer mbp_config_patchinfos_for_device(CPatcherConfig pc, CDevice device);
        static native CPatchInfo mbp_config_find_matching_patchinfo(CPatcherConfig pc, CDevice device, String filename);
        static native Pointer mbp_config_patchers(CPatcherConfig pc);
        static native Pointer mbp_config_autopatchers(CPatcherConfig pc);
        static native Pointer mbp_config_ramdiskpatchers(CPatcherConfig pc);
        static native CPatcher mbp_config_create_patcher(CPatcherConfig pc, String id);
        static native CAutoPatcher mbp_config_create_autopatcher(CPatcherConfig pc, String id, CFileInfo info, CStringMap args);
        static native CRamdiskPatcher mbp_config_create_ramdisk_patcher(CPatcherConfig pc, String id, CFileInfo info, CCpioFile cpio);
        static native void mbp_config_destroy_patcher(CPatcherConfig pc, CPatcher patcher);
        static native void mbp_config_destroy_autopatcher(CPatcherConfig pc, CAutoPatcher patcher);
        static native void mbp_config_destroy_ramdisk_patcher(CPatcherConfig pc, CRamdiskPatcher patcher);
        static native boolean mbp_config_load_patchinfos(CPatcherConfig pc);
        // END: cpatcherconfig.h

        // BEGIN: cpatchererror.h
        static native void mbp_error_destroy(CPatcherError error);
        static native /* ErrorType */ int mbp_error_error_type(CPatcherError error);
        static native /* ErrorCode */ int mbp_error_error_code(CPatcherError error);
        static native Pointer mbp_error_patcher_id(CPatcherError error);
        static native Pointer mbp_error_filename(CPatcherError error);
        // END: cpatchererror.h

        // BEGIN: cpatchinfo.h
        static native CPatchInfo mbp_patchinfo_create();
        static native void mbp_patchinfo_destroy(CPatchInfo info);
        static native Pointer mbp_patchinfo_id(CPatchInfo info);
        static native void mbp_patchinfo_set_id(CPatchInfo info, String id);
        static native Pointer mbp_patchinfo_name(CPatchInfo info);
        static native void mbp_patchinfo_set_name(CPatchInfo info, String name);
        static native Pointer mbp_patchinfo_regexes(CPatchInfo info);
        static native void mbp_patchinfo_set_regexes(CPatchInfo info, StringArray regexes);
        static native Pointer mbp_patchinfo_exclude_regexes(CPatchInfo info);
        static native void mbp_patchinfo_set_exclude_regexes(CPatchInfo info, StringArray regexes);
        static native void mbp_patchinfo_add_autopatcher(CPatchInfo info, String apName, CStringMap args);
        static native void mbp_patchinfo_remove_autopatcher(CPatchInfo info, String apName);
        static native Pointer mbp_patchinfo_autopatchers(CPatchInfo info);
        static native CStringMap mbp_patchinfo_autopatcher_args(CPatchInfo info, String apName);
        static native boolean mbp_patchinfo_has_boot_image(CPatchInfo info);
        static native void mbp_patchinfo_set_has_boot_image(CPatchInfo info, boolean hasBootImage);
        static native boolean mbp_patchinfo_autodetect_boot_images(CPatchInfo info);
        static native void mbp_patchinfo_set_autodetect_boot_images(CPatchInfo info, boolean autoDetect);
        static native Pointer mbp_patchinfo_boot_images(CPatchInfo info);
        static native void mbp_patchinfo_set_boot_images(CPatchInfo info, StringArray bootImages);
        static native Pointer mbp_patchinfo_ramdisk(CPatchInfo info);
        static native void mbp_patchinfo_set_ramdisk(CPatchInfo info, String ramdisk);
        static native boolean mbp_patchinfo_device_check(CPatchInfo info);
        static native void mbp_patchinfo_set_device_check(CPatchInfo info, boolean deviceCheck);
        // END: cpatchinfo.h

        // BEGIN: cstringmap.h
        static native CStringMap mbp_stringmap_create();
        static native void mbp_stringmap_destroy(CStringMap map);
        static native Pointer mbp_stringmap_keys(CStringMap map);
        static native Pointer mbp_stringmap_get(CStringMap map, String key);
        static native void mbp_stringmap_set(CStringMap map, String key, String value);
        static native void mbp_stringmap_remove(CStringMap map, String key);
        static native void mbp_stringmap_clear(CStringMap map);
        // END: cstringmap.h

        // BEGIN: cpatcherinterface.h
        interface MaxProgressUpdatedCallback extends Callback {
            void invoke(int value, Pointer userData);
        }

        interface ProgressUpdatedCallback extends Callback {
            void invoke(int value, Pointer userData);
        }

        interface DetailsUpdatedCallback extends Callback {
            void invoke(String text, Pointer userData);
        }

        static native CPatcherError mbp_patcher_error(CPatcher patcher);
        static native Pointer mbp_patcher_id(CPatcher patcher);
        static native boolean mbp_patcher_uses_patchinfo(CPatcher patcher);
        static native void mbp_patcher_set_fileinfo(CPatcher patcher, CFileInfo info);
        static native Pointer mbp_patcher_new_file_path(CPatcher patcher);
        static native boolean mbp_patcher_patch_file(CPatcher patcher, MaxProgressUpdatedCallback maxProgressCb, ProgressUpdatedCallback progressCb, DetailsUpdatedCallback detailsCb, Pointer userData);
        static native void mbp_patcher_cancel_patching(CPatcher patcher);

        static native CPatcherError mbp_autopatcher_error(CAutoPatcher patcher);
        static native Pointer mbp_autopatcher_id(CAutoPatcher patcher);
        static native Pointer mbp_autopatcher_new_files(CAutoPatcher patcher);
        static native Pointer mbp_autopatcher_existing_files(CAutoPatcher patcher);
        static native boolean mbp_autopatcher_patch_files(CAutoPatcher patcher, String directory);

        static native CPatcherError mbp_ramdiskpatcher_error(CRamdiskPatcher patcher);
        static native Pointer mbp_ramdiskpatcher_id(CRamdiskPatcher patcher);
        static native boolean mbp_ramdiskpatcher_patch_ramdisk(CRamdiskPatcher patcher);
        // END: cpatcherinterface.h
    }

    private static String[] getStringArrayAndFree(Pointer p) {
        String[] array = p.getStringArray(0);
        CWrapper.mbp_free_array(p);
        return array;
    }

    private static String getStringAndFree(Pointer p) {
        String str = p.getString(0);
        CWrapper.mbp_free(p);
        return str;
    }

    private static void ensureNotNull(Object o) {
        if (o == null) {
            throw new NullPointerException();
        }
    }

    private static <T> void incrementRefCount(HashMap<T, Integer> instances, T instance) {
        if (instances.containsKey(instance)) {
            int count = instances.get(instance);
            instances.put(instance, ++count);
        } else {
            instances.put(instance, 1);
        }
    }

    private static <T> boolean decrementRefCount(HashMap<T, Integer> instances, T instance) {
        if (!instances.containsKey(instance)) {
            throw new IllegalStateException("Ref count list does not contain instance");
        }

        int count = instances.get(instance) - 1;
        if (count == 0) {
            // Destroy
            instances.remove(instance);
            return true;
        } else {
            // Decrement
            instances.put(instance, count);
            return false;
        }
    }

    private static String getSig(PointerType ptr, Class clazz, String method, Object... params) {
        StringBuilder sb = new StringBuilder();
        sb.append('(');
        sb.append(ptr.getPointer().getNativeLong(0));
        sb.append(") ");
        sb.append(clazz.getSimpleName());
        sb.append('.');
        sb.append(method);
        sb.append('(');

        for (int i = 0; i < params.length; i++) {
            Object param = params[i];

            if (param instanceof String[]) {
                sb.append(Arrays.toString((String[]) param));
            } else if (param instanceof PointerType) {
                sb.append(((PointerType) param).getPointer().getNativeLong(0));
            } else {
                sb.append(param);
            }

            if (i != params.length - 1) {
                sb.append(", ");
            }
        }

        sb.append(')');

        return sb.toString();
    }

    private static void validate(PointerType ptr, Class clazz, String method, Object... params) {
        String signature = getSig(ptr, clazz, method, params);

        if (DEBUG) {
            Log.d("libmbp", signature);
        }

        if (ptr == null) {
            throw new IllegalStateException("Called on a destroyed object! " + signature);
        }
    }

    public static class BootImage implements Parcelable {
        private static final HashMap<CBootImage, Integer> sInstances = new HashMap<>();
        private CBootImage mCBootImage;

        public BootImage() {
            mCBootImage = CWrapper.mbp_bootimage_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCBootImage);
            }
            validate(mCBootImage, BootImage.class, "(Constructor)");
        }

        BootImage(CBootImage cBootImage) {
            ensureNotNull(cBootImage);

            mCBootImage = cBootImage;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCBootImage);
            }
            validate(mCBootImage, BootImage.class, "(Constructor)");
        }

        public void destroy() {
            validate(mCBootImage, BootImage.class, "destroy");
            synchronized (sInstances) {
                if (mCBootImage != null && decrementRefCount(sInstances, mCBootImage)) {
                    validate(mCBootImage, BootImage.class, "(Destroyed)");
                    CWrapper.mbp_bootimage_destroy(mCBootImage);
                }
                mCBootImage = null;
            }
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof BootImage && mCBootImage.equals(((BootImage) o).mCBootImage);
        }

        @Override
        public int hashCode() {
            return mCBootImage.hashCode();
        }

        @Override
        protected void finalize() throws Throwable {
            destroy();
            super.finalize();
        }

        CBootImage getPointer() {
            validate(mCBootImage, BootImage.class, "getPointer");
            return mCBootImage;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCBootImage.getPointer());
            out.writeLong(peer);
        }

        private BootImage(Parcel in) {
            long peer = in.readLong();
            mCBootImage = new CBootImage();
            mCBootImage.setPointer(new Pointer(peer));
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCBootImage);
            }
            validate(mCBootImage, BootImage.class, "(Constructor)");
        }

        public static final Parcelable.Creator<BootImage> CREATOR
                = new Parcelable.Creator<BootImage>() {
            public BootImage createFromParcel(Parcel in) {
                return new BootImage(in);
            }

            public BootImage[] newArray(int size) {
                return new BootImage[size];
            }
        };

        public PatcherError getError() {
            validate(mCBootImage, BootImage.class, "getError");
            CPatcherError error = CWrapper.mbp_bootimage_error(mCBootImage);
            return new PatcherError(error);
        }

        public boolean load(byte[] data) {
            validate(mCBootImage, BootImage.class, "load", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            return CWrapper.mbp_bootimage_load_data(mCBootImage, mem, data.length);
        }

        public boolean load(String filename) {
            validate(mCBootImage, BootImage.class, "load", filename);
            ensureNotNull(filename);

            return CWrapper.mbp_bootimage_load_file(mCBootImage, filename);
        }

        public byte[] create() {
            validate(mCBootImage, BootImage.class, "create");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();

            CWrapper.mbp_bootimage_create_data(mCBootImage, pData, pSize);

            int size = pSize.getValue();
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public boolean createFile(String path) {
            validate(mCBootImage, BootImage.class, "createFile", path);
            ensureNotNull(path);

            return CWrapper.mbp_bootimage_create_file(mCBootImage, path);
        }

        public boolean wasLoki() {
            validate(mCBootImage, BootImage.class, "wasLoki");

            return CWrapper.mbp_bootimage_was_loki(mCBootImage);
        }

        public boolean wasBump() {
            validate(mCBootImage, BootImage.class, "wasBump");

            return CWrapper.mbp_bootimage_was_bump(mCBootImage);
        }

        public void setApplyLoki(boolean apply) {
            validate(mCBootImage, BootImage.class, "setApplyLoki", apply);

            CWrapper.mbp_bootimage_set_apply_loki(mCBootImage, apply);
        }

        public void setApplyBump(boolean apply) {
            validate(mCBootImage, BootImage.class, "setApplyBump", apply);

            CWrapper.mbp_bootimage_set_apply_bump(mCBootImage, apply);
        }

        public String getBoardName() {
            validate(mCBootImage, BootImage.class, "getBoardName");
            Pointer p = CWrapper.mbp_bootimage_boardname(mCBootImage);
            return getStringAndFree(p);
        }

        public void setBoardName(String name) {
            validate(mCBootImage, BootImage.class, "setBoardName", name);
            ensureNotNull(name);

            CWrapper.mbp_bootimage_set_boardname(mCBootImage, name);
        }

        public void resetBoardName() {
            validate(mCBootImage, BootImage.class, "resetBoardName");
            CWrapper.mbp_bootimage_reset_boardname(mCBootImage);
        }

        public String getKernelCmdline() {
            validate(mCBootImage, BootImage.class, "getKernelCmdline");
            Pointer p = CWrapper.mbp_bootimage_kernel_cmdline(mCBootImage);
            return getStringAndFree(p);
        }

        public void setKernelCmdline(String cmdline) {
            validate(mCBootImage, BootImage.class, "setKernelCmdline", cmdline);
            ensureNotNull(cmdline);

            CWrapper.mbp_bootimage_set_kernel_cmdline(mCBootImage, cmdline);
        }

        public void resetKernelCmdline() {
            validate(mCBootImage, BootImage.class, "resetKernelCmdline");
            CWrapper.mbp_bootimage_reset_kernel_cmdline(mCBootImage);
        }

        public int getPageSize() {
            validate(mCBootImage, BootImage.class, "getPageSize");
            return CWrapper.mbp_bootimage_page_size(mCBootImage);
        }

        public void setPageSize(int size) {
            validate(mCBootImage, BootImage.class, "setPageSize", size);
            CWrapper.mbp_bootimage_set_page_size(mCBootImage, size);
        }

        public void resetPageSize() {
            validate(mCBootImage, BootImage.class, "resetPageSize");
            CWrapper.mbp_bootimage_reset_page_size(mCBootImage);
        }

        public int getKernelAddress() {
            validate(mCBootImage, BootImage.class, "getKernelAddress");
            return CWrapper.mbp_bootimage_kernel_address(mCBootImage);
        }

        public void setKernelAddress(int address) {
            validate(mCBootImage, BootImage.class, "setKernelAddress", address);
            CWrapper.mbp_bootimage_set_kernel_address(mCBootImage, address);
        }

        public void resetKernelAddress() {
            validate(mCBootImage, BootImage.class, "resetKernelAddress");
            CWrapper.mbp_bootimage_reset_kernel_address(mCBootImage);
        }

        public int getRamdiskAddress() {
            validate(mCBootImage, BootImage.class, "getRamdiskAddress");
            return CWrapper.mbp_bootimage_ramdisk_address(mCBootImage);
        }

        public void setRamdiskAddress(int address) {
            validate(mCBootImage, BootImage.class, "setRamdiskAddress", address);
            CWrapper.mbp_bootimage_set_ramdisk_address(mCBootImage, address);
        }

        public void resetRamdiskAddress() {
            validate(mCBootImage, BootImage.class, "resetRamdiskAddress");
            CWrapper.mbp_bootimage_reset_ramdisk_address(mCBootImage);
        }

        public int getSecondBootloaderAddress() {
            validate(mCBootImage, BootImage.class, "getSecondBootloaderAddress");
            return CWrapper.mbp_bootimage_second_bootloader_address(mCBootImage);
        }

        public void setSecondBootloaderAddress(int address) {
            validate(mCBootImage, BootImage.class, "setSecondBootloaderAddress", address);
            CWrapper.mbp_bootimage_set_second_bootloader_address(mCBootImage, address);
        }

        public void resetSecondBootloaderAddress() {
            validate(mCBootImage, BootImage.class, "resetSecondBootloaderAddress");
            CWrapper.mbp_bootimage_reset_second_bootloader_address(mCBootImage);
        }

        public int getKernelTagsAddress() {
            validate(mCBootImage, BootImage.class, "getKernelTagsAddress");
            return CWrapper.mbp_bootimage_kernel_tags_address(mCBootImage);
        }

        public void setKernelTagsAddress(int address) {
            validate(mCBootImage, BootImage.class, "setKernelTagsAddress", address);
            CWrapper.mbp_bootimage_set_kernel_tags_address(mCBootImage, address);
        }

        public void resetKernelTagsAddress() {
            validate(mCBootImage, BootImage.class, "resetKernelTagsAddress");
            CWrapper.mbp_bootimage_reset_kernel_tags_address(mCBootImage);
        }

        public void setAddresses(int base, int kernelOffset, int ramdiskOffset,
                                 int secondBootloaderOffset, int kernelTagsOffset) {
            validate(mCBootImage, BootImage.class, "setAddresses", base, kernelOffset,
                    ramdiskOffset, secondBootloaderOffset, kernelTagsOffset);
            CWrapper.mbp_bootimage_set_addresses(mCBootImage, base, kernelOffset,
                    ramdiskOffset, secondBootloaderOffset, kernelTagsOffset);
        }

        public byte[] getKernelImage() {
            validate(mCBootImage, BootImage.class, "getKernelImage");
            PointerByReference pData = new PointerByReference();
            int size = CWrapper.mbp_bootimage_kernel_image(mCBootImage, pData);
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public void setKernelImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setKernelImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_kernel_image(mCBootImage, mem, data.length);
        }

        public byte[] getRamdiskImage() {
            validate(mCBootImage, BootImage.class, "getRamdiskImage");
            PointerByReference pData = new PointerByReference();
            int size = CWrapper.mbp_bootimage_ramdisk_image(mCBootImage, pData);
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public void setRamdiskImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setRamdiskImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_ramdisk_image(mCBootImage, mem, data.length);
        }

        public byte[] getSecondBootloaderImage() {
            validate(mCBootImage, BootImage.class, "getSecondBootloaderImage");
            PointerByReference pData = new PointerByReference();
            int size = CWrapper.mbp_bootimage_second_bootloader_image(mCBootImage, pData);
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public void setSecondBootloaderImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setSecondBootloaderImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_second_bootloader_image(mCBootImage, mem, data.length);
        }

        public byte[] getDeviceTreeImage() {
            validate(mCBootImage, BootImage.class, "getDeviceTreeImage");
            PointerByReference pData = new PointerByReference();
            int size = CWrapper.mbp_bootimage_device_tree_image(mCBootImage, pData);
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public void setDeviceTreeImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setDeviceTreeImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_device_tree_image(mCBootImage, mem, data.length);
        }

        public byte[] getAbootImage() {
            validate(mCBootImage, BootImage.class, "getAbootImage");
            PointerByReference pData = new PointerByReference();
            int size = CWrapper.mbp_bootimage_aboot_image(mCBootImage, pData);
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public void setAbootImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setAbootImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_aboot_image(mCBootImage, mem, data.length);
        }
    }

    public static class CpioFile implements Parcelable {
        private static final HashMap<CCpioFile, Integer> sInstances = new HashMap<>();
        private CCpioFile mCCpioFile;

        public CpioFile() {
            mCCpioFile = CWrapper.mbp_cpiofile_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCCpioFile);
            }
            validate(mCCpioFile, CpioFile.class, "(Constructor)");
        }

        CpioFile(CCpioFile cCpioFile) {
            ensureNotNull(cCpioFile);

            mCCpioFile = cCpioFile;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCCpioFile);
            }
            validate(mCCpioFile, CpioFile.class, "(Constructor)");
        }

        public void destroy() {
            validate(mCCpioFile, CpioFile.class, "destroy");
            synchronized (sInstances) {
                if (mCCpioFile != null && decrementRefCount(sInstances, mCCpioFile)) {
                    validate(mCCpioFile, CpioFile.class, "(Destroyed)");
                    CWrapper.mbp_cpiofile_destroy(mCCpioFile);
                }
            }
            mCCpioFile = null;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof CpioFile && mCCpioFile.equals(((CpioFile) o).mCCpioFile);
        }

        @Override
        public int hashCode() {
            return mCCpioFile.hashCode();
        }

        @Override
        protected void finalize() throws Throwable {
            destroy();
            super.finalize();
        }

        CCpioFile getPointer() {
            validate(mCCpioFile, CpioFile.class, "getPointer");
            return mCCpioFile;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCCpioFile.getPointer());
            out.writeLong(peer);
        }

        private CpioFile(Parcel in) {
            long peer = in.readLong();
            mCCpioFile = new CCpioFile();
            mCCpioFile.setPointer(new Pointer(peer));
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCCpioFile);
            }
            validate(mCCpioFile, CpioFile.class, "(Constructor)");
        }

        public static final Parcelable.Creator<CpioFile> CREATOR
                = new Parcelable.Creator<CpioFile>() {
            public CpioFile createFromParcel(Parcel in) {
                return new CpioFile(in);
            }

            public CpioFile[] newArray(int size) {
                return new CpioFile[size];
            }
        };

        public PatcherError getError() {
            validate(mCCpioFile, CpioFile.class, "getError");
            CPatcherError error = CWrapper.mbp_cpiofile_error(mCCpioFile);
            return new PatcherError(error);
        }

        public boolean load(byte[] data) {
            validate(mCCpioFile, CpioFile.class, "load", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            return CWrapper.mbp_cpiofile_load_data(mCCpioFile, mem, data.length);
        }

        public byte[] createData() {
            validate(mCCpioFile, CpioFile.class, "createData");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();

            boolean ret = CWrapper.mbp_cpiofile_create_data(mCCpioFile, pData, pSize);

            if (!ret) {
                return null;
            }

            int size = pSize.getValue();
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public boolean isExists(String name) {
            validate(mCCpioFile, CpioFile.class, "isExists", name);
            ensureNotNull(name);

            return CWrapper.mbp_cpiofile_exists(mCCpioFile, name);
        }

        public boolean remove(String name) {
            validate(mCCpioFile, CpioFile.class, "remove", name);
            ensureNotNull(name);

            return CWrapper.mbp_cpiofile_remove(mCCpioFile, name);
        }

        public String[] getFilenames() {
            validate(mCCpioFile, CpioFile.class, "getFilenames");
            Pointer p = CWrapper.mbp_cpiofile_filenames(mCCpioFile);
            return getStringArrayAndFree(p);
        }

        public byte[] getContents(String name) {
            validate(mCCpioFile, CpioFile.class, "getContents", name);
            ensureNotNull(name);

            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();

            boolean ret = CWrapper.mbp_cpiofile_contents(mCCpioFile, name, pData, pSize);

            if (!ret) {
                return null;
            }

            int size = pSize.getValue();
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public boolean setContents(String name, byte[] data) {
            validate(mCCpioFile, CpioFile.class, "setContents", name, data.length);
            ensureNotNull(name);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            return CWrapper.mbp_cpiofile_set_contents(mCCpioFile, name, mem, data.length);
        }

        public boolean addSymlink(String source, String target) {
            validate(mCCpioFile, CpioFile.class, "addSymlink", source, target);
            ensureNotNull(source);
            ensureNotNull(target);

            return CWrapper.mbp_cpiofile_add_symlink(mCCpioFile, source, target);
        }

        public boolean addFile(String path, String name, int perms) {
            validate(mCCpioFile, CpioFile.class, "addFile", path, name, perms);
            ensureNotNull(path);
            ensureNotNull(name);

            return CWrapper.mbp_cpiofile_add_file(mCCpioFile, path, name, perms);
        }

        public boolean addFile(byte[] contents, String name, int perms) {
            validate(mCCpioFile, CpioFile.class, "addFile", contents.length, name, perms);
            ensureNotNull(contents);
            ensureNotNull(name);

            Memory mem = new Memory(contents.length);
            mem.write(0, contents, 0, contents.length);
            return CWrapper.mbp_cpiofile_add_file_from_data(
                    mCCpioFile, mem, contents.length, name, perms);
        }
    }

    public static class Device implements Parcelable {
        private static final HashMap<CDevice, Integer> sInstances = new HashMap<>();
        private CDevice mCDevice;
        private boolean mDestroyable;

        public Device() {
            mCDevice = CWrapper.mbp_device_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCDevice);
            }
            mDestroyable = true;
            validate(mCDevice, Device.class, "(Constructor)");
        }

        Device(CDevice cDevice, boolean destroyable) {
            ensureNotNull(cDevice);

            mCDevice = cDevice;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCDevice);
            }
            mDestroyable = destroyable;
            validate(mCDevice, Device.class, "(Constructor)");
        }

        public void destroy() {
            validate(mCDevice, Device.class, "destroy");
            synchronized (sInstances) {
                if (mCDevice != null && decrementRefCount(sInstances, mCDevice) && mDestroyable) {
                    validate(mCDevice, Device.class, "(Destroyed)");
                    CWrapper.mbp_device_destroy(mCDevice);
                }
            }
            mCDevice = null;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof Device && mCDevice.equals(((Device) o).mCDevice);
        }

        @Override
        public int hashCode() {
            int hashCode = 1;
            hashCode = 31 * hashCode + mCDevice.hashCode();
            hashCode = 31 * hashCode + (mDestroyable ? 1 : 0);
            return hashCode;
        }

        @Override
        protected void finalize() throws Throwable {
            destroy();
            super.finalize();
        }

        CDevice getPointer() {
            validate(mCDevice, Device.class, "getPointer");
            return mCDevice;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCDevice.getPointer());
            out.writeLong(peer);
            out.writeInt(mDestroyable ? 1 : 0);
        }

        private Device(Parcel in) {
            long peer = in.readLong();
            mCDevice = new CDevice();
            mCDevice.setPointer(new Pointer(peer));
            mDestroyable = in.readInt() != 0;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCDevice);
            }
            validate(mCDevice, Device.class, "(Constructor)");
        }

        public static final Parcelable.Creator<Device> CREATOR
                = new Parcelable.Creator<Device>() {
            public Device createFromParcel(Parcel in) {
                return new Device(in);
            }

            public Device[] newArray(int size) {
                return new Device[size];
            }
        };

        public String getId() {
            validate(mCDevice, Device.class, "getId");
            Pointer p = CWrapper.mbp_device_id(mCDevice);
            return getStringAndFree(p);
        }

        public void setId(String id) {
            validate(mCDevice, Device.class, "setId", id);
            ensureNotNull(id);

            CWrapper.mbp_device_set_id(mCDevice, id);
        }

        public String[] getCodenames() {
            validate(mCDevice, Device.class, "getCodenames");
            Pointer p = CWrapper.mbp_device_codenames(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setCodenames(String[] names) {
            validate(mCDevice, Device.class, "setCodenames", (Object) names);
            ensureNotNull(names);

            CWrapper.mbp_device_set_codenames(mCDevice, new StringArray(names));
        }

        public String getName() {
            validate(mCDevice, Device.class, "getName");
            Pointer p = CWrapper.mbp_device_name(mCDevice);
            return getStringAndFree(p);
        }

        public void setName(String name) {
            validate(mCDevice, Device.class, "setName", name);
            ensureNotNull(name);

            CWrapper.mbp_device_set_name(mCDevice, name);
        }

        public String getArchitecture() {
            validate(mCDevice, Device.class, "getArchitecture");
            Pointer p = CWrapper.mbp_device_architecture(mCDevice);
            return getStringAndFree(p);
        }

        public void setArchitecture(String arch) {
            validate(mCDevice, Device.class, "setArchitecture", arch);
            ensureNotNull(arch);

            CWrapper.mbp_device_set_architecture(mCDevice, arch);
        }

        public String[] getBlockDevBaseDirs() {
            validate(mCDevice, Device.class, "getBlockDevBaseDirs");
            Pointer p = CWrapper.mbp_device_block_dev_base_dirs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setBlockDevBaseDirs(String[] dirs) {
            validate(mCDevice, Device.class, "setBlockDevBaseDirs", (Object) dirs);
            ensureNotNull(dirs);

            CWrapper.mbp_device_set_block_dev_base_dirs(mCDevice, new StringArray(dirs));
        }

        public String[] getSystemBlockDevs() {
            validate(mCDevice, Device.class, "getSystemBlockDevs");
            Pointer p = CWrapper.mbp_device_system_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setSystemBlockDevs(String[] blockDevs) {
            validate(mCDevice, Device.class, "setSystemBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_system_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getCacheBlockDevs() {
            validate(mCDevice, Device.class, "getCacheBlockDevs");
            Pointer p = CWrapper.mbp_device_cache_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setCacheBlockDevs(String[] blockDevs) {
            validate(mCDevice, Device.class, "setCacheBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_cache_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getDataBlockDevs() {
            validate(mCDevice, Device.class, "getDataBlockDevs");
            Pointer p = CWrapper.mbp_device_data_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setDataBlockDevs(String[] blockDevs) {
            validate(mCDevice, Device.class, "setDataBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_data_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getBootBlockDevs() {
            validate(mCDevice, Device.class, "getBootBlockDevs");
            Pointer p = CWrapper.mbp_device_boot_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setBootBlockDevs(String[] blockDevs) {
            validate(mCDevice, Device.class, "setBootBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_boot_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getRecoveryBlockDevs() {
            validate(mCDevice, Device.class, "getRecoveryBlockDevs");
            Pointer p = CWrapper.mbp_device_recovery_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setRecoveryBlockDevs(String[] blockDevs) {
            validate(mCDevice, Device.class, "setRecoveryBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_recovery_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getExtraBlockDevs() {
            validate(mCDevice, Device.class, "getExtraBlockDevs");
            Pointer p = CWrapper.mbp_device_extra_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setExtraBlockDevs(String[] blockDevs) {
            validate(mCDevice, Device.class, "setExtraBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_extra_block_devs(mCDevice, new StringArray(blockDevs));
        }
    }

    public static class FileInfo implements Parcelable {
        private static final HashMap<CFileInfo, Integer> sInstances = new HashMap<>();
        private CFileInfo mCFileInfo;

        public FileInfo() {
            mCFileInfo = CWrapper.mbp_fileinfo_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCFileInfo);
            }
            validate(mCFileInfo, FileInfo.class, "(Constructor)");
        }

        FileInfo(CFileInfo cFileInfo) {
            ensureNotNull(cFileInfo);

            mCFileInfo = cFileInfo;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCFileInfo);
            }
            validate(mCFileInfo, FileInfo.class, "(Constructor)");
        }

        public void destroy() {
            validate(mCFileInfo, FileInfo.class, "destroy");
            synchronized (sInstances) {
                if (mCFileInfo != null && decrementRefCount(sInstances, mCFileInfo)) {
                    validate(mCFileInfo, FileInfo.class, "(Destroyed)");
                    CWrapper.mbp_fileinfo_destroy(mCFileInfo);
                }
            }
            mCFileInfo = null;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof FileInfo && mCFileInfo.equals(((FileInfo) o).mCFileInfo);
        }

        @Override
        public int hashCode() {
            return mCFileInfo.hashCode();
        }

        @Override
        protected void finalize() throws Throwable {
            destroy();
            super.finalize();
        }

        CFileInfo getPointer() {
            validate(mCFileInfo, FileInfo.class, "getPointer");
            return mCFileInfo;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCFileInfo.getPointer());
            out.writeLong(peer);
        }

        private FileInfo(Parcel in) {
            long peer = in.readLong();
            mCFileInfo = new CFileInfo();
            mCFileInfo.setPointer(new Pointer(peer));
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCFileInfo);
            }
            validate(mCFileInfo, FileInfo.class, "(Constructor)");
        }

        public static final Parcelable.Creator<FileInfo> CREATOR
                = new Parcelable.Creator<FileInfo>() {
            public FileInfo createFromParcel(Parcel in) {
                return new FileInfo(in);
            }

            public FileInfo[] newArray(int size) {
                return new FileInfo[size];
            }
        };

        public String getFilename() {
            validate(mCFileInfo, FileInfo.class, "getFilename");
            Pointer p = CWrapper.mbp_fileinfo_filename(mCFileInfo);
            return getStringAndFree(p);
        }

        public void setFilename(String path) {
            validate(mCFileInfo, FileInfo.class, "setFilename", path);
            ensureNotNull(path);

            CWrapper.mbp_fileinfo_set_filename(mCFileInfo, path);
        }

        public PatchInfo getPatchInfo() {
            validate(mCFileInfo, FileInfo.class, "getPatchInfo");
            CPatchInfo cPi = CWrapper.mbp_fileinfo_patchinfo(mCFileInfo);
            return cPi == null ? null : new PatchInfo(cPi, false);
        }

        public void setPatchInfo(PatchInfo info) {
            validate(mCFileInfo, FileInfo.class, "setPatchInfo", info);
            ensureNotNull(info);

            CWrapper.mbp_fileinfo_set_patchinfo(mCFileInfo, info.getPointer());
        }

        public Device getDevice() {
            validate(mCFileInfo, FileInfo.class, "getDevice");
            CDevice cDevice = CWrapper.mbp_fileinfo_device(mCFileInfo);
            return cDevice == null ? null : new Device(cDevice, false);
        }

        public void setDevice(Device device) {
            validate(mCFileInfo, FileInfo.class, "setDevice", device);
            ensureNotNull(device);

            CWrapper.mbp_fileinfo_set_device(mCFileInfo, device.getPointer());
        }
    }

    public static class PatcherConfig implements Parcelable {
        private static final HashMap<CPatcherConfig, Integer> sInstances = new HashMap<>();
        private CPatcherConfig mCPatcherConfig;

        public PatcherConfig() {
            mCPatcherConfig = CWrapper.mbp_config_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatcherConfig);
            }
            validate(mCPatcherConfig, PatcherConfig.class, "(Constructor)");
        }

        PatcherConfig(CPatcherConfig cPatcherConfig) {
            ensureNotNull(cPatcherConfig);

            mCPatcherConfig = cPatcherConfig;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatcherConfig);
            }
            validate(mCPatcherConfig, PatcherConfig.class, "(Constructor)");
        }

        public void destroy() {
            validate(mCPatcherConfig, PatcherConfig.class, "destroy");
            synchronized (sInstances) {
                if (mCPatcherConfig != null && decrementRefCount(sInstances, mCPatcherConfig)) {
                    validate(mCPatcherConfig, PatcherConfig.class, "(Destroyed)");
                    CWrapper.mbp_config_destroy(mCPatcherConfig);
                }
                mCPatcherConfig = null;
            }
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof PatcherConfig
                    && mCPatcherConfig.equals(((PatcherConfig) o).mCPatcherConfig);
        }

        @Override
        public int hashCode() {
            return mCPatcherConfig.hashCode();
        }

        @Override
        protected void finalize() throws Throwable {
            destroy();
            super.finalize();
        }

        CPatcherConfig getPointer() {
            validate(mCPatcherConfig, PatcherConfig.class, "getPointer");
            return mCPatcherConfig;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCPatcherConfig.getPointer());
            out.writeLong(peer);
        }

        private PatcherConfig(Parcel in) {
            long peer = in.readLong();
            mCPatcherConfig = new CPatcherConfig();
            mCPatcherConfig.setPointer(new Pointer(peer));
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatcherConfig);
            }
            validate(mCPatcherConfig, PatcherConfig.class, "(Constructor)");
        }

        public static final Parcelable.Creator<PatcherConfig> CREATOR
                = new Parcelable.Creator<PatcherConfig>() {
            public PatcherConfig createFromParcel(Parcel in) {
                return new PatcherConfig(in);
            }

            public PatcherConfig[] newArray(int size) {
                return new PatcherConfig[size];
            }
        };

        public PatcherError getError() {
            validate(mCPatcherConfig, PatcherConfig.class, "getError");
            CPatcherError error = CWrapper.mbp_config_error(mCPatcherConfig);
            return new PatcherError(error);
        }

        public String getDataDirectory() {
            validate(mCPatcherConfig, PatcherConfig.class, "getDataDirectory");
            Pointer p = CWrapper.mbp_config_data_directory(mCPatcherConfig);
            return getStringAndFree(p);
        }

        public String getTempDirectory() {
            validate(mCPatcherConfig, PatcherConfig.class, "getTempDirectory");
            Pointer p = CWrapper.mbp_config_temp_directory(mCPatcherConfig);
            return getStringAndFree(p);
        }

        public void setDataDirectory(String path) {
            validate(mCPatcherConfig, PatcherConfig.class, "setDataDirectory", path);
            ensureNotNull(path);

            CWrapper.mbp_config_set_data_directory(mCPatcherConfig, path);
        }

        public void setTempDirectory(String path) {
            validate(mCPatcherConfig, PatcherConfig.class, "setTempDirectory", path);
            ensureNotNull(path);

            CWrapper.mbp_config_set_temp_directory(mCPatcherConfig, path);
        }

        public String getVersion() {
            validate(mCPatcherConfig, PatcherConfig.class, "getVersion");
            Pointer p = CWrapper.mbp_config_version(mCPatcherConfig);
            return getStringAndFree(p);
        }

        public Device[] getDevices() {
            validate(mCPatcherConfig, PatcherConfig.class, "getDevices");
            Pointer p = CWrapper.mbp_config_devices(mCPatcherConfig);
            Pointer[] ps = p.getPointerArray(0);

            Device[] devices = new Device[ps.length];

            for (int i = 0; i < devices.length; i++) {
                CDevice cDevice = new CDevice();
                cDevice.setPointer(ps[i]);
                devices[i] = new Device(cDevice, false);
            }

            return devices;
        }

        public PatchInfo[] getPatchInfos() {
            validate(mCPatcherConfig, PatcherConfig.class, "getPatchInfos");
            Pointer p = CWrapper.mbp_config_patchinfos(mCPatcherConfig);
            Pointer[] ps = p.getPointerArray(0);

            PatchInfo[] infos = new PatchInfo[ps.length];

            for (int i = 0; i < infos.length; i++) {
                CPatchInfo cInfo = new CPatchInfo();
                cInfo.setPointer(ps[i]);
                infos[i] = new PatchInfo(cInfo, false);
            }

            return infos;
        }

        public PatchInfo[] getPatchInfos(Device device) {
            validate(mCPatcherConfig, PatcherConfig.class, "getPatchInfos", device);
            ensureNotNull(device);

            Pointer p = CWrapper.mbp_config_patchinfos_for_device(
                    mCPatcherConfig, device.getPointer());
            Pointer[] ps = p.getPointerArray(0);

            PatchInfo[] infos = new PatchInfo[ps.length];

            for (int i = 0; i < infos.length; i++) {
                CPatchInfo cInfo = new CPatchInfo();
                cInfo.setPointer(ps[i]);
                infos[i] = new PatchInfo(cInfo, false);
            }

            return infos;
        }

        public PatchInfo findMatchingPatchInfo(Device device, String filename) {
            validate(mCPatcherConfig, PatcherConfig.class, "findMatchingPatchInfo", device,
                    filename);
            ensureNotNull(device);
            ensureNotNull(filename);

            CPatchInfo cPi = CWrapper.mbp_config_find_matching_patchinfo(
                    mCPatcherConfig, device.getPointer(), filename);
            return cPi == null ? null : new PatchInfo(cPi, false);
        }

        public String[] getPatchers() {
            validate(mCPatcherConfig, PatcherConfig.class, "getPatchers");
            Pointer p = CWrapper.mbp_config_patchers(mCPatcherConfig);
            return getStringArrayAndFree(p);
        }

        public String[] getAutoPatchers() {
            validate(mCPatcherConfig, PatcherConfig.class, "getAutoPatchers");
            Pointer p = CWrapper.mbp_config_autopatchers(mCPatcherConfig);
            return getStringArrayAndFree(p);
        }

        public String[] getRamdiskPatchers() {
            validate(mCPatcherConfig, PatcherConfig.class, "getRamdiskPatchers");
            Pointer p = CWrapper.mbp_config_ramdiskpatchers(mCPatcherConfig);
            return getStringArrayAndFree(p);
        }

        public Patcher createPatcher(String id) {
            validate(mCPatcherConfig, PatcherConfig.class, "createPatcher", id);
            ensureNotNull(id);

            CPatcher patcher = CWrapper.mbp_config_create_patcher(mCPatcherConfig, id);
            return patcher == null ? null : new Patcher(patcher);
        }

        public AutoPatcher createAutoPatcher(String id, FileInfo info, StringMap args) {
            validate(mCPatcherConfig, PatcherConfig.class, "createAutoPatcher", id, info, args);
            ensureNotNull(id);
            ensureNotNull(info);
            ensureNotNull(args);

            CAutoPatcher patcher = CWrapper.mbp_config_create_autopatcher(
                    mCPatcherConfig, id, info.getPointer(), args.getPointer());
            return patcher == null ? null : new AutoPatcher(patcher);
        }

        public RamdiskPatcher createRamdiskPatcher(String id, FileInfo info, CpioFile cpio) {
            validate(mCPatcherConfig, PatcherConfig.class, "createRamdiskPatcher", id, info, cpio);
            ensureNotNull(id);
            ensureNotNull(info);
            ensureNotNull(cpio);

            CRamdiskPatcher patcher = CWrapper.mbp_config_create_ramdisk_patcher(
                    mCPatcherConfig, id, info.getPointer(), cpio.getPointer());
            return patcher == null ? null : new RamdiskPatcher(patcher);
        }

        public void destroyPatcher(Patcher patcher) {
            validate(mCPatcherConfig, PatcherConfig.class, "destroyPatcher", patcher);
            ensureNotNull(patcher);

            CWrapper.mbp_config_destroy_patcher(mCPatcherConfig, patcher.getPointer());
        }

        public void destroyAutoPatcher(AutoPatcher patcher) {
            validate(mCPatcherConfig, PatcherConfig.class, "destroyAutoPatcher", patcher);
            ensureNotNull(patcher);

            CWrapper.mbp_config_destroy_autopatcher(mCPatcherConfig, patcher.getPointer());
        }

        public void destroyRamdiskPatcher(RamdiskPatcher patcher) {
            validate(mCPatcherConfig, PatcherConfig.class, "destroyRamdiskPatcher", patcher);
            ensureNotNull(patcher);

            CWrapper.mbp_config_destroy_ramdisk_patcher(mCPatcherConfig, patcher.getPointer());
        }

        public boolean loadPatchInfos() {
            validate(mCPatcherConfig, PatcherConfig.class, "loadPatchInfos");
            return CWrapper.mbp_config_load_patchinfos(mCPatcherConfig);
        }
    }

    public static class PatcherError implements Parcelable {
        public interface ErrorType {
            int GENERIC_ERROR = 0;
            int PATCHER_CREATION_ERROR = 1;
            int IO_ERROR = 2;
            int BOOT_IMAGE_ERROR = 3;
            int CPIO_ERROR = 4;
            int ARCHIVE_ERROR = 5;
            int XML_ERROR = 6;
            int SUPPORTED_FILE_ERROR = 7;
            int CANCELLED_ERROR = 8;
            int PATCHING_ERROR = 9;
        }

        public interface ErrorCode {
            int NO_ERROR = 0;
            int UNKNOWN_ERROR = 1;
            int PATCHER_CREATE_ERROR = 2;
            int AUTOPATCHER_CREATE_ERROR = 3;
            int RAMDISK_PATCHER_CREATE_ERROR = 4;
            int FILE_OPEN_ERROR = 5;
            int FILE_READ_ERROR = 6;
            int FILE_WRITE_ERROR = 7;
            int DIRECTORY_NOT_EXIST_ERROR = 8;
            int BOOT_IMAGE_PARSE_ERROR = 9;
            int BOOT_IMAGE_APPLY_BUMP_ERROR = 10;
            int BOOT_IMAGE_APPLY_LOKI_ERROR = 11;
            int CPIO_FILE_ALREADY_EXIST_ERROR = 12;
            int CPIO_FILE_NOT_EXIST_ERROR = 13;
            int ARCHIVE_READ_OPEN_ERROR = 14;
            int ARCHIVE_READ_DATA_ERROR = 15;
            int ARCHIVE_READ_HEADER_ERROR = 16;
            int ARCHIVE_WRITE_OPEN_ERROR = 17;
            int ARCHIVE_WRITE_DATA_ERROR = 18;
            int ARCHIVE_WRITE_HEADER_ERROR = 19;
            int ARCHIVE_CLOSE_ERROR = 20;
            int ARCHIVE_FREE_ERROR = 21;
            int XML_PARSE_FILE_ERROR = 22;
            int ONLY_ZIP_SUPPORTED = 23;
            int ONLY_BOOT_IMAGE_SUPPORTED = 24;
            int PATCHING_CANCELLED = 25;
            int SYSTEM_CACHE_FORMAT_LINES_NOT_FOUND = 26;
            int APPLY_PATCH_FILE_ERROR = 27;
        }

        private static final HashMap<CPatcherError, Integer> sInstances = new HashMap<>();
        private CPatcherError mCPatcherError;

        PatcherError(CPatcherError cPatcherError) {
            ensureNotNull(cPatcherError);

            mCPatcherError = cPatcherError;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatcherError);
            }
            validate(mCPatcherError, PatcherError.class, "(Constructor)");
        }

        public void destroy() {
            validate(mCPatcherError, PatcherError.class, "destroy");
            synchronized (sInstances) {
                if (mCPatcherError != null && decrementRefCount(sInstances, mCPatcherError)) {
                    validate(mCPatcherError, PatcherError.class, "(Destroyed)");
                    CWrapper.mbp_error_destroy(mCPatcherError);
                }
                mCPatcherError = null;
            }
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof PatcherError
                    && mCPatcherError.equals(((PatcherError) o).mCPatcherError);
        }

        @Override
        public int hashCode() {
            return mCPatcherError.hashCode();
        }

        @Override
        protected void finalize() throws Throwable {
            destroy();
            super.finalize();
        }

        CPatcherError getPointer() {
            validate(mCPatcherError, PatcherError.class, "getPointer");
            return mCPatcherError;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCPatcherError.getPointer());
            out.writeLong(peer);
        }

        private PatcherError(Parcel in) {
            long peer = in.readLong();
            mCPatcherError = new CPatcherError();
            mCPatcherError.setPointer(new Pointer(peer));
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatcherError);
            }
            validate(mCPatcherError, PatcherError.class, "(Constructor)");
        }

        public static final Parcelable.Creator<PatcherError> CREATOR
                = new Parcelable.Creator<PatcherError>() {
            public PatcherError createFromParcel(Parcel in) {
                return new PatcherError(in);
            }

            public PatcherError[] newArray(int size) {
                return new PatcherError[size];
            }
        };

        public /* ErrorType */ int getErrorType() {
            validate(mCPatcherError, PatcherError.class, "getErrorType");
            return CWrapper.mbp_error_error_type(mCPatcherError);
        }

        public /* ErrorCode */ int getErrorCode() {
            validate(mCPatcherError, PatcherError.class, "getErrorCode");
            return CWrapper.mbp_error_error_code(mCPatcherError);
        }

        public String getPatcherId() {
            validate(mCPatcherError, PatcherError.class, "getPatcherId");
            Pointer p = CWrapper.mbp_error_patcher_id(mCPatcherError);
            return getStringAndFree(p);
        }

        public String getFilename() {
            validate(mCPatcherError, PatcherError.class, "getFilename");
            Pointer p = CWrapper.mbp_error_filename(mCPatcherError);
            return getStringAndFree(p);
        }
    }

    public static class PatchInfo implements Parcelable {
        private static final HashMap<CPatchInfo, Integer> sInstances = new HashMap<>();
        private CPatchInfo mCPatchInfo;
        private boolean mDestroyable;

        public PatchInfo() {
            mCPatchInfo = CWrapper.mbp_patchinfo_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatchInfo);
            }
            mDestroyable = true;
            validate(mCPatchInfo, PatchInfo.class, "(Constructor)");
        }

        PatchInfo(CPatchInfo cPatchInfo, boolean destroyable) {
            ensureNotNull(cPatchInfo);

            mCPatchInfo = cPatchInfo;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatchInfo);
            }
            mDestroyable = destroyable;
            validate(mCPatchInfo, PatchInfo.class, "(Constructor)");
        }

        public void destroy() {
            validate(mCPatchInfo, PatchInfo.class, "destroy");
            synchronized (sInstances) {
                if (mCPatchInfo != null && decrementRefCount(sInstances, mCPatchInfo)
                        && mDestroyable) {
                    validate(mCPatchInfo, PatchInfo.class, "(Destroyed)");
                    CWrapper.mbp_patchinfo_destroy(mCPatchInfo);
                }
                mCPatchInfo = null;
            }
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof PatchInfo && mCPatchInfo.equals(((PatchInfo) o).mCPatchInfo);
        }

        @Override
        public int hashCode() {
            int hashCode = 1;
            hashCode = 31 * hashCode + mCPatchInfo.hashCode();
            hashCode = 31 * hashCode + (mDestroyable ? 1 : 0);
            return hashCode;
        }

        @Override
        protected void finalize() throws Throwable {
            destroy();
            super.finalize();
        }

        CPatchInfo getPointer() {
            validate(mCPatchInfo, PatchInfo.class, "getPointer");
            return mCPatchInfo;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCPatchInfo.getPointer());
            out.writeLong(peer);
            out.writeInt(mDestroyable ? 1 : 0);
        }

        private PatchInfo(Parcel in) {
            long peer = in.readLong();
            mCPatchInfo = new CPatchInfo();
            mCPatchInfo.setPointer(new Pointer(peer));
            mDestroyable = in.readInt() != 0;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatchInfo);
            }
            validate(mCPatchInfo, PatchInfo.class, "(Constructor)");
        }

        public static final Parcelable.Creator<PatchInfo> CREATOR
                = new Parcelable.Creator<PatchInfo>() {
            public PatchInfo createFromParcel(Parcel in) {
                return new PatchInfo(in);
            }

            public PatchInfo[] newArray(int size) {
                return new PatchInfo[size];
            }
        };

        public String getId() {
            validate(mCPatchInfo, PatchInfo.class, "getId");
            Pointer p = CWrapper.mbp_patchinfo_id(mCPatchInfo);
            return getStringAndFree(p);
        }

        public void setId(String id) {
            validate(mCPatchInfo, PatchInfo.class, "setId", id);
            ensureNotNull(id);

            CWrapper.mbp_patchinfo_set_id(mCPatchInfo, id);
        }

        public String getName() {
            validate(mCPatchInfo, PatchInfo.class, "getName");
            Pointer p = CWrapper.mbp_patchinfo_name(mCPatchInfo);
            return getStringAndFree(p);
        }

        public void setName(String name) {
            validate(mCPatchInfo, PatchInfo.class, "setName", name);
            ensureNotNull(name);

            CWrapper.mbp_patchinfo_set_name(mCPatchInfo, name);
        }

        public String[] getRegexes() {
            validate(mCPatchInfo, PatchInfo.class, "getRegexes");
            Pointer p = CWrapper.mbp_patchinfo_regexes(mCPatchInfo);
            return getStringArrayAndFree(p);
        }

        public void setRegexes(String[] regexes) {
            validate(mCPatchInfo, PatchInfo.class, "setRegexes", (Object) regexes);
            ensureNotNull(regexes);

            CWrapper.mbp_patchinfo_set_regexes(mCPatchInfo, new StringArray(regexes));
        }

        public String[] getExcludeRegexes() {
            validate(mCPatchInfo, PatchInfo.class, "getExcludeRegexes");
            Pointer p = CWrapper.mbp_patchinfo_exclude_regexes(mCPatchInfo);
            return getStringArrayAndFree(p);
        }

        public void setExcludeRegexes(String[] regexes) {
            validate(mCPatchInfo, PatchInfo.class, "setExcludeRegexes", (Object) regexes);
            ensureNotNull(regexes);

            CWrapper.mbp_patchinfo_set_exclude_regexes(mCPatchInfo, new StringArray(regexes));
        }

        public void addAutoPatcher(String apName, StringMap args) {
            validate(mCPatchInfo, PatchInfo.class, "addAutoPatcher", apName, args);
            ensureNotNull(apName);

            if (args == null) {
                args = new StringMap();
            }

            CStringMap cArgs = args.getPointer();
            CWrapper.mbp_patchinfo_add_autopatcher(mCPatchInfo, apName, cArgs);
        }

        public void removeAutoPatcher(String apName) {
            validate(mCPatchInfo, PatchInfo.class, "removeAutoPatcher", apName);
            ensureNotNull(apName);

            CWrapper.mbp_patchinfo_remove_autopatcher(mCPatchInfo, apName);
        }

        public String[] getAutoPatchers() {
            validate(mCPatchInfo, PatchInfo.class, "getAutoPatchers");

            Pointer p = CWrapper.mbp_patchinfo_autopatchers(mCPatchInfo);
            return getStringArrayAndFree(p);
        }

        public StringMap getAutoPatcherArgs(String apName) {
            validate(mCPatchInfo, PatchInfo.class, "getAutoPatcherArgs", apName);
            ensureNotNull(apName);

            CStringMap args = CWrapper.mbp_patchinfo_autopatcher_args(mCPatchInfo, apName);
            return new StringMap(args);
        }

        public boolean hasBootImage() {
            validate(mCPatchInfo, PatchInfo.class, "hasBootImage");

            return CWrapper.mbp_patchinfo_has_boot_image(mCPatchInfo);
        }

        public void setHasBootImage(boolean hasBootImage) {
            validate(mCPatchInfo, PatchInfo.class, "setHasBootImage", hasBootImage);

            CWrapper.mbp_patchinfo_set_has_boot_image(mCPatchInfo, hasBootImage);
        }

        public boolean autoDetectBootImages() {
            validate(mCPatchInfo, PatchInfo.class, "autodetectBootImages");

            return CWrapper.mbp_patchinfo_autodetect_boot_images(mCPatchInfo);
        }

        public void setAutoDetectBootImages(boolean autoDetect) {
            validate(mCPatchInfo, PatchInfo.class, "setAutoDetectBootImages", autoDetect);

            CWrapper.mbp_patchinfo_set_autodetect_boot_images(mCPatchInfo, autoDetect);
        }

        public String[] getBootImages() {
            validate(mCPatchInfo, PatchInfo.class, "getBootImages");

            Pointer p = CWrapper.mbp_patchinfo_boot_images(mCPatchInfo);
            return getStringArrayAndFree(p);
        }

        public void setBootImages(String[] bootImages) {
            validate(mCPatchInfo, PatchInfo.class, "setBootImages", bootImages);
            ensureNotNull(bootImages);

            CWrapper.mbp_patchinfo_set_boot_images(mCPatchInfo, new StringArray(bootImages));
        }

        public String getRamdisk() {
            validate(mCPatchInfo, PatchInfo.class, "getRamdisk");

            Pointer p = CWrapper.mbp_patchinfo_ramdisk(mCPatchInfo);
            return getStringAndFree(p);
        }

        public void setRamdisk(String ramdisk) {
            validate(mCPatchInfo, PatchInfo.class, "setRamdisk", ramdisk);
            ensureNotNull(ramdisk);

            CWrapper.mbp_patchinfo_set_ramdisk(mCPatchInfo, ramdisk);
        }

        public boolean deviceCheck() {
            validate(mCPatchInfo, PatchInfo.class, "deviceCheck");

            return CWrapper.mbp_patchinfo_device_check(mCPatchInfo);
        }

        public void setDeviceCheck(boolean deviceCheck) {
            validate(mCPatchInfo, PatchInfo.class, "setDeviceCheck", deviceCheck);

            CWrapper.mbp_patchinfo_set_device_check(mCPatchInfo, deviceCheck);
        }
    }

    public static class StringMap implements Parcelable {
        private static final HashMap<CStringMap, Integer> sInstances = new HashMap<>();
        private CStringMap mCStringMap;

        public StringMap() {
            mCStringMap = CWrapper.mbp_stringmap_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCStringMap);
            }
            validate(mCStringMap, StringMap.class, "(Constructor)");
        }

        StringMap(CStringMap cStringMap) {
            ensureNotNull(cStringMap);
            mCStringMap = cStringMap;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCStringMap);
            }
            validate(mCStringMap, StringMap.class, "(Constructor)");
        }

        public void destroy() {
            validate(mCStringMap, StringMap.class, "destroy");
            synchronized (sInstances) {
                if (mCStringMap != null && decrementRefCount(sInstances, mCStringMap)) {
                    validate(mCStringMap, StringMap.class, "(Destroyed)");
                    CWrapper.mbp_stringmap_destroy(mCStringMap);
                }
                mCStringMap = null;
            }
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof StringMap && mCStringMap.equals(((StringMap) o).mCStringMap);
        }

        @Override
        public int hashCode() {
            return mCStringMap.hashCode();
        }

        @Override
        protected void finalize() throws Throwable {
            destroy();
            super.finalize();
        }

        CStringMap getPointer() {
            validate(mCStringMap, StringMap.class, "getPointer");
            return mCStringMap;
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCStringMap.getPointer());
            out.writeLong(peer);
        }

        private StringMap(Parcel in) {
            long peer = in.readLong();
            mCStringMap = new CStringMap();
            mCStringMap.setPointer(new Pointer(peer));
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCStringMap);
            }
            validate(mCStringMap, StringMap.class, "(Constructor)");
        }

        public static final Parcelable.Creator<StringMap> CREATOR
                = new Parcelable.Creator<StringMap>() {
            public StringMap createFromParcel(Parcel in) {
                return new StringMap(in);
            }

            public StringMap[] newArray(int size) {
                return new StringMap[size];
            }
        };

        public HashMap<String, String> getHashMap() {
            validate(mCStringMap, StringMap.class, "getHashMap");
            HashMap<String, String> map = new HashMap<>();
            Pointer pKeys = CWrapper.mbp_stringmap_keys(mCStringMap);
            String[] keys = getStringArrayAndFree(pKeys);
            for (String key : keys) {
                Pointer pValue = CWrapper.mbp_stringmap_get(mCStringMap, key);
                String value = getStringAndFree(pValue);
                map.put(key, value);
            }
            return map;
        }

        public void setHashMap(HashMap<String, String> map) {
            validate(mCStringMap, StringMap.class, "setHashMap", map);
            ensureNotNull(map);

            CWrapper.mbp_stringmap_clear(mCStringMap);
            for (Map.Entry<String, String> entry : map.entrySet()) {
                CWrapper.mbp_stringmap_set(mCStringMap, entry.getKey(), entry.getValue());
            }
        }
    }

    public static class Patcher implements Parcelable {
        private CPatcher mCPatcher;

        Patcher(CPatcher cPatcher) {
            ensureNotNull(cPatcher);
            mCPatcher = cPatcher;
            validate(mCPatcher, Patcher.class, "(Constructor)");
        }

        CPatcher getPointer() {
            validate(mCPatcher, Patcher.class, "getPointer");
            return mCPatcher;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof Patcher && mCPatcher.equals(((Patcher) o).mCPatcher);
        }

        @Override
        public int hashCode() {
            return mCPatcher.hashCode();
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCPatcher.getPointer());
            out.writeLong(peer);
        }

        private Patcher(Parcel in) {
            long peer = in.readLong();
            mCPatcher = new CPatcher();
            mCPatcher.setPointer(new Pointer(peer));
            validate(mCPatcher, Patcher.class, "(Constructor)");
        }

        public static final Parcelable.Creator<Patcher> CREATOR
                = new Parcelable.Creator<Patcher>() {
            public Patcher createFromParcel(Parcel in) {
                return new Patcher(in);
            }

            public Patcher[] newArray(int size) {
                return new Patcher[size];
            }
        };

        public PatcherError getError() {
            validate(mCPatcher, Patcher.class, "getError");
            CPatcherError error = CWrapper.mbp_patcher_error(mCPatcher);
            return new PatcherError(error);
        }

        public String getId() {
            validate(mCPatcher, Patcher.class, "getId");
            Pointer p = CWrapper.mbp_patcher_id(mCPatcher);
            return getStringAndFree(p);
        }

        public boolean usesPatchInfo() {
            validate(mCPatcher, Patcher.class, "usesPatchInfo");
            return CWrapper.mbp_patcher_uses_patchinfo(mCPatcher);
        }

        public void setFileInfo(FileInfo info) {
            validate(mCPatcher, Patcher.class, "setFileInfo", info);
            CWrapper.mbp_patcher_set_fileinfo(mCPatcher, info.getPointer());
        }

        public String newFilePath() {
            validate(mCPatcher, Patcher.class, "newFilePath");
            Pointer p = CWrapper.mbp_patcher_new_file_path(mCPatcher);
            return getStringAndFree(p);
        }

        public boolean patchFile(final ProgressListener listener) {
            validate(mCPatcher, Patcher.class, "patchFile", listener);
            CWrapper.MaxProgressUpdatedCallback maxProgressCb = null;
            CWrapper.ProgressUpdatedCallback progressCb = null;
            CWrapper.DetailsUpdatedCallback detailsCb = null;

            if (listener != null) {
                maxProgressCb = new CWrapper.MaxProgressUpdatedCallback() {
                    @Override
                    public void invoke(int value, Pointer userData) {
                        listener.onMaxProgressUpdated(value);
                    }
                };

                progressCb = new CWrapper.ProgressUpdatedCallback() {
                    @Override
                    public void invoke(int value, Pointer userData) {
                        listener.onProgressUpdated(value);
                    }
                };

                detailsCb = new CWrapper.DetailsUpdatedCallback() {
                    @Override
                    public void invoke(String text, Pointer userData) {
                        listener.onDetailsUpdated(text);
                    }
                };
            }

            return CWrapper.mbp_patcher_patch_file(mCPatcher,
                    maxProgressCb, progressCb, detailsCb, null);
        }

        public void cancelPatching() {
            validate(mCPatcher, Patcher.class, "cancelPatching");
            CWrapper.mbp_patcher_cancel_patching(mCPatcher);
        }

        public interface ProgressListener {
            void onMaxProgressUpdated(int max);

            void onProgressUpdated(int value);

            void onDetailsUpdated(String text);
        }
    }

    public static class AutoPatcher implements Parcelable {
        private CAutoPatcher mCAutoPatcher;

        AutoPatcher(CAutoPatcher cAutoPatcher) {
            ensureNotNull(cAutoPatcher);
            mCAutoPatcher = cAutoPatcher;
            validate(mCAutoPatcher, AutoPatcher.class, "(Constructor)");
        }

        CAutoPatcher getPointer() {
            validate(mCAutoPatcher, AutoPatcher.class, "getPointer");
            return mCAutoPatcher;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof AutoPatcher
                    && mCAutoPatcher.equals(((AutoPatcher) o).mCAutoPatcher);
        }

        @Override
        public int hashCode() {
            return mCAutoPatcher.hashCode();
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCAutoPatcher.getPointer());
            out.writeLong(peer);
        }

        private AutoPatcher(Parcel in) {
            long peer = in.readLong();
            mCAutoPatcher = new CAutoPatcher();
            mCAutoPatcher.setPointer(new Pointer(peer));
            validate(mCAutoPatcher, AutoPatcher.class, "(Constructor)");
        }

        public static final Parcelable.Creator<AutoPatcher> CREATOR
                = new Parcelable.Creator<AutoPatcher>() {
            public AutoPatcher createFromParcel(Parcel in) {
                return new AutoPatcher(in);
            }

            public AutoPatcher[] newArray(int size) {
                return new AutoPatcher[size];
            }
        };

        public PatcherError getError() {
            validate(mCAutoPatcher, AutoPatcher.class, "getError");
            CPatcherError error = CWrapper.mbp_autopatcher_error(mCAutoPatcher);
            return new PatcherError(error);
        }

        public String getId() {
            validate(mCAutoPatcher, AutoPatcher.class, "getId");
            Pointer p = CWrapper.mbp_autopatcher_id(mCAutoPatcher);
            return getStringAndFree(p);
        }

        public String[] newFiles() {
            validate(mCAutoPatcher, AutoPatcher.class, "newFiles");
            Pointer p = CWrapper.mbp_autopatcher_new_files(mCAutoPatcher);
            return getStringArrayAndFree(p);
        }

        public String[] existingFiles() {
            validate(mCAutoPatcher, AutoPatcher.class, "existingFiles");
            Pointer p = CWrapper.mbp_autopatcher_existing_files(mCAutoPatcher);
            return getStringArrayAndFree(p);
        }

        public boolean patchFiles(String directory) {
            validate(mCAutoPatcher, AutoPatcher.class, "patchFiles", directory);
            ensureNotNull(directory);

            return CWrapper.mbp_autopatcher_patch_files(mCAutoPatcher, directory);
        }
    }

    public static class RamdiskPatcher implements Parcelable {
        private CRamdiskPatcher mCRamdiskPatcher;

        RamdiskPatcher(CRamdiskPatcher cRamdiskPatcher) {
            ensureNotNull(cRamdiskPatcher);
            mCRamdiskPatcher = cRamdiskPatcher;
            validate(mCRamdiskPatcher, RamdiskPatcher.class, "(Constructor)");
        }

        CRamdiskPatcher getPointer() {
            validate(mCRamdiskPatcher, RamdiskPatcher.class, "getPointer");
            return mCRamdiskPatcher;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof RamdiskPatcher
                    && mCRamdiskPatcher.equals(((RamdiskPatcher) o).mCRamdiskPatcher);
        }

        @Override
        public int hashCode() {
            return mCRamdiskPatcher.hashCode();
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel out, int flags) {
            long peer = Pointer.nativeValue(mCRamdiskPatcher.getPointer());
            out.writeLong(peer);
        }

        private RamdiskPatcher(Parcel in) {
            long peer = in.readLong();
            mCRamdiskPatcher = new CRamdiskPatcher();
            mCRamdiskPatcher.setPointer(new Pointer(peer));
            validate(mCRamdiskPatcher, RamdiskPatcher.class, "(Constructor)");
        }

        public static final Parcelable.Creator<RamdiskPatcher> CREATOR
                = new Parcelable.Creator<RamdiskPatcher>() {
            public RamdiskPatcher createFromParcel(Parcel in) {
                return new RamdiskPatcher(in);
            }

            public RamdiskPatcher[] newArray(int size) {
                return new RamdiskPatcher[size];
            }
        };

        public PatcherError getError() {
            validate(mCRamdiskPatcher, RamdiskPatcher.class, "getError");
            CPatcherError error = CWrapper.mbp_ramdiskpatcher_error(mCRamdiskPatcher);
            return new PatcherError(error);
        }

        public String getId() {
            validate(mCRamdiskPatcher, RamdiskPatcher.class, "getId");
            Pointer p = CWrapper.mbp_ramdiskpatcher_id(mCRamdiskPatcher);
            return getStringAndFree(p);
        }

        public boolean patchRamdisk() {
            validate(mCRamdiskPatcher, RamdiskPatcher.class, "patchRamdisk");
            return CWrapper.mbp_ramdiskpatcher_patch_ramdisk(mCRamdiskPatcher);
        }
    }
}
