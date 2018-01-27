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

package com.github.chenxiaolong.dualbootpatcher.nativelib

import android.os.Parcel
import android.os.Parcelable

import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.CWrapper.CDevice
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.CWrapper.CJsonError
import com.sun.jna.IntegerType
import com.sun.jna.Native
import com.sun.jna.Pointer
import com.sun.jna.PointerType
import com.sun.jna.StringArray

import java.util.HashMap

// NOTE: Almost no checking of parameters is performed on both the Java and C side of this native
//       wrapper. As a rule of thumb, don't pass null to any function.

object LibMbDevice {
    @Suppress("FunctionName", "unused")
    internal object CWrapper {
        init {
            Native.register(CWrapper::class.java, "mbdevice")
        }

        class CDevice : PointerType {
            constructor()

            constructor(p: Pointer) : super(p)
        }

        class CJsonError : PointerType()

        class SizeT @JvmOverloads constructor(value: Long = 0)
                : IntegerType(Native.SIZE_T_SIZE, value, true) {
            override fun toByte(): Byte {
                TODO("not implemented")
            }

            override fun toChar(): Char {
                TODO("not implemented")
            }

            override fun toShort(): Short {
                TODO("not implemented")
            }
        }

        // BEGIN: device.h
        @JvmStatic
        external fun mb_device_new(): CDevice?

        @JvmStatic
        external fun mb_device_free(device: CDevice?)

        @JvmStatic
        external fun mb_device_id(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_id(device: CDevice, id: String?)

        @JvmStatic
        external fun mb_device_codenames(device: CDevice): Pointer? /* char ** */
        @JvmStatic
        external fun mb_device_set_codenames(device: CDevice, codenames: StringArray?)

        @JvmStatic
        external fun mb_device_name(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_name(device: CDevice, name: String?)

        @JvmStatic
        external fun mb_device_architecture(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_architecture(device: CDevice, architecture: String?)

        @JvmStatic
        external fun mb_device_flags(device: CDevice): Int /* uint32_t */
        @JvmStatic
        external fun mb_device_set_flags(device: CDevice, flags: Int)

        @JvmStatic
        external fun mb_device_block_dev_base_dirs(device: CDevice): Pointer? /* char ** */
        @JvmStatic
        external fun mb_device_set_block_dev_base_dirs(device: CDevice, baseDirs: StringArray?)

        @JvmStatic
        external fun mb_device_system_block_devs(device: CDevice): Pointer? /* char ** */
        @JvmStatic
        external fun mb_device_set_system_block_devs(device: CDevice, systemDevs: StringArray?)

        @JvmStatic
        external fun mb_device_cache_block_devs(device: CDevice): Pointer? /* char ** */
        @JvmStatic
        external fun mb_device_set_cache_block_devs(device: CDevice, cacheDevs: StringArray?)

        @JvmStatic
        external fun mb_device_data_block_devs(device: CDevice): Pointer? /* char ** */
        @JvmStatic
        external fun mb_device_set_data_block_devs(device: CDevice, dataDevs: StringArray?)

        @JvmStatic
        external fun mb_device_boot_block_devs(device: CDevice): Pointer? /* char ** */
        @JvmStatic
        external fun mb_device_set_boot_block_devs(device: CDevice, bootDevs: StringArray?)

        @JvmStatic
        external fun mb_device_recovery_block_devs(device: CDevice): Pointer? /* char ** */
        @JvmStatic
        external fun mb_device_set_recovery_block_devs(device: CDevice, recoveryDevs: StringArray?)

        @JvmStatic
        external fun mb_device_extra_block_devs(device: CDevice): Pointer? /* char ** */
        @JvmStatic
        external fun mb_device_set_extra_block_devs(device: CDevice, extraDevs: StringArray?)

        /* Boot UI */

        @JvmStatic
        external fun mb_device_tw_supported(device: CDevice): Boolean
        @JvmStatic
        external fun mb_device_set_tw_supported(device: CDevice, supported: Boolean)

        @JvmStatic
        external fun mb_device_tw_flags(device: CDevice): Int /* uint32_t */
        @JvmStatic
        external fun mb_device_set_tw_flags(device: CDevice, flags: Int)

        @JvmStatic
        external fun mb_device_tw_pixel_format(device: CDevice): Int /* uint32_t */
        @JvmStatic
        external fun mb_device_set_tw_pixel_format(device: CDevice, format: Int /* uint32_t */)

        @JvmStatic
        external fun mb_device_tw_force_pixel_format(device: CDevice): Int /* uint32_t */
        @JvmStatic
        external fun mb_device_set_tw_force_pixel_format(device: CDevice,
                                                         format: Int /* uint32_t */)

        @JvmStatic
        external fun mb_device_tw_overscan_percent(device: CDevice): Int
        @JvmStatic
        external fun mb_device_set_tw_overscan_percent(device: CDevice, percent: Int)

        @JvmStatic
        external fun mb_device_tw_default_x_offset(device: CDevice): Int
        @JvmStatic
        external fun mb_device_set_tw_default_x_offset(device: CDevice, offset: Int)

        @JvmStatic
        external fun mb_device_tw_default_y_offset(device: CDevice): Int
        @JvmStatic
        external fun mb_device_set_tw_default_y_offset(device: CDevice, offset: Int)

        @JvmStatic
        external fun mb_device_tw_brightness_path(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_tw_brightness_path(device: CDevice, path: String?)

        @JvmStatic
        external fun mb_device_tw_secondary_brightness_path(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_tw_secondary_brightness_path(device: CDevice, path: String?)

        @JvmStatic
        external fun mb_device_tw_max_brightness(device: CDevice): Int
        @JvmStatic
        external fun mb_device_set_tw_max_brightness(device: CDevice, brightness: Int)

        @JvmStatic
        external fun mb_device_tw_default_brightness(device: CDevice): Int
        @JvmStatic
        external fun mb_device_set_tw_default_brightness(device: CDevice, brightness: Int)

        @JvmStatic
        external fun mb_device_tw_battery_path(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_tw_battery_path(device: CDevice, path: String?)

        @JvmStatic
        external fun mb_device_tw_cpu_temp_path(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_tw_cpu_temp_path(device: CDevice, path: String?)

        @JvmStatic
        external fun mb_device_tw_input_blacklist(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_tw_input_blacklist(device: CDevice, blacklist: String?)

        @JvmStatic
        external fun mb_device_tw_input_whitelist(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_tw_input_whitelist(device: CDevice, whitelist: String?)

        @JvmStatic
        external fun mb_device_tw_graphics_backends(device: CDevice): Pointer? /* char ** */
        @JvmStatic
        external fun mb_device_set_tw_graphics_backends(device: CDevice, backends: StringArray?)

        @JvmStatic
        external fun mb_device_tw_theme(device: CDevice): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_set_tw_theme(device: CDevice, theme: String?)

        /* Other */

        @JvmStatic
        external fun mb_device_validate(device: CDevice): Int /* uint32_t */

        @JvmStatic
        external fun mb_device_equals(a: CDevice, b: CDevice): Boolean
        // END: device.h

        // BEGIN: json.h
        @JvmStatic
        external fun mb_device_json_error_new(): CJsonError?

        @JvmStatic
        external fun mb_device_json_error_free(error: CJsonError?)

        @JvmStatic
        external fun mb_device_json_error_type(error: CJsonError): Short /* uint16_t */
        @JvmStatic
        external fun mb_device_json_error_offset(error: CJsonError): SizeT /* size_t */
        @JvmStatic
        external fun mb_device_json_error_message(error: CJsonError): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_json_error_schema_uri(error: CJsonError): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_json_error_schema_keyword(error: CJsonError): Pointer? /* char * */
        @JvmStatic
        external fun mb_device_json_error_document_uri(error: CJsonError): Pointer? /* char * */

        @JvmStatic
        external fun mb_device_new_from_json(json: String, error: CJsonError): CDevice

        @JvmStatic
        external fun mb_device_new_list_from_json(json: String,
                                                  error: CJsonError): Pointer? /* CDevice ** */

        @JvmStatic
        external fun mb_device_to_json(device: CDevice): Pointer /* char * */
        // END: json.h
    }

    private fun <T> incrementRefCount(instances: HashMap<T, Int>, instance: T) {
        if (instances.containsKey(instance)) {
            var count = instances[instance]!!
            instances[instance] = ++count
        } else {
            instances[instance] = 1
        }
    }

    private fun <T> decrementRefCount(instances: HashMap<T, Int>, instance: T): Boolean {
        if (!instances.containsKey(instance)) {
            throw IllegalStateException("Ref count list does not contain instance")
        }

        val count = instances[instance]!! - 1
        return if (count == 0) {
            // Destroy
            instances.remove(instance)
            true
        } else {
            // Decrement
            instances[instance] = count
            false
        }
    }

    @Suppress("unused")
    class JsonError {
        internal var pointer: CJsonError? = null
            private set

        init {
            pointer = CWrapper.mb_device_json_error_new()
            if (pointer == null) {
                throw IllegalStateException("Failed to allocate CJsonError object")
            }
        }

        fun destroy() {
            if (pointer != null) {
                CWrapper.mb_device_json_error_free(pointer!!)
            }
            pointer = null
        }

        @Suppress("ProtectedInFinal")
        protected fun finalize() {
            destroy()
        }

        fun type(): Short {
            return CWrapper.mb_device_json_error_type(pointer!!)
        }

        fun offset(): Long {
            return CWrapper.mb_device_json_error_offset(pointer!!).toLong()
        }

        fun message(): String? {
            val p = CWrapper.mb_device_json_error_message(pointer!!) ?: return null
            return LibC.getStringAndFree(p)
        }

        fun schemaUri(): String? {
            val p = CWrapper.mb_device_json_error_schema_uri(pointer!!) ?: return null
            return LibC.getStringAndFree(p)
        }

        fun schemaKeyword(): String? {
            val p = CWrapper.mb_device_json_error_schema_keyword(pointer!!) ?: return null
            return LibC.getStringAndFree(p)
        }

        fun documentUri(): String? {
            val p = CWrapper.mb_device_json_error_document_uri(pointer!!) ?: return null
            return LibC.getStringAndFree(p)
        }
    }

    @Suppress("unused", "MemberVisibilityCanBePrivate")
    class Device : Parcelable {
        internal var pointer: CDevice? = null
        private var destroyable: Boolean = false

        var id: String?
            get() {
                val p = CWrapper.mb_device_id(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(id) = CWrapper.mb_device_set_id(pointer!!, id)

        var codenames: Array<String>?
            get() {
                val p = CWrapper.mb_device_codenames(pointer!!) ?: return null
                return LibC.getStringArrayAndFree(p)
            }
            set(names) = CWrapper.mb_device_set_codenames(pointer!!, StringArray(names))

        var name: String?
            get() {
                val p = CWrapper.mb_device_name(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(name) = CWrapper.mb_device_set_name(pointer!!, name)

        var architecture: String?
            get() {
                val p = CWrapper.mb_device_architecture(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(arch) = CWrapper.mb_device_set_architecture(pointer!!, arch)

        var flags: Int
            get() = CWrapper.mb_device_flags(pointer!!)
            set(flags) = CWrapper.mb_device_set_flags(pointer!!, flags)

        var blockDevBaseDirs: Array<String>?
            get() {
                val p = CWrapper.mb_device_block_dev_base_dirs(pointer!!) ?: return null
                return LibC.getStringArrayAndFree(p)
            }
            set(dirs) = CWrapper.mb_device_set_block_dev_base_dirs(pointer!!, StringArray(dirs))

        var systemBlockDevs: Array<String>?
            get() {
                val p = CWrapper.mb_device_system_block_devs(pointer!!) ?: return null
                return LibC.getStringArrayAndFree(p)
            }
            set(blockDevs) = CWrapper.mb_device_set_system_block_devs(pointer!!, StringArray(blockDevs))

        var cacheBlockDevs: Array<String>?
            get() {
                val p = CWrapper.mb_device_cache_block_devs(pointer!!) ?: return null
                return LibC.getStringArrayAndFree(p)
            }
            set(blockDevs) = CWrapper.mb_device_set_cache_block_devs(pointer!!, StringArray(blockDevs))

        var dataBlockDevs: Array<String>?
            get() {
                val p = CWrapper.mb_device_data_block_devs(pointer!!) ?: return null
                return LibC.getStringArrayAndFree(p)
            }
            set(blockDevs) = CWrapper.mb_device_set_data_block_devs(pointer!!, StringArray(blockDevs))

        var bootBlockDevs: Array<String>?
            get() {
                val p = CWrapper.mb_device_boot_block_devs(pointer!!) ?: return null
                return LibC.getStringArrayAndFree(p)
            }
            set(blockDevs) = CWrapper.mb_device_set_boot_block_devs(pointer!!, StringArray(blockDevs))

        var recoveryBlockDevs: Array<String>?
            get() {
                val p = CWrapper.mb_device_recovery_block_devs(pointer!!) ?: return null
                return LibC.getStringArrayAndFree(p)
            }
            set(blockDevs) = CWrapper.mb_device_set_recovery_block_devs(pointer!!, StringArray(blockDevs))

        var extraBlockDevs: Array<String>?
            get() {
                val p = CWrapper.mb_device_extra_block_devs(pointer!!) ?: return null
                return LibC.getStringArrayAndFree(p)
            }
            set(blockDevs) = CWrapper.mb_device_set_extra_block_devs(pointer!!, StringArray(blockDevs))

        var isTwSupported: Boolean
            get() = CWrapper.mb_device_tw_supported(pointer!!)
            set(supported) = CWrapper.mb_device_set_tw_supported(pointer!!, supported)

        var twFlags: Int
            get() = CWrapper.mb_device_tw_flags(pointer!!)
            set(flags) = CWrapper.mb_device_set_tw_flags(pointer!!, flags)

        var twPixelFormat: Int
            get() = CWrapper.mb_device_tw_pixel_format(pointer!!)
            set(format) = CWrapper.mb_device_set_tw_pixel_format(pointer!!, format)

        var twForcePixelFormat: Int
            get() = CWrapper.mb_device_tw_force_pixel_format(pointer!!)
            set(format) = CWrapper.mb_device_set_tw_force_pixel_format(pointer!!, format)

        var twOverscanPercent: Int
            get() = CWrapper.mb_device_tw_overscan_percent(pointer!!)
            set(percent) = CWrapper.mb_device_set_tw_overscan_percent(pointer!!, percent)

        var twDefaultXOffset: Int
            get() = CWrapper.mb_device_tw_default_x_offset(pointer!!)
            set(offset) = CWrapper.mb_device_set_tw_default_x_offset(pointer!!, offset)

        var twDefaultYOffset: Int
            get() = CWrapper.mb_device_tw_default_y_offset(pointer!!)
            set(offset) = CWrapper.mb_device_set_tw_default_y_offset(pointer!!, offset)

        var twBrightnessPath: String?
            get() {
                val p = CWrapper.mb_device_tw_brightness_path(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(path) = CWrapper.mb_device_set_tw_brightness_path(pointer!!, path)

        var twSecondaryBrightnessPath: String?
            get() {
                val p = CWrapper.mb_device_tw_secondary_brightness_path(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(path) = CWrapper.mb_device_set_tw_secondary_brightness_path(pointer!!, path)

        var twMaxBrightness: Int
            get() = CWrapper.mb_device_tw_max_brightness(pointer!!)
            set(brightness) = CWrapper.mb_device_set_tw_max_brightness(pointer!!, brightness)

        var twDefaultBrightness: Int
            get() = CWrapper.mb_device_tw_default_brightness(pointer!!)
            set(brightness) = CWrapper.mb_device_set_tw_default_brightness(pointer!!, brightness)

        var twBatteryPath: String?
            get() {
                val p = CWrapper.mb_device_tw_battery_path(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(path) = CWrapper.mb_device_set_tw_battery_path(pointer!!, path)

        var twCpuTempPath: String?
            get() {
                val p = CWrapper.mb_device_tw_cpu_temp_path(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(path) = CWrapper.mb_device_set_tw_cpu_temp_path(pointer!!, path)

        var twInputBlacklist: String?
            get() {
                val p = CWrapper.mb_device_tw_input_blacklist(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(blacklist) = CWrapper.mb_device_set_tw_input_blacklist(pointer!!, blacklist)

        var twInputWhitelist: String?
            get() {
                val p = CWrapper.mb_device_tw_input_whitelist(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(whitelist) = CWrapper.mb_device_set_tw_input_whitelist(pointer!!, whitelist)

        var twGraphicsBackends: Array<String>?
            get() {
                val p = CWrapper.mb_device_tw_graphics_backends(pointer!!) ?: return null
                return LibC.getStringArrayAndFree(p)
            }
            set(backends) = CWrapper.mb_device_set_tw_graphics_backends(pointer!!, StringArray(backends))

        var twTheme: String?
            get() {
                val p = CWrapper.mb_device_tw_theme(pointer!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(theme) = CWrapper.mb_device_set_tw_theme(pointer!!, theme)

        constructor() {
            pointer = CWrapper.mb_device_new()
            if (pointer == null) {
                throw IllegalStateException("Failed to allocate device object")
            }

            synchronized(instances) {
                incrementRefCount(instances, pointer!!)
            }
            destroyable = true
        }

        internal constructor(cDevice: CDevice?, destroyable: Boolean) {
            if (cDevice == null) {
                throw IllegalArgumentException("Cannot wrap null device object pointer")
            }

            pointer = cDevice
            synchronized(instances) {
                incrementRefCount(instances, pointer!!)
            }
            this.destroyable = destroyable
        }

        fun destroy() {
            synchronized(instances) {
                if (pointer != null && decrementRefCount(instances, pointer!!)
                        && destroyable) {
                    CWrapper.mb_device_free(pointer!!)
                }
                pointer = null
            }
        }

        override fun equals(other: Any?): Boolean {
            if (other !is Device) {
                return false
            }

            val otherP = other.pointer
            return pointer == otherP || CWrapper.mb_device_equals(pointer!!, otherP!!)
        }

        override fun hashCode(): Int {
            var hashCode = 1
            hashCode = 31 * hashCode + pointer!!.hashCode()
            hashCode = 31 * hashCode + if (destroyable) 1 else 0
            return hashCode
        }

        @Suppress("ProtectedInFinal")
        protected fun finalize() {
            destroy()
        }

        override fun describeContents(): Int {
            return 0
        }

        override fun writeToParcel(out: Parcel, flags: Int) {
            val peer = Pointer.nativeValue(pointer!!.pointer)
            out.writeLong(peer)
            out.writeInt(if (destroyable) 1 else 0)
        }

        private constructor(p: Parcel) {
            val peer = p.readLong()
            pointer = CDevice()
            pointer!!.pointer = Pointer(peer)
            destroyable = p.readInt() != 0
            synchronized(instances) {
                incrementRefCount(instances, pointer!!)
            }
            if (pointer == null) {
                throw IllegalStateException("Deserialized device object is null")
            }
        }

        fun validate(): Long {
            return CWrapper.mb_device_validate(pointer!!).toLong()
        }

        companion object {
            private val instances = HashMap<CDevice, Int>()

            @JvmField val CREATOR = object : Parcelable.Creator<Device> {
                override fun createFromParcel(p: Parcel): Device {
                    return Device(p)
                }

                override fun newArray(size: Int): Array<Device?> {
                    return arrayOfNulls(size)
                }
            }

            fun newFromJson(json: String): Device {
                val error = JsonError()
                val device = newFromJson(json, error)
                error.destroy()
                return device
            }

            fun newFromJson(json: String, error: JsonError): Device {
                return Device(CWrapper.mb_device_new_from_json(json, error.pointer!!), true)
            }

            fun newListFromJson(json: String): Array<Device>? {
                val error = JsonError()
                val devices = newListFromJson(json, error)
                error.destroy()
                return devices
            }

            fun newListFromJson(json: String, error: JsonError): Array<Device>? {
                val p = CWrapper.mb_device_new_list_from_json(json, error.pointer!!) ?: return null
                val cDevices = p.getPointerArray(0)

                val devices = cDevices.map { Device(CDevice(it), true) }

                LibC.free(p)
                return devices.toTypedArray()
            }
        }
    }
}
