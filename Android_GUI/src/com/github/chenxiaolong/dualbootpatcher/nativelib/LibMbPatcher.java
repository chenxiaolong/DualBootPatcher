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

import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.CWrapper.CDevice;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.CWrapper.CAutoPatcher;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.CWrapper.CFileInfo;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.CWrapper.CPatcher;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.CWrapper.CPatcherConfig;
import com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff.LibMiscStuff;
import com.sun.jna.Callback;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

import java.util.Arrays;
import java.util.HashMap;

// NOTE: Almost no checking of parameters is performed on both the Java and C side of this native
//       wrapper. As a rule of thumb, don't pass null to any function.

@SuppressWarnings("unused")
public class LibMbPatcher {
    /** Log (almost) all native method calls and their parameters */
    private static final boolean DEBUG = false;

    @SuppressWarnings("JniMissingFunction")
    static class CWrapper {
        static {
            Native.register(CWrapper.class, "mbpatcher");
            LibMiscStuff.mblogSetLogcat();
        }

        // BEGIN: ctypes.h
        public static class CFileInfo extends PointerType {}
        public static class CPatcherConfig extends PointerType {}

        public static class CPatcher extends PointerType {}
        public static class CAutoPatcher extends PointerType {}
        // END: ctypes.h

        // BEGIN: cfileinfo.h
        static native CFileInfo mbpatcher_fileinfo_create();
        static native void mbpatcher_fileinfo_destroy(CFileInfo info);
        static native Pointer mbpatcher_fileinfo_input_path(CFileInfo info);
        static native void mbpatcher_fileinfo_set_input_path(CFileInfo info, String path);
        static native Pointer mbpatcher_fileinfo_output_path(CFileInfo info);
        static native void mbpatcher_fileinfo_set_output_path(CFileInfo info, String path);
        static native CDevice mbpatcher_fileinfo_device(CFileInfo info);
        static native void mbpatcher_fileinfo_set_device(CFileInfo info, CDevice device);
        static native Pointer mbpatcher_fileinfo_rom_id(CFileInfo info);
        static native void mbpatcher_fileinfo_set_rom_id(CFileInfo info, String id);
        // END: cfileinfo.h

        // BEGIN: cpatcherconfig.h
        static native CPatcherConfig mbpatcher_config_create();
        static native void mbpatcher_config_destroy(CPatcherConfig pc);
        static native /* ErrorCode */ int mbpatcher_config_error(CPatcherConfig pc);
        static native Pointer mbpatcher_config_data_directory(CPatcherConfig pc);
        static native Pointer mbpatcher_config_temp_directory(CPatcherConfig pc);
        static native void mbpatcher_config_set_data_directory(CPatcherConfig pc, String path);
        static native void mbpatcher_config_set_temp_directory(CPatcherConfig pc, String path);
        static native Pointer mbpatcher_config_patchers(CPatcherConfig pc);
        static native Pointer mbpatcher_config_autopatchers(CPatcherConfig pc);
        static native CPatcher mbpatcher_config_create_patcher(CPatcherConfig pc, String id);
        static native CAutoPatcher mbpatcher_config_create_autopatcher(CPatcherConfig pc, String id, CFileInfo info);
        static native void mbpatcher_config_destroy_patcher(CPatcherConfig pc, CPatcher patcher);
        static native void mbpatcher_config_destroy_autopatcher(CPatcherConfig pc, CAutoPatcher patcher);
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

        static native /* ErrorCode */ int mbpatcher_patcher_error(CPatcher patcher);
        static native Pointer mbpatcher_patcher_id(CPatcher patcher);
        static native void mbpatcher_patcher_set_fileinfo(CPatcher patcher, CFileInfo info);
        static native boolean mbpatcher_patcher_patch_file(CPatcher patcher, ProgressUpdatedCallback progressCb, FilesUpdatedCallback filesCb, DetailsUpdatedCallback detailsCb, Pointer userData);
        static native void mbpatcher_patcher_cancel_patching(CPatcher patcher);

        static native /* ErrorCode */ int mbpatcher_autopatcher_error(CAutoPatcher patcher);
        static native Pointer mbpatcher_autopatcher_id(CAutoPatcher patcher);
        static native Pointer mbpatcher_autopatcher_new_files(CAutoPatcher patcher);
        static native Pointer mbpatcher_autopatcher_existing_files(CAutoPatcher patcher);
        static native boolean mbpatcher_autopatcher_patch_files(CAutoPatcher patcher, String directory);
        // END: cpatcherinterface.h
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
            Log.d("libmbpatcher", signature);
        }

        if (ptr == null && !method.equals("destroy")) {
            throw new IllegalStateException("Called on a destroyed object! " + signature);
        }
    }

    public static class FileInfo implements Parcelable {
        private static final HashMap<CFileInfo, Integer> sInstances = new HashMap<>();
        private CFileInfo mCFileInfo;

        public FileInfo() {
            mCFileInfo = CWrapper.mbpatcher_fileinfo_create();
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
                    CWrapper.mbpatcher_fileinfo_destroy(mCFileInfo);
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
            Pointer p = CWrapper.mbpatcher_fileinfo_input_path(mCFileInfo);
            return LibC.getStringAndFree(p);
        }

        public void setInputPath(String path) {
            validate(mCFileInfo, FileInfo.class, "setInputPath", path);
            ensureNotNull(path);

            CWrapper.mbpatcher_fileinfo_set_input_path(mCFileInfo, path);
        }

        public String getOutputPath() {
            validate(mCFileInfo, FileInfo.class, "getOutputPath");
            Pointer p = CWrapper.mbpatcher_fileinfo_output_path(mCFileInfo);
            return LibC.getStringAndFree(p);
        }

        public void setOutputPath(String path) {
            validate(mCFileInfo, FileInfo.class, "setOutputPath", path);
            ensureNotNull(path);

            CWrapper.mbpatcher_fileinfo_set_output_path(mCFileInfo, path);
        }

        public Device getDevice() {
            validate(mCFileInfo, FileInfo.class, "getDevice");
            CDevice cDevice = CWrapper.mbpatcher_fileinfo_device(mCFileInfo);
            return cDevice == null ? null : new Device(cDevice, false);
        }

        public void setDevice(Device device) {
            validate(mCFileInfo, FileInfo.class, "setDevice", device);
            ensureNotNull(device);

            CWrapper.mbpatcher_fileinfo_set_device(mCFileInfo, device.getPointer());
        }

        public String getRomId() {
            validate(mCFileInfo, FileInfo.class, "getRomId");
            Pointer p = CWrapper.mbpatcher_fileinfo_rom_id(mCFileInfo);
            return LibC.getStringAndFree(p);
        }

        public void setRomId(String id) {
            validate(mCFileInfo, FileInfo.class, "setRomId", id);
            ensureNotNull(id);

            CWrapper.mbpatcher_fileinfo_set_rom_id(mCFileInfo, id);
        }
    }

    public static class PatcherConfig implements Parcelable {
        private static final HashMap<CPatcherConfig, Integer> sInstances = new HashMap<>();
        private CPatcherConfig mCPatcherConfig;

        public PatcherConfig() {
            mCPatcherConfig = CWrapper.mbpatcher_config_create();
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
                    CWrapper.mbpatcher_config_destroy(mCPatcherConfig);
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
            return CWrapper.mbpatcher_config_error(mCPatcherConfig);
        }

        public String getDataDirectory() {
            validate(mCPatcherConfig, PatcherConfig.class, "getDataDirectory");
            Pointer p = CWrapper.mbpatcher_config_data_directory(mCPatcherConfig);
            return LibC.getStringAndFree(p);
        }

        public String getTempDirectory() {
            validate(mCPatcherConfig, PatcherConfig.class, "getTempDirectory");
            Pointer p = CWrapper.mbpatcher_config_temp_directory(mCPatcherConfig);
            return LibC.getStringAndFree(p);
        }

        public void setDataDirectory(String path) {
            validate(mCPatcherConfig, PatcherConfig.class, "setDataDirectory", path);
            ensureNotNull(path);

            CWrapper.mbpatcher_config_set_data_directory(mCPatcherConfig, path);
        }

        public void setTempDirectory(String path) {
            validate(mCPatcherConfig, PatcherConfig.class, "setTempDirectory", path);
            ensureNotNull(path);

            CWrapper.mbpatcher_config_set_temp_directory(mCPatcherConfig, path);
        }

        public String[] getPatchers() {
            validate(mCPatcherConfig, PatcherConfig.class, "getPatchers");
            Pointer p = CWrapper.mbpatcher_config_patchers(mCPatcherConfig);
            return LibC.getStringArrayAndFree(p);
        }

        public String[] getAutoPatchers() {
            validate(mCPatcherConfig, PatcherConfig.class, "getAutoPatchers");
            Pointer p = CWrapper.mbpatcher_config_autopatchers(mCPatcherConfig);
            return LibC.getStringArrayAndFree(p);
        }

        public Patcher createPatcher(String id) {
            validate(mCPatcherConfig, PatcherConfig.class, "createPatcher", id);
            ensureNotNull(id);

            CPatcher patcher = CWrapper.mbpatcher_config_create_patcher(mCPatcherConfig, id);
            return patcher == null ? null : new Patcher(patcher);
        }

        public AutoPatcher createAutoPatcher(String id, FileInfo info) {
            validate(mCPatcherConfig, PatcherConfig.class, "createAutoPatcher", id, info);
            ensureNotNull(id);
            ensureNotNull(info);

            CAutoPatcher patcher = CWrapper.mbpatcher_config_create_autopatcher(
                    mCPatcherConfig, id, info.getPointer());
            return patcher == null ? null : new AutoPatcher(patcher);
        }

        public void destroyPatcher(Patcher patcher) {
            validate(mCPatcherConfig, PatcherConfig.class, "destroyPatcher", patcher);
            ensureNotNull(patcher);

            CWrapper.mbpatcher_config_destroy_patcher(mCPatcherConfig, patcher.getPointer());
        }

        public void destroyAutoPatcher(AutoPatcher patcher) {
            validate(mCPatcherConfig, PatcherConfig.class, "destroyAutoPatcher", patcher);
            ensureNotNull(patcher);

            CWrapper.mbpatcher_config_destroy_autopatcher(mCPatcherConfig, patcher.getPointer());
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
            return CWrapper.mbpatcher_patcher_error(mCPatcher);
        }

        public String getId() {
            validate(mCPatcher, Patcher.class, "getId");
            Pointer p = CWrapper.mbpatcher_patcher_id(mCPatcher);
            return LibC.getStringAndFree(p);
        }

        public void setFileInfo(FileInfo info) {
            validate(mCPatcher, Patcher.class, "setFileInfo", info);
            CWrapper.mbpatcher_patcher_set_fileinfo(mCPatcher, info.getPointer());
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

            return CWrapper.mbpatcher_patcher_patch_file(mCPatcher,
                    progressCb, filesCb, detailsCb, null);
        }

        public void cancelPatching() {
            validate(mCPatcher, Patcher.class, "cancelPatching");
            CWrapper.mbpatcher_patcher_cancel_patching(mCPatcher);
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
            return CWrapper.mbpatcher_autopatcher_error(mCAutoPatcher);
        }

        public String getId() {
            validate(mCAutoPatcher, AutoPatcher.class, "getId");
            Pointer p = CWrapper.mbpatcher_autopatcher_id(mCAutoPatcher);
            return LibC.getStringAndFree(p);
        }

        public String[] newFiles() {
            validate(mCAutoPatcher, AutoPatcher.class, "newFiles");
            Pointer p = CWrapper.mbpatcher_autopatcher_new_files(mCAutoPatcher);
            return LibC.getStringArrayAndFree(p);
        }

        public String[] existingFiles() {
            validate(mCAutoPatcher, AutoPatcher.class, "existingFiles");
            Pointer p = CWrapper.mbpatcher_autopatcher_existing_files(mCAutoPatcher);
            return LibC.getStringArrayAndFree(p);
        }

        public boolean patchFiles(String directory) {
            validate(mCAutoPatcher, AutoPatcher.class, "patchFiles", directory);
            ensureNotNull(directory);

            return CWrapper.mbpatcher_autopatcher_patch_files(mCAutoPatcher, directory);
        }
    }
}
