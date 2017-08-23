/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.support.annotation.NonNull;

import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.CWrapper.CDevice;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.CWrapper.CJsonError;
import com.sun.jna.IntegerType;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.StringArray;

import java.util.HashMap;

// NOTE: Almost no checking of parameters is performed on both the Java and C side of this native
//       wrapper. As a rule of thumb, don't pass null to any function.

@SuppressWarnings("unused")
public class LibMbDevice {
    @SuppressWarnings("JniMissingFunction")
    static class CWrapper {
        static {
            Native.register(CWrapper.class, "mbdevice");
        }

        public static class CDevice extends PointerType {
            public CDevice() {
            }

            public CDevice(Pointer p) {
                super(p);
            }
        }

        public static class CJsonError extends PointerType {}

        public static class SizeT extends IntegerType {
            public SizeT() {
                this(0);
            }

            public SizeT(long value) {
                super(Native.SIZE_T_SIZE, value, true);
            }
        }

        // BEGIN: device.h
        static native CDevice mb_device_new();

        static native void mb_device_free(CDevice device);

        static native /* char * */ Pointer mb_device_id(CDevice device);
        static native void mb_device_set_id(CDevice device, String id);

        static native /* char ** */ Pointer mb_device_codenames(CDevice device);
        static native void mb_device_set_codenames(CDevice device, StringArray codenames);

        static native /* char * */ Pointer mb_device_name(CDevice device);
        static native void mb_device_set_name(CDevice device, String name);

        static native /* char * */ Pointer mb_device_architecture(CDevice device);
        static native void mb_device_set_architecture(CDevice device, String architecture);

        static native /* uint32_t */ int mb_device_flags(CDevice device);
        static native void mb_device_set_flags(CDevice device, int flags);

        static native /* char ** */ Pointer mb_device_block_dev_base_dirs(CDevice device);
        static native void mb_device_set_block_dev_base_dirs(CDevice device, StringArray baseDirs);

        static native /* char ** */ Pointer mb_device_system_block_devs(CDevice device);
        static native void mb_device_set_system_block_devs(CDevice device, StringArray systemDevs);

        static native /* char ** */ Pointer mb_device_cache_block_devs(CDevice device);
        static native void mb_device_set_cache_block_devs(CDevice device, StringArray cacheDevs);

        static native /* char ** */ Pointer mb_device_data_block_devs(CDevice device);
        static native void mb_device_set_data_block_devs(CDevice device, StringArray dataDevs);

        static native /* char ** */ Pointer mb_device_boot_block_devs(CDevice device);
        static native void mb_device_set_boot_block_devs(CDevice device, StringArray bootDevs);

        static native /* char ** */ Pointer mb_device_recovery_block_devs(CDevice device);
        static native void mb_device_set_recovery_block_devs(CDevice device, StringArray recoveryDevs);

        static native /* char ** */ Pointer mb_device_extra_block_devs(CDevice device);
        static native void mb_device_set_extra_block_devs(CDevice device, StringArray extraDevs);

        /* Boot UI */

        static native boolean mb_device_tw_supported(CDevice device);
        static native void mb_device_set_tw_supported(CDevice device, boolean supported);

        static native /* uint32_t */ int mb_device_tw_flags(CDevice device);
        static native void mb_device_set_tw_flags(CDevice device, int flags);

        static native /* uint32_t */ int mb_device_tw_pixel_format(CDevice device);
        static native void mb_device_set_tw_pixel_format(CDevice device, /* uint32_t */ int format);

        static native /* uint32_t */ int mb_device_tw_force_pixel_format(CDevice device);
        static native void mb_device_set_tw_force_pixel_format(CDevice device, /* uint32_t */ int format);

        static native int mb_device_tw_overscan_percent(CDevice device);
        static native void mb_device_set_tw_overscan_percent(CDevice device, int percent);

        static native int mb_device_tw_default_x_offset(CDevice device);
        static native void mb_device_set_tw_default_x_offset(CDevice device, int offset);

        static native int mb_device_tw_default_y_offset(CDevice device);
        static native void mb_device_set_tw_default_y_offset(CDevice device, int offset);

        static native /* char * */ Pointer mb_device_tw_brightness_path(CDevice device);
        static native void mb_device_set_tw_brightness_path(CDevice device, String path);

        static native /* char * */ Pointer mb_device_tw_secondary_brightness_path(CDevice device);
        static native void mb_device_set_tw_secondary_brightness_path(CDevice device, String path);

        static native int mb_device_tw_max_brightness(CDevice device);
        static native void mb_device_set_tw_max_brightness(CDevice device, int brightness);

        static native int mb_device_tw_default_brightness(CDevice device);
        static native void mb_device_set_tw_default_brightness(CDevice device, int brightness);

        static native /* char * */ Pointer mb_device_tw_battery_path(CDevice device);
        static native void mb_device_set_tw_battery_path(CDevice device, String path);

        static native /* char * */ Pointer mb_device_tw_cpu_temp_path(CDevice device);
        static native void mb_device_set_tw_cpu_temp_path(CDevice device, String path);

        static native /* char * */ Pointer mb_device_tw_input_blacklist(CDevice device);
        static native void mb_device_set_tw_input_blacklist(CDevice device, String blacklist);

        static native /* char * */ Pointer mb_device_tw_input_whitelist(CDevice device);
        static native void mb_device_set_tw_input_whitelist(CDevice device, String whitelist);

        static native /* char ** */ Pointer mb_device_tw_graphics_backends(CDevice device);
        static native void mb_device_set_tw_graphics_backends(CDevice device, StringArray backends);

        static native /* char * */ Pointer mb_device_tw_theme(CDevice device);
        static native void mb_device_set_tw_theme(CDevice device, String theme);

        /* Other */

        static native /* uint32_t */ int mb_device_validate(CDevice device);

        static native boolean mb_device_equals(CDevice a, CDevice b);
        // END: device.h

        // BEGIN: json.h
        static native CJsonError mb_device_json_error_new();

        static native void mb_device_json_error_free(CJsonError error);

        static native /* uint16_t */ short mb_device_json_error_type(CJsonError error);
        static native /* size_t */ SizeT mb_device_json_error_offset(CJsonError error);
        static native /* char * */ Pointer mb_device_json_error_message(CJsonError error);
        static native /* char * */ Pointer mb_device_json_error_schema_uri(CJsonError error);
        static native /* char * */ Pointer mb_device_json_error_schema_keyword(CJsonError error);
        static native /* char * */ Pointer mb_device_json_error_document_uri(CJsonError error);

        static native CDevice mb_device_new_from_json(String json, CJsonError error);

        static native /* CDevice ** */ Pointer mb_device_new_list_from_json(String json, CJsonError error);

        static native /* char * */ Pointer mb_device_to_json(CDevice device);
        // END: json.h
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

    public static class JsonError {
        private CJsonError mCJsonError;

        public JsonError() {
            mCJsonError = CWrapper.mb_device_json_error_new();
            if (mCJsonError == null) {
                throw new IllegalStateException("Failed to allocate CJsonError object");
            }
        }

        public void destroy() {
            if (mCJsonError != null) {
                CWrapper.mb_device_json_error_free(mCJsonError);
            }
            mCJsonError = null;
        }

        @Override
        protected void finalize() throws Throwable {
            destroy();
            super.finalize();
        }

        CJsonError getPointer() {
            return mCJsonError;
        }

        public short type() {
            return CWrapper.mb_device_json_error_type(mCJsonError);
        }

        public long offset() {
            return CWrapper.mb_device_json_error_offset(mCJsonError).longValue();
        }

        public String message() {
            Pointer p = CWrapper.mb_device_json_error_message(mCJsonError);
            if (p == null) {
                return null;
            }
            return LibC.getStringAndFree(p);
        }

        public String schemaUri() {
            Pointer p = CWrapper.mb_device_json_error_schema_uri(mCJsonError);
            if (p == null) {
                return null;
            }
            return LibC.getStringAndFree(p);
        }

        public String schemaKeyword() {
            Pointer p = CWrapper.mb_device_json_error_schema_keyword(mCJsonError);
            if (p == null) {
                return null;
            }
            return LibC.getStringAndFree(p);
        }

        public String documentUri() {
            Pointer p = CWrapper.mb_device_json_error_document_uri(mCJsonError);
            if (p == null) {
                return null;
            }
            return LibC.getStringAndFree(p);
        }
    }

    public static class Device implements Parcelable {
        private static final HashMap<CDevice, Integer> sInstances = new HashMap<>();
        private CDevice mCDevice;
        private boolean mDestroyable;

        public Device() {
            mCDevice = CWrapper.mb_device_new();
            if (mCDevice == null) {
                throw new IllegalStateException("Failed to allocate device object");
            }

            synchronized (sInstances) {
                incrementRefCount(sInstances, mCDevice);
            }
            mDestroyable = true;
        }

        Device(CDevice cDevice, boolean destroyable) {
            if (cDevice == null) {
                throw new IllegalArgumentException("Cannot wrap null device object pointer");
            }

            mCDevice = cDevice;
            synchronized (sInstances) {
                incrementRefCount(sInstances, mCDevice);
            }
            mDestroyable = destroyable;
        }

        public void destroy() {
            synchronized (sInstances) {
                if (mCDevice != null && decrementRefCount(sInstances, mCDevice) && mDestroyable) {
                    CWrapper.mb_device_free(mCDevice);
                }
                mCDevice = null;
            }
        }

        @Override
        public boolean equals(Object o) {
            if (!(o instanceof Device)) {
                return false;
            }

            CDevice other = ((Device) o).mCDevice;
            return mCDevice.equals(other) || CWrapper.mb_device_equals(mCDevice, other);

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

        @NonNull
        CDevice getPointer() {
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
            if (mCDevice == null) {
                throw new IllegalStateException("Deserialized device object is null");
            }
        }

        public static final Creator<Device> CREATOR
                = new Creator<Device>() {
            public Device createFromParcel(Parcel in) {
                return new Device(in);
            }

            public Device[] newArray(int size) {
                return new Device[size];
            }
        };

        public static Device newFromJson(String json) {
            JsonError error = new JsonError();
            Device device = newFromJson(json, error);
            error.destroy();
            return device;
        }

        public static Device newFromJson(String json, JsonError error) {
            return new Device(CWrapper.mb_device_new_from_json(json, error.getPointer()), true);
        }

        public static Device[] newListFromJson(String json) {
            JsonError error = new JsonError();
            Device[] devices = newListFromJson(json, error);
            error.destroy();
            return devices;
        }

        public static Device[] newListFromJson(String json, JsonError error) {
            Pointer p = CWrapper.mb_device_new_list_from_json(json, error.getPointer());
            if (p == null) {
                return null;
            }
            Pointer[] cDevices = p.getPointerArray(0);

            Device[] devices = new Device[cDevices.length];
            for (int i = 0; i < cDevices.length; i++) {
                devices[i] = new Device(new CDevice(cDevices[i]), true);
            }

            LibC.free(p);
            return devices;
        }

        public String getId() {
            Pointer p = CWrapper.mb_device_id(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setId(String id) {
            CWrapper.mb_device_set_id(mCDevice, id);
        }

        public String[] getCodenames() {
            Pointer p = CWrapper.mb_device_codenames(mCDevice);
            return p == null ? null : LibC.getStringArrayAndFree(p);
        }

        public void setCodenames(String[] names) {
            CWrapper.mb_device_set_codenames(mCDevice, new StringArray(names));
        }

        public String getName() {
            Pointer p = CWrapper.mb_device_name(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setName(String name) {
            CWrapper.mb_device_set_name(mCDevice, name);
        }

        public String getArchitecture() {
            Pointer p = CWrapper.mb_device_architecture(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setArchitecture(String arch) {
            CWrapper.mb_device_set_architecture(mCDevice, arch);
        }

        public int getFlags() {
            return CWrapper.mb_device_flags(mCDevice);
        }

        public void setFlags(int flags) {
            CWrapper.mb_device_set_flags(mCDevice, flags);
        }

        public String[] getBlockDevBaseDirs() {
            Pointer p = CWrapper.mb_device_block_dev_base_dirs(mCDevice);
            return p == null ? null : LibC.getStringArrayAndFree(p);
        }

        public void setBlockDevBaseDirs(String[] dirs) {
            CWrapper.mb_device_set_block_dev_base_dirs(mCDevice, new StringArray(dirs));
        }

        public String[] getSystemBlockDevs() {
            Pointer p = CWrapper.mb_device_system_block_devs(mCDevice);
            return p == null ? null : LibC.getStringArrayAndFree(p);
        }

        public void setSystemBlockDevs(String[] blockDevs) {
            CWrapper.mb_device_set_system_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getCacheBlockDevs() {
            Pointer p = CWrapper.mb_device_cache_block_devs(mCDevice);
            return p == null ? null : LibC.getStringArrayAndFree(p);
        }

        public void setCacheBlockDevs(String[] blockDevs) {
            CWrapper.mb_device_set_cache_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getDataBlockDevs() {
            Pointer p = CWrapper.mb_device_data_block_devs(mCDevice);
            return p == null ? null : LibC.getStringArrayAndFree(p);
        }

        public void setDataBlockDevs(String[] blockDevs) {
            CWrapper.mb_device_set_data_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getBootBlockDevs() {
            Pointer p = CWrapper.mb_device_boot_block_devs(mCDevice);
            return p == null ? null : LibC.getStringArrayAndFree(p);
        }

        public void setBootBlockDevs(String[] blockDevs) {
            CWrapper.mb_device_set_boot_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getRecoveryBlockDevs() {
            Pointer p = CWrapper.mb_device_recovery_block_devs(mCDevice);
            return p == null ? null : LibC.getStringArrayAndFree(p);
        }

        public void setRecoveryBlockDevs(String[] blockDevs) {
            CWrapper.mb_device_set_recovery_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public String[] getExtraBlockDevs() {
            Pointer p = CWrapper.mb_device_extra_block_devs(mCDevice);
            return p == null ? null : LibC.getStringArrayAndFree(p);
        }

        public void setExtraBlockDevs(String[] blockDevs) {
            CWrapper.mb_device_set_extra_block_devs(mCDevice, new StringArray(blockDevs));
        }

        public boolean isTwSupported() {
            return CWrapper.mb_device_tw_supported(mCDevice);
        }

        public void setTwSupported(boolean supported) {
            CWrapper.mb_device_set_tw_supported(mCDevice, supported);
        }

        public int getTwFlags() {
            return CWrapper.mb_device_tw_flags(mCDevice);
        }

        public void setTwFlags(int flags) {
            CWrapper.mb_device_set_tw_flags(mCDevice, flags);
        }

        public int getTwPixelFormat() {
            return CWrapper.mb_device_tw_pixel_format(mCDevice);
        }

        public void setTwPixelFormat(int format) {
            CWrapper.mb_device_set_tw_pixel_format(mCDevice, format);
        }

        public int getTwForcePixelFormat() {
            return CWrapper.mb_device_tw_force_pixel_format(mCDevice);
        }

        public void setTwForcePixelFormat(int format) {
            CWrapper.mb_device_set_tw_force_pixel_format(mCDevice, format);
        }

        public int getTwOverscanPercent() {
            return CWrapper.mb_device_tw_overscan_percent(mCDevice);
        }

        public void setTwOverscanPercent(int percent) {
            CWrapper.mb_device_set_tw_overscan_percent(mCDevice, percent);
        }

        public int getTwDefaultXOffset() {
            return CWrapper.mb_device_tw_default_x_offset(mCDevice);
        }

        public void setTwDefaultXOffset(int offset) {
            CWrapper.mb_device_set_tw_default_x_offset(mCDevice, offset);
        }

        public int getTwDefaultYOffset() {
            return CWrapper.mb_device_tw_default_y_offset(mCDevice);
        }

        public void setTwDefaultYOffset(int offset) {
            CWrapper.mb_device_set_tw_default_y_offset(mCDevice, offset);
        }

        public String getTwBrightnessPath() {
            Pointer p = CWrapper.mb_device_tw_brightness_path(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setTwBrightnessPath(String path) {
            CWrapper.mb_device_set_tw_brightness_path(mCDevice, path);
        }

        public String getTwSecondaryBrightnessPath() {
            Pointer p = CWrapper.mb_device_tw_secondary_brightness_path(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setTwSecondaryBrightnessPath(String path) {
            CWrapper.mb_device_set_tw_secondary_brightness_path(mCDevice, path);
        }

        public int getTwMaxBrightness() {
            return CWrapper.mb_device_tw_max_brightness(mCDevice);
        }

        public void setTwMaxBrightness(int brightness) {
            CWrapper.mb_device_set_tw_max_brightness(mCDevice, brightness);
        }

        public int getTwDefaultBrightness() {
            return CWrapper.mb_device_tw_default_brightness(mCDevice);
        }

        public void setTwDefaultBrightness(int brightness) {
            CWrapper.mb_device_set_tw_default_brightness(mCDevice, brightness);
        }

        public String getTwBatteryPath() {
            Pointer p = CWrapper.mb_device_tw_battery_path(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setTwBatteryPath(String path) {
            CWrapper.mb_device_set_tw_battery_path(mCDevice, path);
        }

        public String getTwCpuTempPath() {
            Pointer p = CWrapper.mb_device_tw_cpu_temp_path(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setTwCpuTempPath(String path) {
            CWrapper.mb_device_set_tw_cpu_temp_path(mCDevice, path);
        }

        public String getTwInputBlacklist() {
            Pointer p = CWrapper.mb_device_tw_input_blacklist(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setTwInputBlacklist(String blacklist) {
            CWrapper.mb_device_set_tw_input_blacklist(mCDevice, blacklist);
        }

        public String getTwInputWhitelist() {
            Pointer p = CWrapper.mb_device_tw_input_whitelist(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setTwInputWhitelist(String whitelist) {
            CWrapper.mb_device_set_tw_input_whitelist(mCDevice, whitelist);
        }

        public String[] getTwGraphicsBackends() {
            Pointer p = CWrapper.mb_device_tw_graphics_backends(mCDevice);
            return p == null ? null : LibC.getStringArrayAndFree(p);
        }

        public void setTwGraphicsBackends(String[] backends) {
            CWrapper.mb_device_set_tw_graphics_backends(mCDevice, new StringArray(backends));
        }

        public String getTwTheme() {
            Pointer p = CWrapper.mb_device_tw_theme(mCDevice);
            return p == null ? null : LibC.getStringAndFree(p);
        }

        public void setTwTheme(String theme) {
            CWrapper.mb_device_set_tw_theme(mCDevice, theme);
        }

        public long validate() {
            return CWrapper.mb_device_validate(mCDevice);
        }
    }
}
