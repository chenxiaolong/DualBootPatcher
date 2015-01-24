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
        static native int mbp_bootimage_load_data(CBootImage bi, Pointer data, int size);
        static native int mbp_bootimage_load_file(CBootImage bi, String filename);
        static native void mbp_bootimage_create_data(CBootImage bi, PointerByReference dataReturn, /* size_t */ IntByReference size);
        static native int mbp_bootimage_create_file(CBootImage bi, String filename);
        static native int mbp_bootimage_extract(CBootImage bi, String directory, String prefix);
        static native int mbp_bootimage_is_loki(CBootImage bi);
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
        // END: cbootimage.h

        // BEGIN: ccommon.h
        static native void mbp_free(Pointer data);
        static native void mbp_free_array(Pointer array);
        // END: ccommon.h

        // BEGIN: ccpiofile.h
        static native CCpioFile mbp_cpiofile_create();
        static native void mbp_cpiofile_destroy(CCpioFile cpio);
        static native CPatcherError mbp_cpiofile_error(CCpioFile cpio);
        static native int mbp_cpiofile_load_data(CCpioFile cpio, Pointer data, int size);
        static native int mbp_cpiofile_create_data(CCpioFile cpio, PointerByReference dataReturn, /* size_t */ IntByReference size);
        static native int mbp_cpiofile_exists(CCpioFile cpio, String filename);
        static native int mbp_cpiofile_remove(CCpioFile cpio, String filename);
        static native Pointer mbp_cpiofile_filenames(CCpioFile cpio);
        static native int mbp_cpiofile_contents(CCpioFile cpio, String filename, PointerByReference dataReturn, /* size_t */ IntByReference size);
        static native int mbp_cpiofile_set_contents(CCpioFile cpio, String filename, Pointer data, /* size_t */ int size);
        static native int mbp_cpiofile_add_symlink(CCpioFile cpio, String source, String target);
        static native int mbp_cpiofile_add_file(CCpioFile cpio, String path, String name, int perms);
        static native int mbp_cpiofile_add_file_from_data(CCpioFile cpio, Pointer data, /* size_t */ int size, String name, int perms);
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
        static native Pointer mbp_device_system_block_devs(CDevice device);
        static native void mbp_device_set_system_block_devs(CDevice device, StringArray block_devs);
        static native Pointer mbp_device_cache_block_devs(CDevice device);
        static native void mbp_device_set_cache_block_devs(CDevice device, StringArray block_devs);
        static native Pointer mbp_device_data_block_devs(CDevice device);
        static native void mbp_device_set_data_block_devs(CDevice device, StringArray block_devs);
        static native Pointer mbp_device_boot_block_devs(CDevice device);
        static native void mbp_device_set_boot_block_devs(CDevice device, StringArray block_devs);
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
        static native Pointer mbp_config_patcher_name(CPatcherConfig pc, String id);
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
        static native Pointer mbp_error_patcher_name(CPatcherError error);
        static native Pointer mbp_error_filename(CPatcherError error);
        // END: cpatchererror.h

        // BEGIN: cpatchinfo.h
        static native String mbp_patchinfo_default();
        static native String mbp_patchinfo_notmatched();
        static native CPatchInfo mbp_patchinfo_create();
        static native void mbp_patchinfo_destroy(CPatchInfo info);
        static native Pointer mbp_patchinfo_id(CPatchInfo info);
        static native void mbp_patchinfo_set_id(CPatchInfo info, String id);
        static native Pointer mbp_patchinfo_name(CPatchInfo info);
        static native void mbp_patchinfo_set_name(CPatchInfo info, String name);
        static native Pointer mbp_patchinfo_key_from_filename(CPatchInfo info, String fileName);
        static native Pointer mbp_patchinfo_regexes(CPatchInfo info);
        static native void mbp_patchinfo_set_regexes(CPatchInfo info, StringArray regexes);
        static native Pointer mbp_patchinfo_exclude_regexes(CPatchInfo info);
        static native void mbp_patchinfo_set_exclude_regexes(CPatchInfo info, StringArray regexes);
        static native Pointer mbp_patchinfo_cond_regexes(CPatchInfo info);
        static native void mbp_patchinfo_set_cond_regexes(CPatchInfo info, StringArray regexes);
        static native boolean mbp_patchinfo_has_not_matched(CPatchInfo info);
        static native void mbp_patchinfo_set_has_not_matched(CPatchInfo info, boolean hasElem);
        static native void mbp_patchinfo_add_autopatcher(CPatchInfo info, String key, String apName, CStringMap args);
        static native void mbp_patchinfo_remove_autopatcher(CPatchInfo info, String key, String apName);
        static native Pointer mbp_patchinfo_autopatchers(CPatchInfo info, String key);
        static native CStringMap mbp_patchinfo_autopatcher_args(CPatchInfo info, String key, String apName);
        static native boolean mbp_patchinfo_has_boot_image(CPatchInfo info, String key);
        static native void mbp_patchinfo_set_has_boot_image(CPatchInfo info, String key, boolean hasBootImage);
        static native boolean mbp_patchinfo_autodetect_boot_images(CPatchInfo info, String key);
        static native void mbp_patchinfo_set_autodetect_boot_images(CPatchInfo info, String key, boolean autoDetect);
        static native Pointer mbp_patchinfo_boot_images(CPatchInfo info, String key);
        static native void mbp_patchinfo_set_boot_images(CPatchInfo info, String key, StringArray bootImages);
        static native Pointer mbp_patchinfo_ramdisk(CPatchInfo info, String key);
        static native void mbp_patchinfo_set_ramdisk(CPatchInfo info, String key, String ramdisk);
        static native boolean mbp_patchinfo_device_check(CPatchInfo info, String key);
        static native void mbp_patchinfo_set_device_check(CPatchInfo info, String key, boolean deviceCheck);
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
        static native Pointer mbp_patcher_name(CPatcher patcher);
        static native boolean mbp_patcher_uses_patchinfo(CPatcher patcher);
        static native void mbp_patcher_set_fileinfo(CPatcher patcher, CFileInfo info);
        static native Pointer mbp_patcher_new_file_path(CPatcher patcher);
        static native int mbp_patcher_patch_file(CPatcher patcher, MaxProgressUpdatedCallback maxProgressCb, ProgressUpdatedCallback progressCb, DetailsUpdatedCallback detailsCb, Pointer userData);
        static native void mbp_patcher_cancel_patching(CPatcher patcher);

        static native CPatcherError mbp_autopatcher_error(CAutoPatcher patcher);
        static native Pointer mbp_autopatcher_id(CAutoPatcher patcher);
        static native Pointer mbp_autopatcher_new_files(CAutoPatcher patcher);
        static native Pointer mbp_autopatcher_existing_files(CAutoPatcher patcher);
        static native int mbp_autopatcher_patch_files(CAutoPatcher patcher, String directory);

        static native CPatcherError mbp_ramdiskpatcher_error(CRamdiskPatcher patcher);
        static native Pointer mbp_ramdiskpatcher_id(CRamdiskPatcher patcher);
        static native int mbp_ramdiskpatcher_patch_ramdisk(CRamdiskPatcher patcher);
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

    private static void log(PointerType ptr, Class clazz, String method, Object... params) {
        if (DEBUG) {
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

            Log.d("libmbp", sb.toString());
        }
    }

    public static class BootImage implements Parcelable {
        private static HashMap<CBootImage, Integer> sInstances = new HashMap<CBootImage, Integer>();
        private CBootImage mCBootImage;

        public BootImage() {
            mCBootImage = CWrapper.mbp_bootimage_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCBootImage);
            }
            log(mCBootImage, BootImage.class, "(Constructor)");
        }

        BootImage(CBootImage cBootImage) {
            ensureNotNull(cBootImage);

            mCBootImage = cBootImage;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCBootImage);
            }
            log(mCBootImage, BootImage.class, "(Constructor)");
        }

        public void destroy() {
            log(mCBootImage, BootImage.class, "destroy");
            synchronized (sInstances) {
                if (mCBootImage != null && decrementRefCount(sInstances, mCBootImage)) {
                    log(mCBootImage, BootImage.class, "(Destroyed)");
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
        public void finalize() {
            destroy();
        }

        CBootImage getPointer() {
            log(mCBootImage, BootImage.class, "getPointer");
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
            log(mCBootImage, BootImage.class, "(Constructor)");
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
            log(mCBootImage, BootImage.class, "getError");
            CPatcherError error = CWrapper.mbp_bootimage_error(mCBootImage);
            return new PatcherError(error);
        }

        public boolean load(byte[] data) {
            log(mCBootImage, BootImage.class, "load", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            int ret = CWrapper.mbp_bootimage_load_data(mCBootImage, mem, data.length);
            return ret == 0;
        }

        public boolean load(String filename) {
            log(mCBootImage, BootImage.class, "load", filename);
            ensureNotNull(filename);

            return CWrapper.mbp_bootimage_load_file(mCBootImage, filename) == 0;
        }

        public byte[] create() {
            log(mCBootImage, BootImage.class, "create");
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
            log(mCBootImage, BootImage.class, "createFile", path);
            ensureNotNull(path);

            return CWrapper.mbp_bootimage_create_file(mCBootImage, path) == 0;
        }

        public boolean extract(String directory, String prefix) {
            log(mCBootImage, BootImage.class, "extract", directory, prefix);
            ensureNotNull(directory);
            ensureNotNull(prefix);

            return CWrapper.mbp_bootimage_extract(mCBootImage, directory, prefix) == 0;
        }

        public boolean isLoki() {
            log(mCBootImage, BootImage.class, "isLoki");

            return CWrapper.mbp_bootimage_is_loki(mCBootImage) == 0;
        }

        public String getBoardName() {
            log(mCBootImage, BootImage.class, "getBoardName");
            Pointer p = CWrapper.mbp_bootimage_boardname(mCBootImage);
            return getStringAndFree(p);
        }

        public void setBoardName(String name) {
            log(mCBootImage, BootImage.class, "setBoardName", name);
            ensureNotNull(name);

            CWrapper.mbp_bootimage_set_boardname(mCBootImage, name);
        }

        public void resetBoardName() {
            log(mCBootImage, BootImage.class, "resetBoardName");
            CWrapper.mbp_bootimage_reset_boardname(mCBootImage);
        }

        public String getKernelCmdline() {
            log(mCBootImage, BootImage.class, "getKernelCmdline");
            Pointer p = CWrapper.mbp_bootimage_kernel_cmdline(mCBootImage);
            return getStringAndFree(p);
        }

        public void setKernelCmdline(String cmdline) {
            log(mCBootImage, BootImage.class, "setKernelCmdline", cmdline);
            ensureNotNull(cmdline);

            CWrapper.mbp_bootimage_set_kernel_cmdline(mCBootImage, cmdline);
        }

        public void resetKernelCmdline() {
            log(mCBootImage, BootImage.class, "resetKernelCmdline");
            CWrapper.mbp_bootimage_reset_kernel_cmdline(mCBootImage);
        }

        public int getPageSize() {
            log(mCBootImage, BootImage.class, "getPageSize");
            return CWrapper.mbp_bootimage_page_size(mCBootImage);
        }

        public void setPageSize(int size) {
            log(mCBootImage, BootImage.class, "setPageSize", size);
            CWrapper.mbp_bootimage_set_page_size(mCBootImage, size);
        }

        public void resetPageSize() {
            log(mCBootImage, BootImage.class, "resetPageSize");
            CWrapper.mbp_bootimage_reset_page_size(mCBootImage);
        }

        public int getKernelAddress() {
            log(mCBootImage, BootImage.class, "getKernelAddress");
            return CWrapper.mbp_bootimage_kernel_address(mCBootImage);
        }

        public void setKernelAddress(int address) {
            log(mCBootImage, BootImage.class, "setKernelAddress", address);
            CWrapper.mbp_bootimage_set_kernel_address(mCBootImage, address);
        }

        public void resetKernelAddress() {
            log(mCBootImage, BootImage.class, "resetKernelAddress");
            CWrapper.mbp_bootimage_reset_kernel_address(mCBootImage);
        }

        public int getRamdiskAddress() {
            log(mCBootImage, BootImage.class, "getRamdiskAddress");
            return CWrapper.mbp_bootimage_ramdisk_address(mCBootImage);
        }

        public void setRamdiskAddress(int address) {
            log(mCBootImage, BootImage.class, "setRamdiskAddress", address);
            CWrapper.mbp_bootimage_set_ramdisk_address(mCBootImage, address);
        }

        public void resetRamdiskAddress() {
            log(mCBootImage, BootImage.class, "resetRamdiskAddress");
            CWrapper.mbp_bootimage_reset_ramdisk_address(mCBootImage);
        }

        public int getSecondBootloaderAddress() {
            log(mCBootImage, BootImage.class, "getSecondBootloaderAddress");
            return CWrapper.mbp_bootimage_second_bootloader_address(mCBootImage);
        }

        public void setSecondBootloaderAddress(int address) {
            log(mCBootImage, BootImage.class, "setSecondBootloaderAddress", address);
            CWrapper.mbp_bootimage_set_second_bootloader_address(mCBootImage, address);
        }

        public void resetSecondBootloaderAddress() {
            log(mCBootImage, BootImage.class, "resetSecondBootloaderAddress");
            CWrapper.mbp_bootimage_reset_second_bootloader_address(mCBootImage);
        }

        public int getKernelTagsAddress() {
            log(mCBootImage, BootImage.class, "getKernelTagsAddress");
            return CWrapper.mbp_bootimage_kernel_tags_address(mCBootImage);
        }

        public void setKernelTagsAddress(int address) {
            log(mCBootImage, BootImage.class, "setKernelTagsAddress", address);
            CWrapper.mbp_bootimage_set_kernel_tags_address(mCBootImage, address);
        }

        public void resetKernelTagsAddress() {
            log(mCBootImage, BootImage.class, "resetKernelTagsAddress");
            CWrapper.mbp_bootimage_reset_kernel_tags_address(mCBootImage);
        }

        public void setAddresses(int base, int kernelOffset, int ramdiskOffset,
                                 int secondBootloaderOffset, int kernelTagsOffset) {
            log(mCBootImage, BootImage.class, "setAddresses", base, kernelOffset, ramdiskOffset,
                    secondBootloaderOffset, kernelTagsOffset);
            CWrapper.mbp_bootimage_set_addresses(mCBootImage, base, kernelOffset,
                    ramdiskOffset, secondBootloaderOffset, kernelTagsOffset);
        }

        public byte[] getKernelImage() {
            log(mCBootImage, BootImage.class, "getKernelImage");
            PointerByReference pData = new PointerByReference();
            int size = CWrapper.mbp_bootimage_kernel_image(mCBootImage, pData);
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public void setKernelImage(byte[] data) {
            log(mCBootImage, BootImage.class, "setKernelImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_kernel_image(mCBootImage, mem, data.length);
        }

        public byte[] getRamdiskImage() {
            log(mCBootImage, BootImage.class, "getRamdiskImage");
            PointerByReference pData = new PointerByReference();
            int size = CWrapper.mbp_bootimage_ramdisk_image(mCBootImage, pData);
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public void setRamdiskImage(byte[] data) {
            log(mCBootImage, BootImage.class, "setRamdiskImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_ramdisk_image(mCBootImage, mem, data.length);
        }

        public byte[] getSecondBootloaderImage() {
            log(mCBootImage, BootImage.class, "getSecondBootloaderImage");
            PointerByReference pData = new PointerByReference();
            int size = CWrapper.mbp_bootimage_second_bootloader_image(mCBootImage, pData);
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public void setSecondBootloaderImage(byte[] data) {
            log(mCBootImage, BootImage.class, "setSecondBootloaderImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_second_bootloader_image(mCBootImage, mem, data.length);
        }

        public byte[] getDeviceTreeImage() {
            log(mCBootImage, BootImage.class, "getDeviceTreeImages");
            PointerByReference pData = new PointerByReference();
            int size = CWrapper.mbp_bootimage_device_tree_image(mCBootImage, pData);
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public void setDeviceTreeImage(byte[] data) {
            log(mCBootImage, BootImage.class, "setDeviceTreeImage", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            CWrapper.mbp_bootimage_set_device_tree_image(mCBootImage, mem, data.length);
        }
    }

    public static class CpioFile implements Parcelable {
        private static HashMap<CCpioFile, Integer> sInstances = new HashMap<CCpioFile, Integer>();
        private CCpioFile mCCpioFile;

        public CpioFile() {
            mCCpioFile = CWrapper.mbp_cpiofile_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCCpioFile);
            }
            log(mCCpioFile, CpioFile.class, "(Constructor)");
        }

        CpioFile(CCpioFile cCpioFile) {
            ensureNotNull(cCpioFile);

            mCCpioFile = cCpioFile;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCCpioFile);
            }
            log(mCCpioFile, CpioFile.class, "(Constructor)");
        }

        public void destroy() {
            log(mCCpioFile, CpioFile.class, "destroy");
            synchronized (sInstances) {
                if (mCCpioFile != null && decrementRefCount(sInstances, mCCpioFile)) {
                    log(mCCpioFile, CpioFile.class, "(Destroyed)");
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
        public void finalize() {
            destroy();
        }

        CCpioFile getPointer() {
            log(mCCpioFile, CpioFile.class, "getPointer");
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
            log(mCCpioFile, CpioFile.class, "(Constructor)");
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
            log(mCCpioFile, CpioFile.class, "getError");
            CPatcherError error = CWrapper.mbp_cpiofile_error(mCCpioFile);
            return new PatcherError(error);
        }

        public boolean load(byte[] data) {
            log(mCCpioFile, CpioFile.class, "load", data.length);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            int ret = CWrapper.mbp_cpiofile_load_data(mCCpioFile, mem, data.length);
            return ret == 0;
        }

        public byte[] createData() {
            log(mCCpioFile, CpioFile.class, "createData");
            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();

            int ret = CWrapper.mbp_cpiofile_create_data(mCCpioFile, pData, pSize);

            if (ret != 0) {
                return null;
            }

            int size = pSize.getValue();
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public boolean isExists(String name) {
            log(mCCpioFile, CpioFile.class, "isExists", name);
            ensureNotNull(name);

            return CWrapper.mbp_cpiofile_exists(mCCpioFile, name) == 0;
        }

        public boolean remove(String name) {
            log(mCCpioFile, CpioFile.class, "remove", name);
            ensureNotNull(name);

            return CWrapper.mbp_cpiofile_remove(mCCpioFile, name) == 0;
        }

        public String[] getFilenames() {
            log(mCCpioFile, CpioFile.class, "getFilenames");
            Pointer p = CWrapper.mbp_cpiofile_filenames(mCCpioFile);
            return getStringArrayAndFree(p);
        }

        public byte[] getContents(String name) {
            log(mCCpioFile, CpioFile.class, "getContents", name);
            ensureNotNull(name);

            PointerByReference pData = new PointerByReference();
            IntByReference pSize = new IntByReference();

            int ret = CWrapper.mbp_cpiofile_contents(mCCpioFile, name, pData, pSize);

            if (ret != 0) {
                return null;
            }

            int size = pSize.getValue();
            Pointer data = pData.getValue();
            byte[] out = data.getByteArray(0, size);
            CWrapper.mbp_free(data);
            return out;
        }

        public boolean setContents(String name, byte[] data) {
            log(mCCpioFile, CpioFile.class, "setContents", name, data.length);
            ensureNotNull(name);
            ensureNotNull(data);

            Memory mem = new Memory(data.length);
            mem.write(0, data, 0, data.length);
            int ret = CWrapper.mbp_cpiofile_set_contents(mCCpioFile, name, mem, data.length);
            return ret == 0;
        }

        public boolean addSymlink(String source, String target) {
            log(mCCpioFile, CpioFile.class, "addSymlink", source, target);
            ensureNotNull(source);
            ensureNotNull(target);

            return CWrapper.mbp_cpiofile_add_symlink(mCCpioFile, source, target) == 0;
        }

        public boolean addFile(String path, String name, int perms) {
            log(mCCpioFile, CpioFile.class, "addFile", path, name, perms);
            ensureNotNull(path);
            ensureNotNull(name);

            return CWrapper.mbp_cpiofile_add_file(mCCpioFile, path, name, perms) == 0;
        }

        public boolean addFile(byte[] contents, String name, int perms) {
            log(mCCpioFile, CpioFile.class, "addFile", contents.length, name, perms);
            ensureNotNull(contents);
            ensureNotNull(name);

            Memory mem = new Memory(contents.length);
            mem.write(0, contents, 0, contents.length);
            int ret = CWrapper.mbp_cpiofile_add_file_from_data(
                    mCCpioFile, mem, contents.length, name, perms);
            return ret == 0;
        }
    }

    public static class Device implements Parcelable {
        private static HashMap<CDevice, Integer> sInstances = new HashMap<CDevice, Integer>();
        private CDevice mCDevice;
        private boolean mDestroyable;

        public Device() {
            mCDevice = CWrapper.mbp_device_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCDevice);
            }
            mDestroyable = true;
            log(mCDevice, Device.class, "(Constructor)");
        }

        Device(CDevice cDevice, boolean destroyable) {
            ensureNotNull(cDevice);

            mCDevice = cDevice;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCDevice);
            }
            mDestroyable = destroyable;
            log(mCDevice, Device.class, "(Constructor)");
        }

        public void destroy() {
            log(mCDevice, Device.class, "destroy");
            synchronized (sInstances) {
                if (mCDevice != null && decrementRefCount(sInstances, mCDevice) && mDestroyable) {
                    log(mCDevice, Device.class, "(Destroyed)");
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
        public void finalize() {
            destroy();
        }

        CDevice getPointer() {
            log(mCDevice, Device.class, "getPointer");
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
            log(mCDevice, Device.class, "(Constructor)");
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
            log(mCDevice, Device.class, "getId");
            Pointer p = CWrapper.mbp_device_id(mCDevice);
            return getStringAndFree(p);
        }

        public void setId(String id) {
            log(mCDevice, Device.class, "setId", id);
            ensureNotNull(id);

            CWrapper.mbp_device_set_id(mCDevice, id);
        }

        public String[] getCodenames() {
            log(mCDevice, Device.class, "getCodenames");
            Pointer p = CWrapper.mbp_device_codenames(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setCodenames(String[] names) {
            log(mCDevice, Device.class, "setCodenames", (Object) names);
            ensureNotNull(names);

            CWrapper.mbp_device_set_codenames(mCDevice, new StringArray(names));
        }

        public String getName() {
            log(mCDevice, Device.class, "getName");
            Pointer p = CWrapper.mbp_device_name(mCDevice);
            return getStringAndFree(p);
        }

        public void setName(String name) {
            log(mCDevice, Device.class, "setName", name);
            ensureNotNull(name);

            CWrapper.mbp_device_set_name(mCDevice, name);
        }

        public String getArchitecture() {
            log(mCDevice, Device.class, "getArchitecture");
            Pointer p = CWrapper.mbp_device_architecture(mCDevice);
            return getStringAndFree(p);
        }

        public void setArchitecture(String arch) {
            log(mCDevice, Device.class, "setArchitecture", arch);
            ensureNotNull(arch);

            CWrapper.mbp_device_set_architecture(mCDevice, arch);
        }

        public String[] getSystemBlockDevs() {
            log(mCDevice, Device.class, "getSystemBlockDevs");
            Pointer p = CWrapper.mbp_device_system_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setSystemBlockDevs(String[] blockDevs) {
            log(mCDevice, Device.class, "setSystemBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_system_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getCacheBlockDevs() {
            log(mCDevice, Device.class, "getCacheBlockDevs");
            Pointer p = CWrapper.mbp_device_cache_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setCacheBlockDevs(String[] blockDevs) {
            log(mCDevice, Device.class, "setCacheBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_cache_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getDataBlockDevs() {
            log(mCDevice, Device.class, "getDataBlockDevs");
            Pointer p = CWrapper.mbp_device_data_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setDataBlockDevs(String[] blockDevs) {
            log(mCDevice, Device.class, "setDataBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_data_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getBootBlockDevs() {
            log(mCDevice, Device.class, "getBootBlockDevs");
            Pointer p = CWrapper.mbp_device_boot_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setBootBlockDevs(String[] blockDevs) {
            log(mCDevice, Device.class, "setBootBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_boot_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getExtraBlockDevs() {
            log(mCDevice, Device.class, "getExtraBlockDevs");
            Pointer p = CWrapper.mbp_device_extra_block_devs(mCDevice);
            return getStringArrayAndFree(p);
        }

        public void setExtraBlockDevs(String[] blockDevs) {
            log(mCDevice, Device.class, "setExtraBlockDevs", (Object) blockDevs);
            ensureNotNull(blockDevs);

            CWrapper.mbp_device_set_extra_block_devs(mCDevice, new StringArray(blockDevs));
        }
    }

    public static class FileInfo implements Parcelable {
        private static HashMap<CFileInfo, Integer> sInstances = new HashMap<CFileInfo, Integer>();
        private CFileInfo mCFileInfo;

        public FileInfo() {
            mCFileInfo = CWrapper.mbp_fileinfo_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCFileInfo);
            }
            log(mCFileInfo, FileInfo.class, "(Constructor)");
        }

        FileInfo(CFileInfo cFileInfo) {
            ensureNotNull(cFileInfo);

            mCFileInfo = cFileInfo;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCFileInfo);
            }
            log(mCFileInfo, FileInfo.class, "(Constructor)");
        }

        public void destroy() {
            log(mCFileInfo, FileInfo.class, "destroy");
            synchronized (sInstances) {
                if (mCFileInfo != null && decrementRefCount(sInstances, mCFileInfo)) {
                    log(mCFileInfo, FileInfo.class, "(Destroyed)");
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
        public void finalize() {
            destroy();
        }

        CFileInfo getPointer() {
            log(mCFileInfo, FileInfo.class, "getPointer");
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
            log(mCFileInfo, FileInfo.class, "(Constructor)");
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
            log(mCFileInfo, FileInfo.class, "getFilename");
            Pointer p = CWrapper.mbp_fileinfo_filename(mCFileInfo);
            return getStringAndFree(p);
        }

        public void setFilename(String path) {
            log(mCFileInfo, FileInfo.class, "setFilename", path);
            ensureNotNull(path);

            CWrapper.mbp_fileinfo_set_filename(mCFileInfo, path);
        }

        public PatchInfo getPatchInfo() {
            log(mCFileInfo, FileInfo.class, "getPatchInfo");
            CPatchInfo cPi = CWrapper.mbp_fileinfo_patchinfo(mCFileInfo);
            return cPi == null ? null : new PatchInfo(cPi, false);
        }

        public void setPatchInfo(PatchInfo info) {
            log(mCFileInfo, FileInfo.class, "setPatchInfo", info);
            ensureNotNull(info);

            CWrapper.mbp_fileinfo_set_patchinfo(mCFileInfo, info.getPointer());
        }

        public Device getDevice() {
            log(mCFileInfo, FileInfo.class, "getDevice");
            CDevice cDevice = CWrapper.mbp_fileinfo_device(mCFileInfo);
            return cDevice == null ? null : new Device(cDevice, false);
        }

        public void setDevice(Device device) {
            log(mCFileInfo, FileInfo.class, "setDevice", device);
            ensureNotNull(device);

            CWrapper.mbp_fileinfo_set_device(mCFileInfo, device.getPointer());
        }
    }

    public static class PatcherConfig implements Parcelable {
        private static HashMap<CPatcherConfig, Integer> sInstances =
                new HashMap<CPatcherConfig, Integer>();
        private CPatcherConfig mCPatcherConfig;

        public PatcherConfig() {
            mCPatcherConfig = CWrapper.mbp_config_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatcherConfig);
            }
            log(mCPatcherConfig, PatcherConfig.class, "(Constructor)");
        }

        PatcherConfig(CPatcherConfig cPatcherConfig) {
            ensureNotNull(cPatcherConfig);

            mCPatcherConfig = cPatcherConfig;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatcherConfig);
            }
            log(mCPatcherConfig, PatcherConfig.class, "(Constructor)");
        }

        public void destroy() {
            log(mCPatcherConfig, PatcherConfig.class, "destroy");
            synchronized (sInstances) {
                if (mCPatcherConfig != null && decrementRefCount(sInstances, mCPatcherConfig)) {
                    log(mCPatcherConfig, PatcherConfig.class, "(Destroyed)");
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
        public void finalize() {
            destroy();
        }

        CPatcherConfig getPointer() {
            log(mCPatcherConfig, PatcherConfig.class, "getPointer");
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
            log(mCPatcherConfig, PatcherConfig.class, "(Constructor)");
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
            log(mCPatcherConfig, PatcherConfig.class, "getError");
            CPatcherError error = CWrapper.mbp_config_error(mCPatcherConfig);
            return new PatcherError(error);
        }

        public String getDataDirectory() {
            log(mCPatcherConfig, PatcherConfig.class, "getDataDirectory");
            Pointer p = CWrapper.mbp_config_data_directory(mCPatcherConfig);
            return getStringAndFree(p);
        }

        public String getTempDirectory() {
            log(mCPatcherConfig, PatcherConfig.class, "getTempDirectory");
            Pointer p = CWrapper.mbp_config_temp_directory(mCPatcherConfig);
            return getStringAndFree(p);
        }

        public void setDataDirectory(String path) {
            log(mCPatcherConfig, PatcherConfig.class, "setDataDirectory", path);
            ensureNotNull(path);

            CWrapper.mbp_config_set_data_directory(mCPatcherConfig, path);
        }

        public void setTempDirectory(String path) {
            log(mCPatcherConfig, PatcherConfig.class, "setTempDirectory", path);
            ensureNotNull(path);

            CWrapper.mbp_config_set_temp_directory(mCPatcherConfig, path);
        }

        public String getVersion() {
            log(mCPatcherConfig, PatcherConfig.class, "getVersion");
            Pointer p = CWrapper.mbp_config_version(mCPatcherConfig);
            return getStringAndFree(p);
        }

        public Device[] getDevices() {
            log(mCPatcherConfig, PatcherConfig.class, "getDevices");
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
            log(mCPatcherConfig, PatcherConfig.class, "getPatchInfos");
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
            log(mCPatcherConfig, PatcherConfig.class, "getPatchInfos", device);
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
            log(mCPatcherConfig, PatcherConfig.class, "findMatchingPatchInfo", device, filename);
            ensureNotNull(device);
            ensureNotNull(filename);

            CPatchInfo cPi = CWrapper.mbp_config_find_matching_patchinfo(
                    mCPatcherConfig, device.getPointer(), filename);
            return cPi == null ? null : new PatchInfo(cPi, false);
        }

        public String[] getPatchers() {
            log(mCPatcherConfig, PatcherConfig.class, "getPatchers");
            Pointer p = CWrapper.mbp_config_patchers(mCPatcherConfig);
            return getStringArrayAndFree(p);
        }

        public String[] getAutoPatchers() {
            log(mCPatcherConfig, PatcherConfig.class, "getAutoPatchers");
            Pointer p = CWrapper.mbp_config_autopatchers(mCPatcherConfig);
            return getStringArrayAndFree(p);
        }

        public String[] getRamdiskPatchers() {
            log(mCPatcherConfig, PatcherConfig.class, "getRamdiskPatchers");
            Pointer p = CWrapper.mbp_config_ramdiskpatchers(mCPatcherConfig);
            return getStringArrayAndFree(p);
        }

        public String getPatcherName(String id) {
            log(mCPatcherConfig, PatcherConfig.class, "getPatcherName", id);
            ensureNotNull(id);

            Pointer p = CWrapper.mbp_config_patcher_name(mCPatcherConfig, id);
            return getStringAndFree(p);
        }

        public Patcher createPatcher(String id) {
            log(mCPatcherConfig, PatcherConfig.class, "createPatcher", id);
            ensureNotNull(id);

            CPatcher patcher = CWrapper.mbp_config_create_patcher(mCPatcherConfig, id);
            return patcher == null ? null : new Patcher(patcher);
        }

        public AutoPatcher createAutoPatcher(String id, FileInfo info, StringMap args) {
            log(mCPatcherConfig, PatcherConfig.class, "createAutoPatcher", id, info, args);
            ensureNotNull(id);
            ensureNotNull(info);
            ensureNotNull(args);

            CAutoPatcher patcher = CWrapper.mbp_config_create_autopatcher(
                    mCPatcherConfig, id, info.getPointer(), args.getPointer());
            return patcher == null ? null : new AutoPatcher(patcher);
        }

        public RamdiskPatcher createRamdiskPatcher(String id, FileInfo info, CpioFile cpio) {
            log(mCPatcherConfig, PatcherConfig.class, "createRamdiskPatcher", id, info, cpio);
            ensureNotNull(id);
            ensureNotNull(info);
            ensureNotNull(cpio);

            CRamdiskPatcher patcher = CWrapper.mbp_config_create_ramdisk_patcher(
                    mCPatcherConfig, id, info.getPointer(), cpio.getPointer());
            return patcher == null ? null : new RamdiskPatcher(patcher);
        }

        public void destroyPatcher(Patcher patcher) {
            log(mCPatcherConfig, PatcherConfig.class, "destroyPatcher", patcher);
            ensureNotNull(patcher);

            CWrapper.mbp_config_destroy_patcher(mCPatcherConfig, patcher.getPointer());
        }

        public void destroyAutoPatcher(AutoPatcher patcher) {
            log(mCPatcherConfig, PatcherConfig.class, "destroyAutoPatcher", patcher);
            ensureNotNull(patcher);

            CWrapper.mbp_config_destroy_autopatcher(mCPatcherConfig, patcher.getPointer());
        }

        public void destroyRamdiskPatcher(RamdiskPatcher patcher) {
            log(mCPatcherConfig, PatcherConfig.class, "destroyRamdiskPatcher", patcher);
            ensureNotNull(patcher);

            CWrapper.mbp_config_destroy_ramdisk_patcher(mCPatcherConfig, patcher.getPointer());
        }

        public boolean loadPatchInfos() {
            log(mCPatcherConfig, PatcherConfig.class, "loadPatchInfos");
            return CWrapper.mbp_config_load_patchinfos(mCPatcherConfig);
        }
    }

    public static class PatcherError implements Parcelable {
        public static interface ErrorType {
            public static int GENERIC_ERROR = 0;
            public static int PATCHER_CREATION_ERROR = 1;
            public static int IO_ERROR = 2;
            public static int BOOT_IMAGE_ERROR = 3;
            public static int CPIO_ERROR = 4;
            public static int ARCHIVE_ERROR = 5;
            public static int XML_ERROR = 6;
            public static int SUPPORTED_FILE_ERROR = 7;
            public static int CANCELLED_ERROR = 8;
            public static int PATCHING_ERROR = 9;
        }

        public static interface ErrorCode {
            public static int NO_ERROR = 0;
            public static int UNKNOWN_ERROR = 1;
            public static int PATCHER_CREATE_ERROR = 2;
            public static int AUTOPATCHER_CREATE_ERROR = 3;
            public static int RAMDISK_PATCHER_CREATE_ERROR = 4;
            public static int FILE_OPEN_ERROR = 5;
            public static int FILE_READ_ERROR = 6;
            public static int FILE_WRITE_ERROR = 7;
            public static int DIRECTORY_NOT_EXIST_ERROR = 8;
            public static int BOOT_IMAGE_SMALLER_THAN_HEADER_ERROR = 9;
            public static int BOOT_IMAGE_NO_ANDROID_HEADER_ERROR = 10;
            public static int BOOT_IMAGE_NO_RAMDISK_GZIP_HEADER_ERROR = 11;
            public static int BOOT_IMAGE_NO_RAMDISK_SIZE_ERROR = 12;
            public static int BOOT_IMAGE_NO_KERNEL_SIZE_ERROR = 13;
            public static int BOOT_IMAGE_NO_RAMDISK_ADDRESS_ERROR = 14;
            public static int CPIO_FILE_ALREADY_EXIST_ERROR = 15;
            public static int CPIO_FILE_NOT_EXIST_ERROR = 16;
            public static int ARCHIVE_READ_OPEN_ERROR = 17;
            public static int ARCHIVE_READ_DATA_ERROR = 18;
            public static int ARCHIVE_READ_HEADER_ERROR = 19;
            public static int ARCHIVE_WRITE_OPEN_ERROR = 20;
            public static int ARCHIVE_WRITE_DATA_ERROR = 21;
            public static int ARCHIVE_WRITE_HEADER_ERROR = 22;
            public static int ARCHIVE_CLOSE_ERROR = 23;
            public static int ARCHIVE_FREE_ERROR = 24;
            public static int XML_PARSE_FILE_ERROR = 25;
            public static int ONLY_ZIP_SUPPORTED = 26;
            public static int ONLY_BOOT_IMAGE_SUPPORTED = 27;
            public static int PATCHING_CANCELLED = 28;
            public static int SYSTEM_CACHE_FORMAT_LINES_NOT_FOUND = 29;
            public static int APPLY_PATCH_FILE_ERROR = 30;
        }

        private static HashMap<CPatcherError, Integer> sInstances =
                new HashMap<CPatcherError, Integer>();
        private CPatcherError mCPatcherError;

        PatcherError(CPatcherError cPatcherError) {
            ensureNotNull(cPatcherError);

            mCPatcherError = cPatcherError;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatcherError);
            }
            log(mCPatcherError, PatcherError.class, "(Constructor)");
        }

        public void destroy() {
            log(mCPatcherError, PatcherError.class, "destroy");
            synchronized (sInstances) {
                if (mCPatcherError != null && decrementRefCount(sInstances, mCPatcherError)) {
                    log(mCPatcherError, PatcherError.class, "(Destroyed)");
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
        public void finalize() {
            destroy();
        }

        CPatcherError getPointer() {
            log(mCPatcherError, PatcherError.class, "getPointer");
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
            log(mCPatcherError, PatcherError.class, "(Constructor)");
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
            log(mCPatcherError, PatcherError.class, "getErrorType");
            return CWrapper.mbp_error_error_type(mCPatcherError);
        }

        public /* ErrorCode */ int getErrorCode() {
            log(mCPatcherError, PatcherError.class, "getErrorCode");
            return CWrapper.mbp_error_error_code(mCPatcherError);
        }

        public String getPatcherName() {
            log(mCPatcherError, PatcherError.class, "getPatcherName");
            Pointer p = CWrapper.mbp_error_patcher_name(mCPatcherError);
            return getStringAndFree(p);
        }

        public String getFilename() {
            log(mCPatcherError, PatcherError.class, "getFilename");
            Pointer p = CWrapper.mbp_error_filename(mCPatcherError);
            return getStringAndFree(p);
        }
    }

    public static class PatchInfo implements Parcelable {
        private static HashMap<CPatchInfo, Integer> sInstances = new HashMap<CPatchInfo, Integer>();
        private CPatchInfo mCPatchInfo;
        private boolean mDestroyable;

        public PatchInfo() {
            mCPatchInfo = CWrapper.mbp_patchinfo_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatchInfo);
            }
            mDestroyable = true;
            log(mCPatchInfo, PatchInfo.class, "(Constructor)");
        }

        PatchInfo(CPatchInfo cPatchInfo, boolean destroyable) {
            ensureNotNull(cPatchInfo);

            mCPatchInfo = cPatchInfo;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCPatchInfo);
            }
            mDestroyable = destroyable;
            log(mCPatchInfo, PatchInfo.class, "(Constructor)");
        }

        public void destroy() {
            log(mCPatchInfo, PatchInfo.class, "destroy");
            synchronized (sInstances) {
                if (mCPatchInfo != null && decrementRefCount(sInstances, mCPatchInfo)
                        && mDestroyable) {
                    log(mCPatchInfo, PatchInfo.class, "(Destroyed)");
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
        public void finalize() {
            destroy();
        }

        CPatchInfo getPointer() {
            log(mCPatchInfo, PatchInfo.class, "getPointer");
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
            log(mCPatchInfo, PatchInfo.class, "(Constructor)");
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

        public static String Default() {
            return CWrapper.mbp_patchinfo_default();
        }

        public static String NotMatched() {
            return CWrapper.mbp_patchinfo_notmatched();
        }

        public String getId() {
            log(mCPatchInfo, PatchInfo.class, "getId");
            Pointer p = CWrapper.mbp_patchinfo_id(mCPatchInfo);
            return getStringAndFree(p);
        }

        public void setId(String id) {
            log(mCPatchInfo, PatchInfo.class, "setId", id);
            ensureNotNull(id);

            CWrapper.mbp_patchinfo_set_id(mCPatchInfo, id);
        }

        public String getName() {
            log(mCPatchInfo, PatchInfo.class, "getName");
            Pointer p = CWrapper.mbp_patchinfo_name(mCPatchInfo);
            return getStringAndFree(p);
        }

        public void setName(String name) {
            log(mCPatchInfo, PatchInfo.class, "setName", name);
            ensureNotNull(name);

            CWrapper.mbp_patchinfo_set_name(mCPatchInfo, name);
        }

        public String keyFromFilename(String fileName) {
            log(mCPatchInfo, PatchInfo.class, "keyFromFilename", fileName);
            ensureNotNull(fileName);

            Pointer p = CWrapper.mbp_patchinfo_key_from_filename(mCPatchInfo, fileName);
            return getStringAndFree(p);
        }

        public String[] getRegexes() {
            log(mCPatchInfo, PatchInfo.class, "getRegexes");
            Pointer p = CWrapper.mbp_patchinfo_regexes(mCPatchInfo);
            return getStringArrayAndFree(p);
        }

        public void setRegexes(String[] regexes) {
            log(mCPatchInfo, PatchInfo.class, "setRegexes", (Object) regexes);
            ensureNotNull(regexes);

            CWrapper.mbp_patchinfo_set_regexes(mCPatchInfo, new StringArray(regexes));
        }

        public String[] getExcludeRegexes() {
            log(mCPatchInfo, PatchInfo.class, "getExcludeRegexes");
            Pointer p = CWrapper.mbp_patchinfo_exclude_regexes(mCPatchInfo);
            return getStringArrayAndFree(p);
        }

        public void setExcludeRegexes(String[] regexes) {
            log(mCPatchInfo, PatchInfo.class, "setExcludeRegexes", (Object) regexes);
            ensureNotNull(regexes);

            CWrapper.mbp_patchinfo_set_exclude_regexes(mCPatchInfo, new StringArray(regexes));
        }

        public String[] getCondRegexes() {
            log(mCPatchInfo, PatchInfo.class, "getCondRegexes");
            Pointer p = CWrapper.mbp_patchinfo_cond_regexes(mCPatchInfo);
            return getStringArrayAndFree(p);
        }

        public void setCondRegexes(String[] regexes) {
            log(mCPatchInfo, PatchInfo.class, "setCondRegexes", (Object) regexes);
            ensureNotNull(regexes);

            CWrapper.mbp_patchinfo_set_cond_regexes(mCPatchInfo, new StringArray(regexes));
        }

        public boolean hasNotMatched() {
            log(mCPatchInfo, PatchInfo.class, "hasNotMatched");
            return CWrapper.mbp_patchinfo_has_not_matched(mCPatchInfo);
        }

        public void setHasNotMatched(boolean hasElem) {
            log(mCPatchInfo, PatchInfo.class, "setHasNotMatched", hasElem);
            CWrapper.mbp_patchinfo_set_has_not_matched(mCPatchInfo, hasElem);
        }

        public void addAutoPatcher(String key, String apName, StringMap args) {
            log(mCPatchInfo, PatchInfo.class, "addAutoPatcher", key, apName, args);
            ensureNotNull(key);
            ensureNotNull(apName);

            if (args == null) {
                args = new StringMap();
            }

            CStringMap cArgs = args.getPointer();
            CWrapper.mbp_patchinfo_add_autopatcher(mCPatchInfo, key, apName, cArgs);
        }

        public void removeAutoPatcher(String key, String apName) {
            log(mCPatchInfo, PatchInfo.class, "removeAutoPatcher", key, apName);
            ensureNotNull(key);
            ensureNotNull(apName);

            CWrapper.mbp_patchinfo_remove_autopatcher(mCPatchInfo, key, apName);
        }

        public String[] getAutoPatchers(String key) {
            log(mCPatchInfo, PatchInfo.class, "getAutoPatchers", key);
            ensureNotNull(key);

            Pointer p = CWrapper.mbp_patchinfo_autopatchers(mCPatchInfo, key);
            return getStringArrayAndFree(p);
        }

        public StringMap getAutoPatcherArgs(String key, String apName) {
            log(mCPatchInfo, PatchInfo.class, "getAutoPatcherArgs", key, apName);
            ensureNotNull(key);
            ensureNotNull(apName);

            CStringMap args = CWrapper.mbp_patchinfo_autopatcher_args(mCPatchInfo, key, apName);
            return new StringMap(args);
        }

        public boolean hasBootImage(String key) {
            log(mCPatchInfo, PatchInfo.class, "hasBootImage", key);
            ensureNotNull(key);

            return CWrapper.mbp_patchinfo_has_boot_image(mCPatchInfo, key);
        }

        public void setHasBootImage(String key, boolean hasBootImage) {
            log(mCPatchInfo, PatchInfo.class, "setHasBootImage", key, hasBootImage);
            ensureNotNull(key);

            CWrapper.mbp_patchinfo_set_has_boot_image(mCPatchInfo, key, hasBootImage);
        }

        public boolean autoDetectBootImages(String key) {
            log(mCPatchInfo, PatchInfo.class, "autodetectBootImages", key);
            ensureNotNull(key);

            return CWrapper.mbp_patchinfo_autodetect_boot_images(mCPatchInfo, key);
        }

        public void setAutoDetectBootImages(String key, boolean autoDetect) {
            log(mCPatchInfo, PatchInfo.class, "setAutoDetectBootImages", key, autoDetect);
            ensureNotNull(key);

            CWrapper.mbp_patchinfo_set_autodetect_boot_images(mCPatchInfo, key, autoDetect);
        }

        public String[] getBootImages(String key) {
            log(mCPatchInfo, PatchInfo.class, "getBootImages", key);
            ensureNotNull(key);

            Pointer p = CWrapper.mbp_patchinfo_boot_images(mCPatchInfo, key);
            return getStringArrayAndFree(p);
        }

        public void setBootImages(String key, String[] bootImages) {
            log(mCPatchInfo, PatchInfo.class, "setBootImages", key, bootImages);
            ensureNotNull(key);
            ensureNotNull(bootImages);

            CWrapper.mbp_patchinfo_set_boot_images(mCPatchInfo, key, new StringArray(bootImages));
        }

        public String getRamdisk(String key) {
            log(mCPatchInfo, PatchInfo.class, "getRamdisk", key);
            ensureNotNull(key);

            Pointer p = CWrapper.mbp_patchinfo_ramdisk(mCPatchInfo, key);
            return getStringAndFree(p);
        }

        public void setRamdisk(String key, String ramdisk) {
            log(mCPatchInfo, PatchInfo.class, "setRamdisk", key, ramdisk);
            ensureNotNull(key);
            ensureNotNull(ramdisk);

            CWrapper.mbp_patchinfo_set_ramdisk(mCPatchInfo, key, ramdisk);
        }

        public boolean deviceCheck(String key) {
            log(mCPatchInfo, PatchInfo.class, "deviceCheck", key);
            ensureNotNull(key);

            return CWrapper.mbp_patchinfo_device_check(mCPatchInfo, key);
        }

        public void setDeviceCheck(String key, boolean deviceCheck) {
            log(mCPatchInfo, PatchInfo.class, "setDeviceCheck", key, deviceCheck);
            ensureNotNull(key);

            CWrapper.mbp_patchinfo_set_device_check(mCPatchInfo, key, deviceCheck);
        }
    }

    public static class StringMap implements Parcelable {
        private static HashMap<CStringMap, Integer> sInstances = new HashMap<CStringMap, Integer>();
        private CStringMap mCStringMap;

        public StringMap() {
            mCStringMap = CWrapper.mbp_stringmap_create();
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCStringMap);
            }
            log(mCStringMap, StringMap.class, "(Constructor)");
        }

        StringMap(CStringMap cStringMap) {
            ensureNotNull(cStringMap);
            mCStringMap = cStringMap;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCStringMap);
            }
            log(mCStringMap, StringMap.class, "(Constructor)");
        }

        public void destroy() {
            log(mCStringMap, StringMap.class, "destroy");
            synchronized (sInstances) {
                if (mCStringMap != null && decrementRefCount(sInstances, mCStringMap)) {
                    log(mCStringMap, StringMap.class, "(Destroyed)");
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
        public void finalize() {
            destroy();
        }

        CStringMap getPointer() {
            log(mCStringMap, StringMap.class, "getPointer");
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
            log(mCStringMap, StringMap.class, "(Constructor)");
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
            log(mCStringMap, StringMap.class, "getHashMap");
            HashMap<String, String> map = new HashMap<String, String>();
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
            log(mCStringMap, StringMap.class, "setHashMap", map);
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
            log(mCPatcher, Patcher.class, "(Constructor)");
        }

        CPatcher getPointer() {
            log(mCPatcher, Patcher.class, "getPointer");
            return mCPatcher;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof Patcher && mCPatcher.equals(((Patcher) o).mCPatcher);
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
            log(mCPatcher, Patcher.class, "(Constructor)");
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
            log(mCPatcher, Patcher.class, "getError");
            CPatcherError error = CWrapper.mbp_patcher_error(mCPatcher);
            return new PatcherError(error);
        }

        public String getId() {
            log(mCPatcher, Patcher.class, "getId");
            Pointer p = CWrapper.mbp_patcher_id(mCPatcher);
            return getStringAndFree(p);
        }

        public String getName() {
            log(mCPatcher, Patcher.class, "getName");
            Pointer p = CWrapper.mbp_patcher_name(mCPatcher);
            return getStringAndFree(p);
        }

        public boolean usesPatchInfo() {
            log(mCPatcher, Patcher.class, "usesPatchInfo");
            return CWrapper.mbp_patcher_uses_patchinfo(mCPatcher);
        }

        public void setFileInfo(FileInfo info) {
            log(mCPatcher, Patcher.class, "setFileInfo", info);
            CWrapper.mbp_patcher_set_fileinfo(mCPatcher, info.getPointer());
        }

        public String newFilePath() {
            log(mCPatcher, Patcher.class, "newFilePath");
            Pointer p = CWrapper.mbp_patcher_new_file_path(mCPatcher);
            return getStringAndFree(p);
        }

        public boolean patchFile(final ProgressListener listener) {
            log(mCPatcher, Patcher.class, "patchFile", listener);
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

            int ret = CWrapper.mbp_patcher_patch_file(mCPatcher,
                    maxProgressCb, progressCb, detailsCb, null);
            return ret == 0;
        }

        public void cancelPatching() {
            log(mCPatcher, Patcher.class, "cancelPatching");
            CWrapper.mbp_patcher_cancel_patching(mCPatcher);
        }

        public static interface ProgressListener {
            public void onMaxProgressUpdated(int max);

            public void onProgressUpdated(int value);

            public void onDetailsUpdated(String text);
        }
    }

    public static class AutoPatcher implements Parcelable {
        private CAutoPatcher mCAutoPatcher;

        AutoPatcher(CAutoPatcher cAutoPatcher) {
            ensureNotNull(cAutoPatcher);
            mCAutoPatcher = cAutoPatcher;
            log(mCAutoPatcher, AutoPatcher.class, "(Constructor)");
        }

        CAutoPatcher getPointer() {
            log(mCAutoPatcher, AutoPatcher.class, "getPointer");
            return mCAutoPatcher;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof AutoPatcher
                    && mCAutoPatcher.equals(((AutoPatcher) o).mCAutoPatcher);
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
            log(mCAutoPatcher, AutoPatcher.class, "(Constructor)");
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
            log(mCAutoPatcher, AutoPatcher.class, "getError");
            CPatcherError error = CWrapper.mbp_autopatcher_error(mCAutoPatcher);
            return new PatcherError(error);
        }

        public String getId() {
            log(mCAutoPatcher, AutoPatcher.class, "getId");
            Pointer p = CWrapper.mbp_autopatcher_id(mCAutoPatcher);
            return getStringAndFree(p);
        }

        public String[] newFiles() {
            log(mCAutoPatcher, AutoPatcher.class, "newFiles");
            Pointer p = CWrapper.mbp_autopatcher_new_files(mCAutoPatcher);
            return getStringArrayAndFree(p);
        }

        public String[] existingFiles() {
            log(mCAutoPatcher, AutoPatcher.class, "existingFiles");
            Pointer p = CWrapper.mbp_autopatcher_existing_files(mCAutoPatcher);
            return getStringArrayAndFree(p);
        }

        public boolean patchFiles(String directory) {
            log(mCAutoPatcher, AutoPatcher.class, "patchFiles", directory);
            ensureNotNull(directory);

            int ret = CWrapper.mbp_autopatcher_patch_files(
                    mCAutoPatcher, directory);
            return ret == 0;
        }
    }

    public static class RamdiskPatcher implements Parcelable {
        private CRamdiskPatcher mCRamdiskPatcher;

        RamdiskPatcher(CRamdiskPatcher cRamdiskPatcher) {
            ensureNotNull(cRamdiskPatcher);
            mCRamdiskPatcher = cRamdiskPatcher;
            log(mCRamdiskPatcher, RamdiskPatcher.class, "(Constructor)");
        }

        CRamdiskPatcher getPointer() {
            log(mCRamdiskPatcher, RamdiskPatcher.class, "getPointer");
            return mCRamdiskPatcher;
        }

        @Override
        public boolean equals(Object o) {
            return o instanceof RamdiskPatcher
                    && mCRamdiskPatcher.equals(((RamdiskPatcher) o).mCRamdiskPatcher);
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
            log(mCRamdiskPatcher, RamdiskPatcher.class, "(Constructor)");
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
            log(mCRamdiskPatcher, RamdiskPatcher.class, "getError");
            CPatcherError error = CWrapper.mbp_ramdiskpatcher_error(mCRamdiskPatcher);
            return new PatcherError(error);
        }

        public String getId() {
            log(mCRamdiskPatcher, RamdiskPatcher.class, "getId");
            Pointer p = CWrapper.mbp_ramdiskpatcher_id(mCRamdiskPatcher);
            return getStringAndFree(p);
        }

        public boolean patchRamdisk() {
            log(mCRamdiskPatcher, RamdiskPatcher.class, "patchRamdisk");
            int ret = CWrapper.mbp_ramdiskpatcher_patch_ramdisk(mCRamdiskPatcher);
            return ret == 0;
        }
    }
}
