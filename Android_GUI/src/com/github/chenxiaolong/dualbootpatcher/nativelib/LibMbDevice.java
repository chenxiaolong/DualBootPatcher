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
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.StringArray;
import com.sun.jna.ptr.PointerByReference;

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

        // BEGIN: device.h
        static native CDevice mb_device_new();

        static native void mb_device_free(CDevice device);

        static native String mb_device_id(CDevice device);
        static native int mb_device_set_id(CDevice device, String id);

        static native Pointer mb_device_codenames(CDevice device);
        static native int mb_device_set_codenames(CDevice device, StringArray codenames);

        static native String mb_device_name(CDevice device);
        static native int mb_device_set_name(CDevice device, String name);

        static native String mb_device_architecture(CDevice device);
        static native int mb_device_set_architecture(CDevice device, String architecture);

        static native long mb_device_flags(CDevice device);
        static native int mb_device_set_flags(CDevice device, long flags);

        static native Pointer mb_device_block_dev_base_dirs(CDevice device);
        static native int mb_device_set_block_dev_base_dirs(CDevice device, StringArray baseDirs);

        static native Pointer mb_device_system_block_devs(CDevice device);
        static native int mb_device_set_system_block_devs(CDevice device, StringArray systemDevs);

        static native Pointer mb_device_cache_block_devs(CDevice device);
        static native int mb_device_set_cache_block_devs(CDevice device, StringArray cacheDevs);

        static native Pointer mb_device_data_block_devs(CDevice device);
        static native int mb_device_set_data_block_devs(CDevice device, StringArray dataDevs);

        static native Pointer mb_device_boot_block_devs(CDevice device);
        static native int mb_device_set_boot_block_devs(CDevice device, StringArray bootDevs);

        static native Pointer mb_device_recovery_block_devs(CDevice device);
        static native int mb_device_set_recovery_block_devs(CDevice device, StringArray recoveryDevs);

        static native Pointer mb_device_extra_block_devs(CDevice device);
        static native int mb_device_set_extra_block_devs(CDevice device, StringArray extraDevs);

        /* Boot UI */

        static native boolean mb_device_tw_supported(CDevice device);
        static native int mb_device_set_tw_supported(CDevice device, boolean supported);

        static native long mb_device_tw_flags(CDevice device);
        static native int mb_device_set_tw_flags(CDevice device, long flags);

        static native /* enum TwPixelFormat */ int mb_device_tw_pixel_format(CDevice device);
        static native int mb_device_set_tw_pixel_format(CDevice device, /* enum TwPixelFormat */ int format);

        static native /* enum TwForcePixelFormat */ int mb_device_tw_force_pixel_format(CDevice device);
        static native int mb_device_set_tw_force_pixel_format(CDevice device, /* enum TwForcePixelFormat */ int format);

        static native int mb_device_tw_overscan_percent(CDevice device);
        static native int mb_device_set_tw_overscan_percent(CDevice device, int percent);

        static native int mb_device_tw_default_x_offset(CDevice device);
        static native int mb_device_set_tw_default_x_offset(CDevice device, int offset);

        static native int mb_device_tw_default_y_offset(CDevice device);
        static native int mb_device_set_tw_default_y_offset(CDevice device, int offset);

        static native String mb_device_tw_brightness_path(CDevice device);
        static native int mb_device_set_tw_brightness_path(CDevice device, String path);

        static native String mb_device_tw_secondary_brightness_path(CDevice device);
        static native int mb_device_set_tw_secondary_brightness_path(CDevice device, String path);

        static native int mb_device_tw_max_brightness(CDevice device);
        static native int mb_device_set_tw_max_brightness(CDevice device, int brightness);

        static native int mb_device_tw_default_brightness(CDevice device);
        static native int mb_device_set_tw_default_brightness(CDevice device, int brightness);

        static native String mb_device_tw_battery_path(CDevice device);
        static native int mb_device_set_tw_battery_path(CDevice device, String path);

        static native String mb_device_tw_cpu_temp_path(CDevice device);
        static native int mb_device_set_tw_cpu_temp_path(CDevice device, String path);

        static native String mb_device_tw_input_blacklist(CDevice device);
        static native int mb_device_set_tw_input_blacklist(CDevice device, String blacklist);

        static native String mb_device_tw_input_whitelist(CDevice device);
        static native int mb_device_set_tw_input_whitelist(CDevice device, String whitelist);

        static native Pointer mb_device_tw_graphics_backends(CDevice device);
        static native int mb_device_set_tw_graphics_backends(CDevice device, StringArray backends);

        static native String mb_device_tw_theme(CDevice device);
        static native int mb_device_set_tw_theme(CDevice device, String theme);

        /* Crypto */

        static native boolean mb_device_crypto_supported(CDevice device);
        static native int mb_device_set_crypto_supported(CDevice device, boolean supported);

        static native String mb_device_crypto_header_path(CDevice device);
        static native int mb_device_set_crypto_header_path(CDevice device, String path);

        /* Other */

        static native boolean mb_device_equals(CDevice a, CDevice b);
        // END: device.h

        // BEGIN: json.h
        static native CDevice mb_device_new_from_json(String json, PointerByReference error);

        static native /* CDevice[] */ Pointer mb_device_new_list_from_json(String json, PointerByReference error);
        // END: json.h

        // BEGIN: validate.h
        static native long mb_device_validate(CDevice device);
        // END: validate.h
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

    private static void handleReturn(int ret) {
        if (ret != 0) {
            throw new IllegalStateException("libmbdevice returned: " + ret);
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
            PointerByReference error = new PointerByReference();
            return new Device(CWrapper.mb_device_new_from_json(json, error), true);
        }

        public static Device[] newListFromJson(String json) {
            PointerByReference error = new PointerByReference();
            Pointer p = CWrapper.mb_device_new_list_from_json(json, error);
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
            return CWrapper.mb_device_id(mCDevice);
        }

        public void setId(String id) {
            handleReturn(CWrapper.mb_device_set_id(mCDevice, id));
        }

        public String[] getCodenames() {
            Pointer ptr = CWrapper.mb_device_codenames(mCDevice);
            return ptr == null ? null : ptr.getStringArray(0);
        }

        public void setCodenames(String[] names) {
            handleReturn(CWrapper.mb_device_set_codenames(mCDevice, new StringArray(names)));
        }

        public String getName() {
            return CWrapper.mb_device_name(mCDevice);
        }

        public void setName(String name) {
            handleReturn(CWrapper.mb_device_set_name(mCDevice, name));
        }

        public String getArchitecture() {
            return CWrapper.mb_device_architecture(mCDevice);
        }

        public void setArchitecture(String arch) {
            handleReturn(CWrapper.mb_device_set_architecture(mCDevice, arch));
        }

        public long getFlags() {
            return CWrapper.mb_device_flags(mCDevice);
        }

        public void setFlags(long flags) {
            handleReturn(CWrapper.mb_device_set_flags(mCDevice, flags));
        }

        public String[] getBlockDevBaseDirs() {
            Pointer ptr = CWrapper.mb_device_block_dev_base_dirs(mCDevice);
            return ptr == null ? null : ptr.getStringArray(0);
        }

        public void setBlockDevBaseDirs(String[] dirs) {
            handleReturn(CWrapper.mb_device_set_block_dev_base_dirs(
                    mCDevice, new StringArray(dirs)));
        }

        public String[] getSystemBlockDevs() {
            Pointer ptr = CWrapper.mb_device_system_block_devs(mCDevice);
            return ptr == null ? null : ptr.getStringArray(0);
        }

        public void setSystemBlockDevs(String[] blockDevs) {
            handleReturn(CWrapper.mb_device_set_system_block_devs(
                    mCDevice, new StringArray(blockDevs)));
        }

        public String[] getCacheBlockDevs() {
            Pointer ptr = CWrapper.mb_device_cache_block_devs(mCDevice);
            return ptr == null ? null : ptr.getStringArray(0);
        }

        public void setCacheBlockDevs(String[] blockDevs) {
            handleReturn(CWrapper.mb_device_set_cache_block_devs(
                    mCDevice, new StringArray(blockDevs)));
        }

        public String[] getDataBlockDevs() {
            Pointer ptr = CWrapper.mb_device_data_block_devs(mCDevice);
            return ptr == null ? null : ptr.getStringArray(0);
        }

        public void setDataBlockDevs(String[] blockDevs) {
            handleReturn(CWrapper.mb_device_set_data_block_devs(
                    mCDevice, new StringArray(blockDevs)));
        }

        public String[] getBootBlockDevs() {
            Pointer ptr = CWrapper.mb_device_boot_block_devs(mCDevice);
            return ptr == null ? null : ptr.getStringArray(0);
        }

        public void setBootBlockDevs(String[] blockDevs) {
            handleReturn(CWrapper.mb_device_set_boot_block_devs(
                    mCDevice, new StringArray(blockDevs)));
        }

        public String[] getRecoveryBlockDevs() {
            Pointer ptr = CWrapper.mb_device_recovery_block_devs(mCDevice);
            return ptr == null ? null : ptr.getStringArray(0);
        }

        public void setRecoveryBlockDevs(String[] blockDevs) {
            handleReturn(CWrapper.mb_device_set_recovery_block_devs(
                    mCDevice, new StringArray(blockDevs)));
        }

        public String[] getExtraBlockDevs() {
            Pointer ptr = CWrapper.mb_device_extra_block_devs(mCDevice);
            return ptr == null ? null : ptr.getStringArray(0);
        }

        public void setExtraBlockDevs(String[] blockDevs) {
            handleReturn(CWrapper.mb_device_set_extra_block_devs(
                    mCDevice, new StringArray(blockDevs)));
        }

        public boolean isTwSupported() {
            return CWrapper.mb_device_tw_supported(mCDevice);
        }

        public void setTwSupported(boolean supported) {
            handleReturn(CWrapper.mb_device_set_tw_supported(mCDevice, supported));
        }

        public long getTwFlags() {
            return CWrapper.mb_device_tw_flags(mCDevice);
        }

        public void setTwFlags(long flags) {
            handleReturn(CWrapper.mb_device_set_tw_flags(mCDevice, flags));
        }

        public int getTwPixelFormat() {
            return CWrapper.mb_device_tw_pixel_format(mCDevice);
        }

        public void setTwPixelFormat(int format) {
            handleReturn(CWrapper.mb_device_set_tw_pixel_format(mCDevice, format));
        }

        public int getTwForcePixelFormat() {
            return CWrapper.mb_device_tw_force_pixel_format(mCDevice);
        }

        public void setTwForcePixelFormat(int format) {
            handleReturn(CWrapper.mb_device_set_tw_force_pixel_format(mCDevice, format));
        }

        public int getTwOverscanPercent() {
            return CWrapper.mb_device_tw_overscan_percent(mCDevice);
        }

        public void setTwOverscanPercent(int percent) {
            handleReturn(CWrapper.mb_device_set_tw_overscan_percent(mCDevice, percent));
        }

        public int getTwDefaultXOffset() {
            return CWrapper.mb_device_tw_default_x_offset(mCDevice);
        }

        public void setTwDefaultXOffset(int offset) {
            handleReturn(CWrapper.mb_device_set_tw_default_x_offset(mCDevice, offset));
        }

        public int getTwDefaultYOffset() {
            return CWrapper.mb_device_tw_default_y_offset(mCDevice);
        }

        public void setTwDefaultYOffset(int offset) {
            handleReturn(CWrapper.mb_device_set_tw_default_y_offset(mCDevice, offset));
        }

        public String getTwBrightnessPath() {
            return CWrapper.mb_device_tw_brightness_path(mCDevice);
        }

        public void setTwBrightnessPath(String path) {
            handleReturn(CWrapper.mb_device_set_tw_brightness_path(mCDevice, path));
        }

        public String getTwSecondaryBrightnessPath() {
            return CWrapper.mb_device_tw_secondary_brightness_path(mCDevice);
        }

        public void setTwSecondaryBrightnessPath(String path) {
            handleReturn(CWrapper.mb_device_set_tw_secondary_brightness_path(mCDevice, path));
        }

        public int getTwMaxBrightness() {
            return CWrapper.mb_device_tw_max_brightness(mCDevice);
        }

        public void setTwMaxBrightness(int brightness) {
            handleReturn(CWrapper.mb_device_set_tw_max_brightness(mCDevice, brightness));
        }

        public int getTwDefaultBrightness() {
            return CWrapper.mb_device_tw_default_brightness(mCDevice);
        }

        public void setTwDefaultBrightness(int brightness) {
            handleReturn(CWrapper.mb_device_set_tw_default_brightness(mCDevice, brightness));
        }

        public String getTwBatteryPath() {
            return CWrapper.mb_device_tw_battery_path(mCDevice);
        }

        public void setTwBatteryPath(String path) {
            handleReturn(CWrapper.mb_device_set_tw_battery_path(mCDevice, path));
        }

        public String getTwCpuTempPath() {
            return CWrapper.mb_device_tw_cpu_temp_path(mCDevice);
        }

        public void setTwCpuTempPath(String path) {
            handleReturn(CWrapper.mb_device_set_tw_cpu_temp_path(mCDevice, path));
        }

        public String getTwInputBlacklist() {
            return CWrapper.mb_device_tw_input_blacklist(mCDevice);
        }

        public void setTwInputBlacklist(String blacklist) {
            handleReturn(CWrapper.mb_device_set_tw_input_blacklist(mCDevice, blacklist));
        }

        public String getTwInputWhitelist() {
            return CWrapper.mb_device_tw_input_whitelist(mCDevice);
        }

        public void setTwInputWhitelist(String whitelist) {
            handleReturn(CWrapper.mb_device_set_tw_input_whitelist(mCDevice, whitelist));
        }

        public String[] getTwGraphicsBackends() {
            Pointer ptr = CWrapper.mb_device_tw_graphics_backends(mCDevice);
            return ptr == null ? null : ptr.getStringArray(0);
        }

        public void setTwGraphicsBackends(String[] backends) {
            handleReturn(CWrapper.mb_device_set_tw_graphics_backends(
                    mCDevice, new StringArray(backends)));
        }

        public String getTwTheme() {
            return CWrapper.mb_device_tw_theme(mCDevice);
        }

        public void setTwTheme(String theme) {
            handleReturn(CWrapper.mb_device_set_tw_theme(mCDevice, theme));
        }

        public boolean isCryptoSupported() {
            return CWrapper.mb_device_crypto_supported(mCDevice);
        }

        public void setCryptoSupported(boolean supported) {
            handleReturn(CWrapper.mb_device_set_crypto_supported(mCDevice, supported));
        }

        public String getCryptoHeaderPath() {
            return CWrapper.mb_device_crypto_header_path(mCDevice);
        }

        public void setCryptoHeaderPath(String path) {
            handleReturn(CWrapper.mb_device_set_crypto_header_path(mCDevice, path));
        }

        public long validate() {
            return CWrapper.mb_device_validate(mCDevice);
        }
    }
}
