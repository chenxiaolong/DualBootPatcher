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

package com.github.chenxiaolong.dualbootpatcher.nativelib

import android.os.Parcel
import android.os.Parcelable
import android.util.Log

import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.CWrapper.CDevice
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.CWrapper.CAutoPatcher
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.CWrapper.CFileInfo
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.CWrapper.CPatcher
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.CWrapper.CPatcherConfig
import com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff.LibMiscStuff
import com.sun.jna.Callback
import com.sun.jna.Native
import com.sun.jna.Pointer
import com.sun.jna.PointerType

import java.util.Arrays
import java.util.HashMap

// NOTE: Almost no checking of parameters is performed on both the Java and C side of this native
//       wrapper. As a rule of thumb, don't pass null to any function.

object LibMbPatcher {
    /** Log (almost) all native method calls and their parameters */
    private const val DEBUG = false

    @Suppress("FunctionName")
    internal object CWrapper {
        init {
            Native.register(CWrapper::class.java, "mbpatcher")
            LibMiscStuff.mblogSetLogcat()
        }

        // BEGIN: ctypes.h
        class CFileInfo : PointerType()

        class CPatcherConfig : PointerType()

        class CPatcher : PointerType()
        class CAutoPatcher : PointerType()
        // END: ctypes.h

        // BEGIN: cfileinfo.h
        @JvmStatic
        external fun mbpatcher_fileinfo_create(): CFileInfo?
        @JvmStatic
        external fun mbpatcher_fileinfo_destroy(info: CFileInfo?)
        @JvmStatic
        external fun mbpatcher_fileinfo_input_path(info: CFileInfo): Pointer?
        @JvmStatic
        external fun mbpatcher_fileinfo_set_input_path(info: CFileInfo, path: String?)
        @JvmStatic
        external fun mbpatcher_fileinfo_output_path(info: CFileInfo): Pointer?
        @JvmStatic
        external fun mbpatcher_fileinfo_set_output_path(info: CFileInfo, path: String?)
        @JvmStatic
        external fun mbpatcher_fileinfo_device(info: CFileInfo): CDevice?
        @JvmStatic
        external fun mbpatcher_fileinfo_set_device(info: CFileInfo, device: CDevice?)
        @JvmStatic
        external fun mbpatcher_fileinfo_rom_id(info: CFileInfo): Pointer?
        @JvmStatic
        external fun mbpatcher_fileinfo_set_rom_id(info: CFileInfo, id: String?)
        // END: cfileinfo.h

        // BEGIN: cpatcherconfig.h
        @JvmStatic
        external fun mbpatcher_config_create(): CPatcherConfig?
        @JvmStatic
        external fun mbpatcher_config_destroy(pc: CPatcherConfig?)
        @JvmStatic
        external fun mbpatcher_config_error(pc: CPatcherConfig): Int /* ErrorCode */
        @JvmStatic
        external fun mbpatcher_config_data_directory(pc: CPatcherConfig): Pointer
        @JvmStatic
        external fun mbpatcher_config_temp_directory(pc: CPatcherConfig): Pointer
        @JvmStatic
        external fun mbpatcher_config_set_data_directory(pc: CPatcherConfig, path: String)
        @JvmStatic
        external fun mbpatcher_config_set_temp_directory(pc: CPatcherConfig, path: String)
        @JvmStatic
        external fun mbpatcher_config_patchers(pc: CPatcherConfig): Pointer
        @JvmStatic
        external fun mbpatcher_config_autopatchers(pc: CPatcherConfig): Pointer
        @JvmStatic
        external fun mbpatcher_config_create_patcher(pc: CPatcherConfig, id: String): CPatcher?
        @JvmStatic
        external fun mbpatcher_config_create_autopatcher(pc: CPatcherConfig, id: String,
                                                         info: CFileInfo): CAutoPatcher?
        @JvmStatic
        external fun mbpatcher_config_destroy_patcher(pc: CPatcherConfig, patcher: CPatcher)
        @JvmStatic
        external fun mbpatcher_config_destroy_autopatcher(pc: CPatcherConfig,
                                                          patcher: CAutoPatcher)
        // END: cpatcherconfig.h

        // BEGIN: cpatcherinterface.h
        internal interface ProgressUpdatedCallback : Callback {
            operator fun invoke(bytes: Long, maxBytes: Long, userData: Pointer?)
        }

        internal interface FilesUpdatedCallback : Callback {
            operator fun invoke(files: Long, maxFiles: Long, userData: Pointer?)
        }

        internal interface DetailsUpdatedCallback : Callback {
            operator fun invoke(text: String, userData: Pointer?)
        }

        @JvmStatic
        external fun mbpatcher_patcher_error(patcher: CPatcher): Int /* ErrorCode */
        @JvmStatic
        external fun mbpatcher_patcher_id(patcher: CPatcher): Pointer
        @JvmStatic
        external fun mbpatcher_patcher_set_fileinfo(patcher: CPatcher, info: CFileInfo)
        @JvmStatic
        external fun mbpatcher_patcher_patch_file(patcher: CPatcher,
                                                  progressCb: ProgressUpdatedCallback?,
                                                  filesCb: FilesUpdatedCallback?,
                                                  detailsCb: DetailsUpdatedCallback?,
                                                  userData: Pointer?): Boolean
        @JvmStatic
        external fun mbpatcher_patcher_cancel_patching(patcher: CPatcher)

        @JvmStatic
        external fun mbpatcher_autopatcher_error(patcher: CAutoPatcher): Int /* ErrorCode */
        @JvmStatic
        external fun mbpatcher_autopatcher_id(patcher: CAutoPatcher): Pointer
        @JvmStatic
        external fun mbpatcher_autopatcher_new_files(patcher: CAutoPatcher): Pointer
        @JvmStatic
        external fun mbpatcher_autopatcher_existing_files(patcher: CAutoPatcher): Pointer
        @JvmStatic
        external fun mbpatcher_autopatcher_patch_files(patcher: CAutoPatcher,
                                                       directory: String): Boolean
        // END: cpatcherinterface.h
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

    private fun getSig(ptr: PointerType?, clazz: Class<*>, method: String,
                       vararg params: Any?): String {
        val sb = StringBuilder()
        sb.append('(')
        if (ptr == null) {
            sb.append("null")
        } else {
            sb.append(ptr.pointer)
        }
        sb.append(") ")
        sb.append(clazz.simpleName)
        sb.append('.')
        sb.append(method)
        sb.append('(')

        for (i in params.indices) {
            val param = params[i]

            when (param) {
                is Array<*> -> sb.append(Arrays.toString(param))
                is PointerType -> sb.append(param.pointer.getNativeLong(0))
                else -> sb.append(param)
            }

            if (i != params.size - 1) {
                sb.append(", ")
            }
        }

        sb.append(')')

        return sb.toString()
    }

    private fun validate(ptr: PointerType?, clazz: Class<*>, method: String, vararg params: Any?) {
        val signature = getSig(ptr, clazz, method, *params)

        @Suppress("ConstantConditionIf")
        if (DEBUG) {
            Log.d("libmbpatcher", signature)
        }

        if (ptr == null && method != "destroy") {
            throw IllegalStateException("Called on a destroyed object! $signature")
        }
    }

    @Suppress("unused")
    class FileInfo : Parcelable {
        private var cFileInfo: CFileInfo? = null

        internal val pointer: CFileInfo?
            get() {
                validate(cFileInfo, FileInfo::class.java, "getPointer")
                return cFileInfo
            }

        var inputPath: String?
            get() {
                validate(cFileInfo, FileInfo::class.java, "getInputPath")
                val p = CWrapper.mbpatcher_fileinfo_input_path(cFileInfo!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(path) {
                validate(cFileInfo, FileInfo::class.java, "setInputPath", path)
                CWrapper.mbpatcher_fileinfo_set_input_path(cFileInfo!!, path!!)
            }

        var outputPath: String?
            get() {
                validate(cFileInfo, FileInfo::class.java, "getOutputPath")
                val p = CWrapper.mbpatcher_fileinfo_output_path(cFileInfo!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(path) {
                validate(cFileInfo, FileInfo::class.java, "setOutputPath", path)
                CWrapper.mbpatcher_fileinfo_set_output_path(cFileInfo!!, path!!)
            }

        var device: Device?
            get() {
                validate(cFileInfo, FileInfo::class.java, "getDevice")
                val cDevice = CWrapper.mbpatcher_fileinfo_device(cFileInfo!!)
                return if (cDevice == null) null else Device(cDevice, false)
            }
            set(device) {
                validate(cFileInfo, FileInfo::class.java, "setDevice", device)
                CWrapper.mbpatcher_fileinfo_set_device(cFileInfo!!, device!!.pointer)
            }

        var romId: String?
            get() {
                validate(cFileInfo, FileInfo::class.java, "getRomId")
                val p = CWrapper.mbpatcher_fileinfo_rom_id(cFileInfo!!) ?: return null
                return LibC.getStringAndFree(p)
            }
            set(id) {
                validate(cFileInfo, FileInfo::class.java, "setRomId", id)
                CWrapper.mbpatcher_fileinfo_set_rom_id(cFileInfo!!, id!!)
            }

        constructor() {
            cFileInfo = CWrapper.mbpatcher_fileinfo_create()
            synchronized(instances) {
                incrementRefCount(instances, cFileInfo!!)
            }
            validate(cFileInfo, FileInfo::class.java, "(Constructor)")
        }

        internal constructor(cFileInfo: CFileInfo) {
            this.cFileInfo = cFileInfo
            synchronized(instances) {
                incrementRefCount(instances, this.cFileInfo!!)
            }
            validate(this.cFileInfo, FileInfo::class.java, "(Constructor)")
        }

        fun destroy() {
            synchronized(instances) {
                validate(cFileInfo, FileInfo::class.java, "destroy")
                if (cFileInfo != null && decrementRefCount(instances, cFileInfo!!)) {
                    validate(cFileInfo, FileInfo::class.java, "(Destroyed)")
                    CWrapper.mbpatcher_fileinfo_destroy(cFileInfo)
                }
                cFileInfo = null
            }
        }

        override fun equals(other: Any?): Boolean {
            return other is FileInfo && cFileInfo == other.cFileInfo
        }

        override fun hashCode(): Int {
            return cFileInfo!!.hashCode()
        }

        @Suppress("ProtectedInFinal")
        protected fun finalize() {
            destroy()
        }

        override fun describeContents(): Int {
            return 0
        }

        override fun writeToParcel(out: Parcel, flags: Int) {
            val peer = Pointer.nativeValue(cFileInfo!!.pointer)
            out.writeLong(peer)
        }

        private constructor(p: Parcel) {
            val peer = p.readLong()
            cFileInfo = CFileInfo()
            cFileInfo!!.pointer = Pointer(peer)
            synchronized(instances) {
                incrementRefCount(instances, cFileInfo!!)
            }
            validate(cFileInfo, FileInfo::class.java, "(Constructor)")
        }

        companion object {
            private val instances = HashMap<CFileInfo, Int>()

            @JvmField val CREATOR = object : Parcelable.Creator<FileInfo> {
                override fun createFromParcel(p: Parcel): FileInfo {
                    return FileInfo(p)
                }

                override fun newArray(size: Int): Array<FileInfo?> {
                    return arrayOfNulls(size)
                }
            }
        }
    }

    @Suppress("unused")
    class PatcherConfig : Parcelable {
        private var cPatcherConfig: CPatcherConfig? = null

        internal val pointer: CPatcherConfig?
            get() {
                validate(cPatcherConfig, PatcherConfig::class.java, "getPointer")
                return cPatcherConfig
            }

        val error: Int /* ErrorCode */
            get() {
                validate(cPatcherConfig, PatcherConfig::class.java, "getError")
                return CWrapper.mbpatcher_config_error(cPatcherConfig!!)
            }

        var dataDirectory: String
            get() {
                validate(cPatcherConfig, PatcherConfig::class.java, "getDataDirectory")
                val p = CWrapper.mbpatcher_config_data_directory(cPatcherConfig!!)
                return LibC.getStringAndFree(p)
            }
            set(path) {
                validate(cPatcherConfig, PatcherConfig::class.java, "setDataDirectory", path)
                CWrapper.mbpatcher_config_set_data_directory(cPatcherConfig!!, path)
            }

        var tempDirectory: String
            get() {
                validate(cPatcherConfig, PatcherConfig::class.java, "getTempDirectory")
                val p = CWrapper.mbpatcher_config_temp_directory(cPatcherConfig!!)
                return LibC.getStringAndFree(p)
            }
            set(path) {
                validate(cPatcherConfig, PatcherConfig::class.java, "setTempDirectory", path)
                CWrapper.mbpatcher_config_set_temp_directory(cPatcherConfig!!, path)
            }

        val patchers: Array<String>
            get() {
                validate(cPatcherConfig, PatcherConfig::class.java, "getPatchers")
                val p = CWrapper.mbpatcher_config_patchers(cPatcherConfig!!)
                return LibC.getStringArrayAndFree(p)
            }

        val autoPatchers: Array<String>
            get() {
                validate(cPatcherConfig, PatcherConfig::class.java, "getAutoPatchers")
                val p = CWrapper.mbpatcher_config_autopatchers(cPatcherConfig!!)
                return LibC.getStringArrayAndFree(p)
            }

        constructor() {
            cPatcherConfig = CWrapper.mbpatcher_config_create()
            synchronized(instances) {
                incrementRefCount(instances, cPatcherConfig!!)
            }
            validate(cPatcherConfig, PatcherConfig::class.java, "(Constructor)")
        }

        internal constructor(cPatcherConfig: CPatcherConfig) {
            this.cPatcherConfig = cPatcherConfig
            synchronized(instances) {
                incrementRefCount(instances, this.cPatcherConfig!!)
            }
            validate(this.cPatcherConfig, PatcherConfig::class.java, "(Constructor)")
        }

        fun destroy() {
            synchronized(instances) {
                validate(cPatcherConfig, PatcherConfig::class.java, "destroy")
                if (cPatcherConfig != null && decrementRefCount(instances, cPatcherConfig!!)) {
                    validate(cPatcherConfig, PatcherConfig::class.java, "(Destroyed)")
                    CWrapper.mbpatcher_config_destroy(cPatcherConfig)
                }
                cPatcherConfig = null
            }
        }

        override fun equals(other: Any?): Boolean {
            return other is PatcherConfig && cPatcherConfig == other.cPatcherConfig
        }

        override fun hashCode(): Int {
            return cPatcherConfig!!.hashCode()
        }

        @Suppress("ProtectedInFinal")
        protected fun finalize() {
            destroy()
        }

        override fun describeContents(): Int {
            return 0
        }

        override fun writeToParcel(out: Parcel, flags: Int) {
            val peer = Pointer.nativeValue(cPatcherConfig!!.pointer)
            out.writeLong(peer)
        }

        private constructor(p: Parcel) {
            val peer = p.readLong()
            cPatcherConfig = CPatcherConfig()
            cPatcherConfig!!.pointer = Pointer(peer)
            synchronized(instances) {
                incrementRefCount(instances, cPatcherConfig!!)
            }
            validate(cPatcherConfig, PatcherConfig::class.java, "(Constructor)")
        }

        fun createPatcher(id: String): Patcher? {
            validate(cPatcherConfig, PatcherConfig::class.java, "createPatcher", id)
            val patcher = CWrapper.mbpatcher_config_create_patcher(cPatcherConfig!!, id)
            return if (patcher == null) null else Patcher(patcher)
        }

        fun createAutoPatcher(id: String, info: FileInfo): AutoPatcher? {
            validate(cPatcherConfig, PatcherConfig::class.java, "createAutoPatcher", id, info)
            val patcher = CWrapper.mbpatcher_config_create_autopatcher(
                    cPatcherConfig!!, id, info.pointer!!)
            return if (patcher == null) null else AutoPatcher(patcher)
        }

        fun destroyPatcher(patcher: Patcher) {
            validate(cPatcherConfig, PatcherConfig::class.java, "destroyPatcher", patcher)
            CWrapper.mbpatcher_config_destroy_patcher(cPatcherConfig!!, patcher.pointer!!)
        }

        fun destroyAutoPatcher(patcher: AutoPatcher) {
            validate(cPatcherConfig, PatcherConfig::class.java, "destroyAutoPatcher", patcher)
            CWrapper.mbpatcher_config_destroy_autopatcher(cPatcherConfig!!, patcher.pointer!!)
        }

        companion object {
            private val instances = HashMap<CPatcherConfig, Int>()

            @JvmField val CREATOR = object : Parcelable.Creator<PatcherConfig> {
                override fun createFromParcel(p: Parcel): PatcherConfig {
                    return PatcherConfig(p)
                }

                override fun newArray(size: Int): Array<PatcherConfig?> {
                    return arrayOfNulls(size)
                }
            }
        }
    }

    @Suppress("unused")
    class Patcher : Parcelable {
        private var cPatcher: CPatcher? = null

        internal val pointer: CPatcher?
            get() {
                validate(cPatcher, Patcher::class.java, "getPointer")
                return cPatcher
            }

        val error: Int /* ErrorCode */
            get() {
                validate(cPatcher, Patcher::class.java, "getError")
                return CWrapper.mbpatcher_patcher_error(cPatcher!!)
            }

        val id: String
            get() {
                validate(cPatcher, Patcher::class.java, "getId")
                val p = CWrapper.mbpatcher_patcher_id(cPatcher!!)
                return LibC.getStringAndFree(p)
            }

        internal constructor(cPatcher: CPatcher) {
            this.cPatcher = cPatcher
            validate(this.cPatcher, Patcher::class.java, "(Constructor)")
        }

        override fun equals(other: Any?): Boolean {
            return other is Patcher && cPatcher == other.cPatcher
        }

        override fun hashCode(): Int {
            return cPatcher!!.hashCode()
        }

        override fun describeContents(): Int {
            return 0
        }

        override fun writeToParcel(out: Parcel, flags: Int) {
            val peer = Pointer.nativeValue(cPatcher!!.pointer)
            out.writeLong(peer)
        }

        private constructor(p: Parcel) {
            val peer = p.readLong()
            cPatcher = CPatcher()
            cPatcher!!.pointer = Pointer(peer)
            validate(cPatcher, Patcher::class.java, "(Constructor)")
        }

        fun setFileInfo(info: FileInfo) {
            validate(cPatcher, Patcher::class.java, "setFileInfo", info)
            CWrapper.mbpatcher_patcher_set_fileinfo(cPatcher!!, info.pointer!!)
        }

        fun patchFile(listener: ProgressListener?): Boolean {
            validate(cPatcher, Patcher::class.java, "patchFile", listener)
            var progressCb: CWrapper.ProgressUpdatedCallback? = null
            var filesCb: CWrapper.FilesUpdatedCallback? = null
            var detailsCb: CWrapper.DetailsUpdatedCallback? = null

            if (listener != null) {
                progressCb = object : CWrapper.ProgressUpdatedCallback {
                    override fun invoke(bytes: Long, maxBytes: Long, userData: Pointer?) {
                        listener.onProgressUpdated(bytes, maxBytes)
                    }
                }

                filesCb = object : CWrapper.FilesUpdatedCallback {
                    override fun invoke(files: Long, maxFiles: Long, userData: Pointer?) {
                        listener.onFilesUpdated(files, maxFiles)
                    }
                }

                detailsCb = object : CWrapper.DetailsUpdatedCallback {
                    override fun invoke(text: String, userData: Pointer?) {
                        listener.onDetailsUpdated(text)
                    }
                }
            }

            return CWrapper.mbpatcher_patcher_patch_file(cPatcher!!,
                    progressCb, filesCb, detailsCb, null)
        }

        fun cancelPatching() {
            validate(cPatcher, Patcher::class.java, "cancelPatching")
            CWrapper.mbpatcher_patcher_cancel_patching(cPatcher!!)
        }

        interface ProgressListener {
            fun onProgressUpdated(bytes: Long, maxBytes: Long)

            fun onFilesUpdated(files: Long, maxFiles: Long)

            fun onDetailsUpdated(text: String)
        }

        companion object {
            @JvmField val CREATOR = object : Parcelable.Creator<Patcher> {
                override fun createFromParcel(p: Parcel): Patcher {
                    return Patcher(p)
                }

                override fun newArray(size: Int): Array<Patcher?> {
                    return arrayOfNulls(size)
                }
            }
        }
    }

    @Suppress("unused")
    class AutoPatcher : Parcelable {
        private var cAutoPatcher: CAutoPatcher? = null

        internal val pointer: CAutoPatcher?
            get() {
                validate(cAutoPatcher, AutoPatcher::class.java, "getPointer")
                return cAutoPatcher
            }

        val error: Int /* ErrorCode */
            get() {
                validate(cAutoPatcher, AutoPatcher::class.java, "getError")
                return CWrapper.mbpatcher_autopatcher_error(cAutoPatcher!!)
            }

        val id: String
            get() {
                validate(cAutoPatcher, AutoPatcher::class.java, "getId")
                val p = CWrapper.mbpatcher_autopatcher_id(cAutoPatcher!!)
                return LibC.getStringAndFree(p)
            }

        internal constructor(cAutoPatcher: CAutoPatcher) {
            this.cAutoPatcher = cAutoPatcher
            validate(this.cAutoPatcher, AutoPatcher::class.java, "(Constructor)")
        }

        override fun equals(other: Any?): Boolean {
            return other is AutoPatcher && cAutoPatcher == other.cAutoPatcher
        }

        override fun hashCode(): Int {
            return cAutoPatcher!!.hashCode()
        }

        override fun describeContents(): Int {
            return 0
        }

        override fun writeToParcel(out: Parcel, flags: Int) {
            val peer = Pointer.nativeValue(cAutoPatcher!!.pointer)
            out.writeLong(peer)
        }

        private constructor(p: Parcel) {
            val peer = p.readLong()
            cAutoPatcher = CAutoPatcher()
            cAutoPatcher!!.pointer = Pointer(peer)
            validate(cAutoPatcher, AutoPatcher::class.java, "(Constructor)")
        }

        fun newFiles(): Array<String> {
            validate(cAutoPatcher, AutoPatcher::class.java, "newFiles")
            val p = CWrapper.mbpatcher_autopatcher_new_files(cAutoPatcher!!)
            return LibC.getStringArrayAndFree(p)
        }

        fun existingFiles(): Array<String> {
            validate(cAutoPatcher, AutoPatcher::class.java, "existingFiles")
            val p = CWrapper.mbpatcher_autopatcher_existing_files(cAutoPatcher!!)
            return LibC.getStringArrayAndFree(p)
        }

        fun patchFiles(directory: String): Boolean {
            validate(cAutoPatcher, AutoPatcher::class.java, "patchFiles", directory)
            return CWrapper.mbpatcher_autopatcher_patch_files(cAutoPatcher!!, directory)
        }

        companion object {
            @JvmField val CREATOR = object : Parcelable.Creator<AutoPatcher> {
                override fun createFromParcel(p: Parcel): AutoPatcher {
                    return AutoPatcher(p)
                }

                override fun newArray(size: Int): Array<AutoPatcher?> {
                    return arrayOfNulls(size)
                }
            }
        }
    }
}
