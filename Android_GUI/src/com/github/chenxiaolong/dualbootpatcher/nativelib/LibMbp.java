/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.nativelib;

import android.os.Parcel;
import android.os.Parcelable;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CWrapper.CAutoPatcher;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CWrapper.CBootImage;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CWrapper.CCpioFile;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CWrapper.CDevice;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CWrapper.CFileInfo;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CWrapper.CPatcher;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CWrapper.CPatcherConfig;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.CWrapper.CRamdiskPatcher;
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

// NOTE: Almost no checking of parameters is performed on both the Java and C side of this native
//       wrapper. As a rule of thumb, don't pass null to any function.

@SuppressWarnings("unused")
public class LibMbp {
    /** Log (almost) all native method calls and their parameters */
    private static final boolean DEBUG = false;

    static class CWrapper {
        static {
            Native.register(CWrapper.class, "mbp");
            LibMiscStuff.INSTANCE.mblog_set_logcat();
        }

        // BEGIN: ctypes.h
        public static class CBootImage extends PointerType {}
        public static class CCpioFile extends PointerType {}
        public static class CDevice extends PointerType {}
        public static class CFileInfo extends PointerType {}
        public static class CPatcherConfig extends PointerType {}

        public static class CPatcher extends PointerType {}
        public static class CAutoPatcher extends PointerType {}
        public static class CRamdiskPatcher extends PointerType {}
        // END: ctypes.h

        // BEGIN: cbootimage.h
        static native CBootImage mbp_bootimage_create();
        static native void mbp_bootimage_destroy(CBootImage bi);
        static native /* ErrorCode */ int mbp_bootimage_error(CBootImage bi);
        static native boolean mbp_bootimage_load_data(CBootImage bi, Pointer data, int size);
        static native boolean mbp_bootimage_load_file(CBootImage bi, String filename);
        static native boolean mbp_bootimage_create_data(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference size);
        static native boolean mbp_bootimage_create_file(CBootImage bi, String filename);
        static native int /* BootImageType */ mbp_bootimage_was_type(CBootImage bi);
        static native int /* BootImageType */ mbp_bootimage_target_type(CBootImage bi);
        static native void mbp_bootimage_set_target_type(CBootImage bi, /* BootImageType */ int type);
        static native Pointer mbp_bootimage_boardname(CBootImage bi);
        static native void mbp_bootimage_set_boardname(CBootImage bi, String name);
        static native Pointer mbp_bootimage_kernel_cmdline(CBootImage bi);
        static native void mbp_bootimage_set_kernel_cmdline(CBootImage bi, String cmdline);
        static native int mbp_bootimage_page_size(CBootImage bi);
        static native void mbp_bootimage_set_page_size(CBootImage bi, int pageSize);
        static native int mbp_bootimage_kernel_address(CBootImage bi);
        static native void mbp_bootimage_set_kernel_address(CBootImage bi, int address);
        static native int mbp_bootimage_ramdisk_address(CBootImage bi);
        static native void mbp_bootimage_set_ramdisk_address(CBootImage bi, int address);
        static native int mbp_bootimage_second_bootloader_address(CBootImage bi);
        static native void mbp_bootimage_set_second_bootloader_address(CBootImage bi, int address);
        static native int mbp_bootimage_kernel_tags_address(CBootImage bi);
        static native void mbp_bootimage_set_kernel_tags_address(CBootImage bi, int address);
        static native int mbp_bootimage_ipl_address(CBootImage bi);
        static native void mbp_bootimage_set_ipl_address(CBootImage bi, int address);
        static native int mbp_bootimage_rpm_address(CBootImage bi);
        static native void mbp_bootimage_set_rpm_address(CBootImage bi, int address);
        static native int mbp_bootimage_appsbl_address(CBootImage bi);
        static native void mbp_bootimage_set_appsbl_address(CBootImage bi, int address);
        static native int mbp_bootimage_entrypoint_address(CBootImage bi);
        static native void mbp_bootimage_set_entrypoint_address(CBootImage bi, int address);
        static native void mbp_bootimage_kernel_image(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_kernel_image(CBootImage bi, Pointer data, /* size_t */ int size);
        static native void mbp_bootimage_ramdisk_image(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_ramdisk_image(CBootImage bi, Pointer data, /* size_t */ int size);
        static native void mbp_bootimage_second_bootloader_image(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_second_bootloader_image(CBootImage bi, Pointer data, /* size_t */ int size);
        static native void mbp_bootimage_device_tree_image(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_device_tree_image(CBootImage bi, Pointer data, int size);
        static native void mbp_bootimage_aboot_image(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_aboot_image(CBootImage bi, Pointer data, int size);
        static native void mbp_bootimage_kernel_mtk_header(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_kernel_mtk_header(CBootImage bi, Pointer data, int size);
        static native void mbp_bootimage_ramdisk_mtk_header(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_ramdisk_mtk_header(CBootImage bi, Pointer data, int size);
        static native void mbp_bootimage_ipl_image(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_ipl_image(CBootImage bi, Pointer data, int size);
        static native void mbp_bootimage_rpm_image(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_rpm_image(CBootImage bi, Pointer data, int size);
        static native void mbp_bootimage_appsbl_image(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_appsbl_image(CBootImage bi, Pointer data, int size);
        static native void mbp_bootimage_sin_image(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_sin_image(CBootImage bi, Pointer data, int size);
        static native void mbp_bootimage_sin_header(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference sizeReturn);
        static native void mbp_bootimage_set_sin_header(CBootImage bi, Pointer data, int size);
        static native boolean mbp_bootimage_equals(CBootImage lhs, CBootImage rhs);
        // END: cbootimage.h

        // BEGIN: ccommon.h
        static native void mbp_free(Pointer data);
        static native void mbp_free_array(Pointer array);
        // END: ccommon.h

        // BEGIN: ccpiofile.h
        static native CCpioFile mbp_cpiofile_create();
        static native void mbp_cpiofile_destroy(CCpioFile cpio);
        static native /* ErrorCode */ int mbp_cpiofile_error(CCpioFile cpio);
        static native boolean mbp_cpiofile_load_data(CCpioFile cpio, Pointer data, int size);
        static native boolean mbp_cpiofile_create_data(CCpioFile cpio, PointerByReference dataReturn, /* size_t */ IntByReference size);
        static native boolean mbp_cpiofile_exists(CCpioFile cpio, String filename);
        static native boolean mbp_cpiofile_remove(CCpioFile cpio, String filename);
        static native Pointer mbp_cpiofile_filenames(CCpioFile cpio);
        static native boolean mbp_cpiofile_is_symlink(CCpioFile cpio, String filename);
        static native Pointer mbp_cpiofile_symlink_path(CCpioFile cpio, String filename);
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
        static native long mbp_device_flags(CDevice device);
        static native void mbp_device_set_flags(CDevice device, long flags);
        static native Pointer mbp_device_ramdisk_patcher(CDevice device);
        static native void mbp_device_set_ramdisk_patcher(CDevice device, String id);
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
        static native Pointer mbp_fileinfo_input_path(CFileInfo info);
        static native void mbp_fileinfo_set_input_path(CFileInfo info, String path);
        static native Pointer mbp_fileinfo_output_path(CFileInfo info);
        static native void mbp_fileinfo_set_output_path(CFileInfo info, String path);
        static native CDevice mbp_fileinfo_device(CFileInfo info);
        static native void mbp_fileinfo_set_device(CFileInfo info, CDevice device);
        static native Pointer mbp_fileinfo_rom_id(CFileInfo info);
        static native void mbp_fileinfo_set_rom_id(CFileInfo info, String id);
        // END: cfileinfo.h

        // BEGIN: cpatcherconfig.h
        static native CPatcherConfig mbp_config_create();
        static native void mbp_config_destroy(CPatcherConfig pc);
        static native /* ErrorCode */ int mbp_config_error(CPatcherConfig pc);
        static native Pointer mbp_config_data_directory(CPatcherConfig pc);
        static native Pointer mbp_config_temp_directory(CPatcherConfig pc);
        static native void mbp_config_set_data_directory(CPatcherConfig pc, String path);
        static native void mbp_config_set_temp_directory(CPatcherConfig pc, String path);
        static native Pointer mbp_config_version(CPatcherConfig pc);
        static native Pointer mbp_config_devices(CPatcherConfig pc);
        static native Pointer mbp_config_patchers(CPatcherConfig pc);
        static native Pointer mbp_config_autopatchers(CPatcherConfig pc);
        static native Pointer mbp_config_ramdiskpatchers(CPatcherConfig pc);
        static native CPatcher mbp_config_create_patcher(CPatcherConfig pc, String id);
        static native CAutoPatcher mbp_config_create_autopatcher(CPatcherConfig pc, String id, CFileInfo info);
        static native CRamdiskPatcher mbp_config_create_ramdisk_patcher(CPatcherConfig pc, String id, CFileInfo info, CCpioFile cpio);
        static native void mbp_config_destroy_patcher(CPatcherConfig pc, CPatcher patcher);
        static native void mbp_config_destroy_autopatcher(CPatcherConfig pc, CAutoPatcher patcher);
        static native void mbp_config_destroy_ramdisk_patcher(CPatcherConfig pc, CRamdiskPatcher patcher);
        // END: cpatcherconfig.h

        // BEGIN: cpatcherinterface.h
        interface ProgressUpdatedCallback extends Callback {
            void invoke(long bytes, long maxBytes, Pointer userData);
        }

        interface FilesUpdatedCallback extends Callback {
            void invoke(long files, long maxFiles, Pointer userData);
        }

        interface DetailsUpdatedCallback extends Callback {
            void invoke(String text, Pointer userData);
        }

        static native /* ErrorCode */ int mbp_patcher_error(CPatcher patcher);
        static native Pointer mbp_patcher_id(CPatcher patcher);
        static native void mbp_patcher_set_fileinfo(CPatcher patcher, CFileInfo info);
        static native boolean mbp_patcher_patch_file(CPatcher patcher, ProgressUpdatedCallback progressCb, FilesUpdatedCallback filesCb, DetailsUpdatedCallback detailsCb, Pointer userData);
        static native void mbp_patcher_cancel_patching(CPatcher patcher);

        static native /* ErrorCode */ int mbp_autopatcher_error(CAutoPatcher patcher);
        static native Pointer mbp_autopatcher_id(CAutoPatcher patcher);
        static native Pointer mbp_autopatcher_new_files(CAutoPatcher patcher);
        static native Pointer mbp_autopatcher_existing_files(CAutoPatcher patcher);
        static native boolean mbp_autopatcher_patch_files(CAutoPatcher patcher, String directory);

        static native /* ErrorCode */ int mbp_ramdiskpatcher_error(CRamdiskPatcher patcher);
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
        if (ptr == null) {
            sb.append("null");
        } else {
            sb.append(ptr.getPointer());
        }
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

        if (ptr == null && !method.equals("destroy")) {
            throw new IllegalStateException("Called on a destroyed object! " + signature);
        }
    }

    public static class BootImage implements Parcelable {
        public interface Type {
            int ANDROID = 1;
            int LOKI = 2;
            int BUMP = 3;
            int SONY_ELF = 4;
        }

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
            synchronized (sInstances) {
                validate(mCBootImage, BootImage.class, "destroy");
                if (mCBootImage != null && decrementRefCount(sInstances, mCBootImage)) {
                    validate(mCBootImage, BootImage.class, "(Destroyed)");
                    CWrapper.mbp_bootimage_destroy(mCBootImage);
                }
                mCBootImage = null;
            }
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) {
                return true;
            }
            if (!(o instanceof BootImage)) {
                return false;
            }

            BootImage bi = (BootImage) o;

            return mCBootImage.equals(bi.mCBootImage) ||
                    CWrapper.mbp_bootimage_equals(mCBootImage, bi.mCBootImage);
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

        public /* ErrorCode */ int getError() {
            validate(mCBootImage, BootImage.class, "getError");
            return CWrapper.mbp_bootimage_error(mCBootImage);
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

            boolean ret = CWrapper.mbp_bootimage_create_data(mCBootImage, pData, pSize);
            if (!ret) {
                return null;
            }

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

        public /* BootImageType */ int wasType() {
            validate(mCBootImage, BootImage.class, "wasType");

            return CWrapper.mbp_bootimage_was_type(mCBootImage);
        }

        public /* BootImageType */ int targetType() {
            validate(mCBootImage, BootImage.class, "targetType");

            return CWrapper.mbp_bootimage_target_type(mCBootImage);
        }

        public void setTargetType(/* BootImageType */ int type) {
            validate(mCBootImage, BootImage.class, "setTargetType", type);

            CWrapper.mbp_bootimage_set_target_type(mCBootImage, type);
        }

        public String getBoardName() {
            validate(mCBootImage, BootImage.class, "getBoardName");
            Pointer p = CWrapper.mbp_bootimage_boardname(mCBootImage);
            return p.getString(0);
        }

        public void setBoardName(String name) {
            validate(mCBootImage, BootImage.class, "setBoardName", name);
            ensureNotNull(name);

            CWrapper.mbp_bootimage_set_boardname(mCBootImage, name);
        }

        public String getKernelCmdline() {
            validate(mCBootImage, BootImage.class, "getKernelCmdline");
            Pointer p = CWrapper.mbp_bootimage_kernel_cmdline(mCBootImage);
            return p.getString(0);
        }

        public void setKernelCmdline(String cmdline) {
            validate(mCBootImage, BootImage.class, "setKernelCmdline", cmdline);
            ensureNotNull(cmdline);

            CWrapper.mbp_bootimage_set_kernel_cmdline(mCBootImage, cmdline);
        }

        public int getPageSize() {
            validate(mCBootImage, BootImage.class, "getPageSize");
            return CWrapper.mbp_bootimage_page_size(mCBootImage);
        }

        public void setPageSize(int size) {
            validate(mCBootImage, BootImage.class, "setPageSize", size);
            CWrapper.mbp_bootimage_set_page_size(mCBootImage, size);
        }

        public int getKernelAddress() {
            validate(mCBootImage, BootImage.class, "getKernelAddress");
            return CWrapper.mbp_bootimage_kernel_address(mCBootImage);
        }

        public void setKernelAddress(int address) {
            validate(mCBootImage, BootImage.class, "setKernelAddress", address);
            CWrapper.mbp_bootimage_set_kernel_address(mCBootImage, address);
        }

        public int getRamdiskAddress() {
            validate(mCBootImage, BootImage.class, "getRamdiskAddress");
            return CWrapper.mbp_bootimage_ramdisk_address(mCBootImage);
        }

        public void setRamdiskAddress(int address) {
            validate(mCBootImage, BootImage.class, "setRamdiskAddress", address);
            CWrapper.mbp_bootimage_set_ramdisk_address(mCBootImage, address);
        }

        public int getSecondBootloaderAddress() {
            validate(mCBootImage, BootImage.class, "getSecondBootloaderAddress");
            return CWrapper.mbp_bootimage_second_bootloader_address(mCBootImage);
        }

        public void setSecondBootloaderAddress(int address) {
            validate(mCBootImage, BootImage.class, "setSecondBootloaderAddress", address);
            CWrapper.mbp_bootimage_set_second_bootloader_address(mCBootImage, address);
        }

        public int getKernelTagsAddress() {
            validate(mCBootImage, BootImage.class, "getKernelTagsAddress");
            return CWrapper.mbp_bootimage_kernel_tags_address(mCBootImage);
        }

        public void setKernelTagsAddress(int address) {
            validate(mCBootImage, BootImage.class, "setKernelTagsAddress", address);
            CWrapper.mbp_bootimage_set_kernel_tags_address(mCBootImage, address);
        }

        public int getIplAddress() {
            validate(mCBootImage, BootImage.class, "getIplAddress");
            return CWrapper.mbp_bootimage_ipl_address(mCBootImage);
        }

        public void setIplAddress(int address) {
            validate(mCBootImage, BootImage.class, "setIplAddress", address);
            CWrapper.mbp_bootimage_set_ipl_address(mCBootImage, address);
        }

        public int getRpmAddress() {
            validate(mCBootImage, BootImage.class, "getRpmAddress");
            return CWrapper.mbp_bootimage_rpm_address(mCBootImage);
        }

        public void setRpmAddress(int address) {
            validate(mCBootImage, BootImage.class, "setRpmAddress", address);
            CWrapper.mbp_bootimage_set_rpm_address(mCBootImage, address);
        }

        public int getAppsblAddress() {
            validate(mCBootImage, BootImage.class, "getAppsblAddress");
            return CWrapper.mbp_bootimage_appsbl_address(mCBootImage);
        }

        public void setAppsblAddress(int address) {
            validate(mCBootImage, BootImage.class, "setAppsblAddress", address);
            CWrapper.mbp_bootimage_set_appsbl_address(mCBootImage, address);
        }

        public int getEntrypointAddress() {
            validate(mCBootImage, BootImage.class, "getEntrypointAddress");
            return CWrapper.mbp_bootimage_entrypoint_address(mCBootImage);
        }

        public void setEntrypointAddress(int address) {
            validate(mCBootImage, BootImage.class, "setEntrypointAddress", address);
            CWrapper.mbp_bootimage_set_entrypoint_address(mCBootImage, address);
        }

        public byte[] getKernelImage() {
            validate(mCBootImage, BootImage.class, "getKernelImage");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_kernel_image(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
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
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_ramdisk_image(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
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
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_second_bootloader_image(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
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
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_device_tree_image(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
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
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_aboot_image(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
        }

        public void setAbootImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setAbootImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_aboot_image(mCBootImage, mem, data.length);
        }

        public byte[] getKernelMtkHeader() {
            validate(mCBootImage, BootImage.class, "getKernelMtkHeader");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_kernel_mtk_header(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
        }

        public void setKernelMtkHeader(byte[] data) {
            validate(mCBootImage, BootImage.class, "setKernelMtkHeader", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_kernel_mtk_header(mCBootImage, mem, data.length);
        }

        public byte[] getRamdiskMtkHeader() {
            validate(mCBootImage, BootImage.class, "getRamdiskMtkHeader");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_ramdisk_mtk_header(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
        }

        public void setRamdiskMtkHeader(byte[] data) {
            validate(mCBootImage, BootImage.class, "setRamdiskMtkHeader", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_ramdisk_mtk_header(mCBootImage, mem, data.length);
        }

        public byte[] getIplImage() {
            validate(mCBootImage, BootImage.class, "getIplImage");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_ipl_image(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
        }

        public void setIplImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setIplImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_ipl_image(mCBootImage, mem, data.length);
        }

        public byte[] getRpmImage() {
            validate(mCBootImage, BootImage.class, "getRpmImage");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_rpm_image(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
        }

        public void setRpmImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setRpmImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_rpm_image(mCBootImage, mem, data.length);
        }

        public byte[] getAppsblImage() {
            validate(mCBootImage, BootImage.class, "getAppsblImage");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_appsbl_image(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
        }

        public void setAppsblImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setAppsblImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_appsbl_image(mCBootImage, mem, data.length);
        }

        public byte[] getSinImage() {
            validate(mCBootImage, BootImage.class, "getSinImage");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();
            CWrapper.mbp_bootimage_sin_image(mCBootImage, pData, pSize);
            Pointer data = pData.getValue();
            int size = pSize.getValue();
            return data.getByteArray(0, size);
        }

        public void setSinImage(byte[] data) {
            validate(mCBootImage, BootImage.class, "setSinImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_sin_image(mCBootImage, mem, data.length);
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
            synchronized (sInstances) {
                validate(mCCpioFile, CpioFile.class, "destroy");
                if (mCCpioFile != null && decrementRefCount(sInstances, mCCpioFile)) {
                    validate(mCCpioFile, CpioFile.class, "(Destroyed)");
                    CWrapper.mbp_cpiofile_destroy(mCCpioFile);
                }
                mCCpioFile = null;
            }
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

        public /* ErrorCode */ int getError() {
            validate(mCCpioFile, CpioFile.class, "getError");
            return CWrapper.mbp_cpiofile_error(mCCpioFile);
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

        public boolean isSymlink(String name) {
            validate(mCCpioFile, CpioFile.class, "isSymlink", name);
            ensureNotNull(name);

            return CWrapper.mbp_cpiofile_is_symlink(mCCpioFile, name);
        }

        public String getSymlinkPath(String name) {
            validate(mCCpioFile, CpioFile.class, "getSymlinkPath", name);
            ensureNotNull(name);

            Pointer p = CWrapper.mbp_cpiofile_symlink_path(mCCpioFile, name);
            if (p == null) {
                return null;
            }

            return getStringAndFree(p);
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
            return data.getByteArray(0, size);
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
            validate(mCDevice, Device.class, "(Constructor)", destroyable);
        }

        public void destroy() {
            synchronized (sInstances) {
                validate(mCDevice, Device.class, "destroy", mDestroyable);
                if (mCDevice != null && decrementRefCount(sInstances, mCDevice) && mDestroyable) {
                    validate(mCDevice, Device.class, "(Destroyed)");
                    CWrapper.mbp_device_destroy(mCDevice);
                }
                mCDevice = null;
            }
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

        public long getFlags() {
            validate(mCDevice, Device.class, "getFlags");
            return CWrapper.mbp_device_flags(mCDevice);
        }

        public void setFlags(long flags) {
            validate(mCDevice, Device.class, "setFlags", flags);

            CWrapper.mbp_device_set_flags(mCDevice, flags);
        }

        public String getRamdiskPatcher() {
            validate(mCDevice, Device.class, "getRamdiskPatcher");
            Pointer p = CWrapper.mbp_device_ramdisk_patcher(mCDevice);
            return getStringAndFree(p);
        }

        public void setRamdiskPatcher(String id) {
            validate(mCDevice, Device.class, "setRamdiskPatcher", id);
            ensureNotNull(id);

            CWrapper.mbp_device_set_ramdisk_patcher(mCDevice, id);
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
            synchronized (sInstances) {
                validate(mCFileInfo, FileInfo.class, "destroy");
                if (mCFileInfo != null && decrementRefCount(sInstances, mCFileInfo)) {
                    validate(mCFileInfo, FileInfo.class, "(Destroyed)");
                    CWrapper.mbp_fileinfo_destroy(mCFileInfo);
                }
                mCFileInfo = null;
            }
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

        public String getInputPath() {
            validate(mCFileInfo, FileInfo.class, "getInputPath");
            Pointer p = CWrapper.mbp_fileinfo_input_path(mCFileInfo);
            return getStringAndFree(p);
        }

        public void setInputPath(String path) {
            validate(mCFileInfo, FileInfo.class, "setInputPath", path);
            ensureNotNull(path);

            CWrapper.mbp_fileinfo_set_input_path(mCFileInfo, path);
        }

        public String getOutputPath() {
            validate(mCFileInfo, FileInfo.class, "getOutputPath");
            Pointer p = CWrapper.mbp_fileinfo_output_path(mCFileInfo);
            return getStringAndFree(p);
        }

        public void setOutputPath(String path) {
            validate(mCFileInfo, FileInfo.class, "setOutputPath", path);
            ensureNotNull(path);

            CWrapper.mbp_fileinfo_set_output_path(mCFileInfo, path);
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

        public String getRomId() {
            validate(mCFileInfo, FileInfo.class, "getRomId");
            Pointer p = CWrapper.mbp_fileinfo_rom_id(mCFileInfo);
            return getStringAndFree(p);
        }

        public void setRomId(String id) {
            validate(mCFileInfo, FileInfo.class, "setRomId", id);
            ensureNotNull(id);

            CWrapper.mbp_fileinfo_set_rom_id(mCFileInfo, id);
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
            synchronized (sInstances) {
                validate(mCPatcherConfig, PatcherConfig.class, "destroy");
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

        public /* ErrorCode */ int getError() {
            validate(mCPatcherConfig, PatcherConfig.class, "getError");
            return CWrapper.mbp_config_error(mCPatcherConfig);
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

        public AutoPatcher createAutoPatcher(String id, FileInfo info) {
            validate(mCPatcherConfig, PatcherConfig.class, "createAutoPatcher", id, info);
            ensureNotNull(id);
            ensureNotNull(info);

            CAutoPatcher patcher = CWrapper.mbp_config_create_autopatcher(
                    mCPatcherConfig, id, info.getPointer());
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

        public /* ErrorCode */ int getError() {
            validate(mCPatcher, Patcher.class, "getError");
            return CWrapper.mbp_patcher_error(mCPatcher);
        }

        public String getId() {
            validate(mCPatcher, Patcher.class, "getId");
            Pointer p = CWrapper.mbp_patcher_id(mCPatcher);
            return getStringAndFree(p);
        }

        public void setFileInfo(FileInfo info) {
            validate(mCPatcher, Patcher.class, "setFileInfo", info);
            CWrapper.mbp_patcher_set_fileinfo(mCPatcher, info.getPointer());
        }

        public boolean patchFile(final ProgressListener listener) {
            validate(mCPatcher, Patcher.class, "patchFile", listener);
            CWrapper.ProgressUpdatedCallback progressCb = null;
            CWrapper.FilesUpdatedCallback filesCb = null;
            CWrapper.DetailsUpdatedCallback detailsCb = null;

            if (listener != null) {
                progressCb = new CWrapper.ProgressUpdatedCallback() {
                    @Override
                    public void invoke(long bytes, long maxBytes, Pointer userData) {
                        listener.onProgressUpdated(bytes, maxBytes);
                    }
                };

                filesCb = new CWrapper.FilesUpdatedCallback() {
                    @Override
                    public void invoke(long files, long maxFiles, Pointer userData) {
                        listener.onFilesUpdated(files, maxFiles);
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
                    progressCb, filesCb, detailsCb, null);
        }

        public void cancelPatching() {
            validate(mCPatcher, Patcher.class, "cancelPatching");
            CWrapper.mbp_patcher_cancel_patching(mCPatcher);
        }

        public interface ProgressListener {
            void onProgressUpdated(long bytes, long maxBytes);

            void onFilesUpdated(long files, long maxFiles);

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

        public /* ErrorCode */ int getError() {
            validate(mCAutoPatcher, AutoPatcher.class, "getError");
            return CWrapper.mbp_autopatcher_error(mCAutoPatcher);
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

        public /* ErrorCode */ int getError() {
            validate(mCRamdiskPatcher, RamdiskPatcher.class, "getError");
            return CWrapper.mbp_ramdiskpatcher_error(mCRamdiskPatcher);
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
