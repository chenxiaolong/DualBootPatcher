/*
 * Copyright (C) 2016-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.util.Log
import com.sun.jna.Memory
import com.sun.jna.Native
import com.sun.jna.NativeLong
import com.sun.jna.Pointer
import com.sun.jna.Structure
import com.sun.jna.ptr.PointerByReference
import java.io.Closeable
import java.io.IOException
import java.io.InputStream
import java.util.Arrays

/**
 * A FFI layer for minizip. It contains the bare minimum needed for reading a ZIP file from Java.
 */
object LibMiniZip {
    private val TAG = LibMiniZip::class.java.simpleName

    // BEGIN: mz.h
    private const val MZ_OK = 0
    private const val MZ_END_OF_LIST = -100

    private const val MZ_OPEN_MODE_READ = 0x01
    // END: mz.h

    @Suppress("PropertyName", "unused", "ClassName")
    class mz_zip_file(p: Pointer) : Structure(p) {
        @JvmField var version_madeby: Short /* uint16_t */ = 0
        @JvmField var version_needed: Short /* uint16_t */ = 0
        @JvmField var flag: Short /* uint16_t */ = 0
        @JvmField var compression_method: Short /* uint16_t */ = 0
        @JvmField var modified_date: NativeLong? /* time_t */ = null
        @JvmField var accessed_date: NativeLong? /* time_t */ = null
        @JvmField var creation_date: NativeLong? /* time_t */ = null
        @JvmField var crc: Int /* uint32_t */ = 0
        @JvmField var compressed_size: Long /* uint64_t */ = 0
        @JvmField var uncompressed_size: Long /* uint64_t */ = 0
        @JvmField var filename_size: Short /* uint16_t */ = 0
        @JvmField var extrafield_size: Short /* uint16_t */ = 0
        @JvmField var comment_size: Short /* uint16_t */ = 0
        @JvmField var disk_number: Int /* uint32_t */ = 0
        @JvmField var disk_offset: Long /* uint64_t */ = 0
        @JvmField var internal_fa: Short /* uint16_t */ = 0
        @JvmField var external_fa: Int /* uint32_t */ = 0
        @JvmField var zip64: Short /* uint16_t */ = 0

        @JvmField var filename: Pointer? /* char * */ = null
        @JvmField var extrafield: Pointer? /* uint8_t * */ = null
        @JvmField var comment: Pointer? /* char * */ = null

        init {
            read()
        }

        override fun getFieldOrder(): List<String> {
            return Arrays.asList("version_madeby", "version_needed", "flag",
                    "compression_method", "modified_date", "accessed_date", "creation_date", "crc",
                    "compressed_size", "uncompressed_size", "filename_size", "extrafield_size",
                    "comment_size", "disk_number", "disk_offset", "internal_fa", "external_fa",
                    "zip64", "filename", "extrafield", "comment")
        }
    }

    @Suppress("FunctionName")
    internal object CWrapper {
        init {
            Native.register(CWrapper::class.java, "minizip")
        }

        // BEGIN: mz_strm.h
        @JvmStatic
        external fun mz_stream_open(stream: Pointer /* void * */, path: String,
                                    mode: Int /* int32_t */): Int /* int32_t */

        @JvmStatic
        external fun mz_stream_is_open(stream: Pointer /* void * */): Int /* int32_t */

        @JvmStatic
        external fun mz_stream_close(stream: Pointer /* void * */): Int /* int32_t */

        @JvmStatic
        external fun mz_stream_set_base(stream: Pointer /* void * */, base: Pointer /* void * */):
                Int /* int32_t */

        @JvmStatic
        external fun mz_stream_delete(stream: PointerByReference /* void ** */)
        // END: mz_strm.h

        // BEGIN: mz_strm_android.h
        @JvmStatic
        external fun mz_stream_android_create(stream: PointerByReference /* void ** */):
                Pointer? /* void * */
        // END: mz_strm_android.h

        // BEGIN: mz_strm_buf.h
        @JvmStatic
        external fun mz_stream_buffered_create(stream: PointerByReference /* void ** */):
                Pointer? /* void * */
        // END: mz_strm_buf.h

        // BEGIN: mz_zip.h
        @JvmStatic
        external fun mz_zip_open(stream: Pointer /* void * */, mode: Int /* int32_t */):
                Pointer? /* void * */

        @JvmStatic
        external fun mz_zip_close(handle: Pointer /* void * */): Int /* int32_t */

        @JvmStatic
        external fun mz_zip_entry_read_open(handle: Pointer /* void * */, raw: Short /* int16_t */,
                                            password: String? /* const char * */): Int /* int32_t */

        @JvmStatic
        external fun mz_zip_entry_read(handle: Pointer /* void * */, buf: Pointer /* void * */,
                                       len: Int /* uint32_t */): Int /* int32_t */

        @JvmStatic
        external fun mz_zip_entry_get_info(handle: Pointer /* void * */,
                                           file_info: PointerByReference /* mz_zip_file ** */):
                Int /* int32_t */

        @JvmStatic
        external fun mz_zip_entry_close(handle: Pointer /* void * */): Int /* int32_t */

        @JvmStatic
        external fun mz_zip_goto_first_entry(handle: Pointer /* void * */): Int /* int32_t */

        @JvmStatic
        external fun mz_zip_goto_next_entry(handle: Pointer /* void * */): Int /* int32_t */

        @JvmStatic
        external fun mz_zip_locate_entry(handle: Pointer /* void * */, filename: String,
                                         filename_compare_cb: Pointer? /* mz_filename_compare_cb */):
                Int /* int32_t */
    }

    class MiniZipEntry {
        var comment: String? = null
        var compressedSize: Long = -1
        var crc: Int = -1
        var extra: ByteArray? = null
        var method: Short = -1
        var name: String? = null
        var size: Long = -1
        var time: Long = -1
    }

    class MiniZipInputFile @Throws(IOException::class)
    constructor(path: String) : Closeable {
        private var fileStream: Pointer? = null
        private var bufStream: Pointer? = null
        private var file: Pointer? = null

        private var _entry: MiniZipEntry? = null
        private var stream: MiniZipInputStream? = null
        private var seekRequested = true

        val entry: MiniZipEntry
            @Throws(IOException::class)
            get() {
                ensureOpened()
                ensureSeeked()

                return _entry!!
            }

        val inputStream: InputStream
            @Throws(IOException::class)
            get() {
                ensureOpened()
                ensureSeeked()
                closeExistingStream()

                return MiniZipInputStream(file!!)
            }

        init {
            try {
                // Create file stream
                var ref = PointerByReference()
                if (CWrapper.mz_stream_android_create(ref) == null) {
                    throw IOException("Failed to create new file stream")
                }
                fileStream = ref.value

                // Create buffered stream
                ref = PointerByReference()
                if (CWrapper.mz_stream_buffered_create(ref) == null) {
                    throw IOException("Failed to create new buffered stream")
                }
                bufStream = ref.value

                // Set base stream for buffered stream
                var ret = CWrapper.mz_stream_set_base(bufStream!!, fileStream!!)
                if (ret != MZ_OK) {
                    throw IOException("Failed to set base stream for buffered stream: " +
                            "error code $ret")
                }

                // Open stream
                ret = CWrapper.mz_stream_open(bufStream!!, path, MZ_OPEN_MODE_READ)
                if (ret != MZ_OK) {
                    throw IOException("Failed to open stream: error code $ret")
                }

                // Open zip
                file = CWrapper.mz_zip_open(bufStream!!, MZ_OPEN_MODE_READ)
                if (file == null) {
                    throw IOException("Failed to open zip for reading: $ret")
                }

                // Read the first entry
                goToFirstEntry()
            } catch (e: IOException) {
                close()
                throw e
            }
        }

        @Throws(IOException::class)
        override fun close() {
            try {
                closeExistingStream()
            } finally {
                if (file != null) {
                    CWrapper.mz_zip_close(file!!)
                    file = null
                }
                if (bufStream != null) {
                    if (CWrapper.mz_stream_is_open(bufStream!!) == MZ_OK) {
                        CWrapper.mz_stream_close(bufStream!!)
                    }
                    CWrapper.mz_stream_delete(PointerByReference(bufStream))
                    bufStream = null
                }
                if (fileStream != null) {
                    CWrapper.mz_stream_delete(PointerByReference(fileStream))
                    fileStream = null
                }
                _entry = null
            }
        }

        @Suppress("ProtectedInFinal")
        protected fun finalize() {
            if (file != null) {
                Log.w(TAG, "Zip was open in finalize()!")
            }

            close()
        }

        @Throws(IOException::class)
        private fun ensureOpened() {
            if (file == null) {
                throw IOException("Zip file is not open")
            }
        }

        private fun ensureSeeked() {
            if (_entry == null) {
                // Bad calling code should not be recoverable
                throw IllegalStateException("Zip file not seeked to entry")
            }
        }

        @Throws(IOException::class)
        private fun closeExistingStream() {
            if (stream != null) {
                try {
                    if (stream!!.isOpen) {
                        stream!!.close()
                    }
                } finally {
                    stream = null
                }
            }
        }

        @Throws(IOException::class)
        private fun readEntry() {
            val ref = PointerByReference()
            val ret = CWrapper.mz_zip_entry_get_info(file!!, ref)
            if (ret != MZ_OK) {
                throw IOException("Failed to get entry metadata: error code $ret")
            }
            val info = mz_zip_file(ref.value)

            val entry = MiniZipEntry()

            if (info.filename_size <= 0) {
                throw IOException("Invalid ZIP entry name")
            }

            var buf = info.filename!!.getByteArray(0, info.filename_size.toInt())
            entry.name = String(buf, Charsets.UTF_8)

            if (info.extrafield_size > 0) {
                entry.extra = info.extrafield!!.getByteArray(0, info.extrafield_size.toInt())
            }
            if (info.comment_size > 0) {
                buf = info.comment!!.getByteArray(0, info.comment_size.toInt())
                entry.comment = String(buf, Charsets.UTF_8)
            }

            entry.compressedSize = info.compressed_size
            entry.size = info.uncompressed_size
            entry.method = info.compression_method
            entry.crc = info.crc
            entry.time = 1000L * info.modified_date!!.toLong()

            _entry = entry
        }

        @Throws(IOException::class)
        fun goToFirstEntry() {
            ensureOpened()
            closeExistingStream()

            val ret = CWrapper.mz_zip_goto_first_entry(file!!)
            when (ret) {
                MZ_OK -> {
                    readEntry()
                    seekRequested = true
                }
                MZ_END_OF_LIST -> {
                    // Empty zip
                    _entry = null
                    seekRequested = true
                }
                else -> {
                    _entry = null
                    throw IOException("Failed to move to first file: error code $ret")
                }
            }
        }

        @Throws(IOException::class)
        fun goToNextEntry(): Boolean {
            ensureOpened()
            closeExistingStream()

            val ret = CWrapper.mz_zip_goto_next_entry(file!!)
            return when (ret) {
                MZ_OK -> {
                    readEntry()
                    seekRequested = true
                    true
                }
                MZ_END_OF_LIST -> {
                    _entry = null
                    seekRequested = true
                    false
                }
                else -> {
                    _entry = null
                    throw IOException("Failed to move to next file: error code $ret")
                }
            }
        }

        @Throws(IOException::class)
        fun goToEntry(filename: String): Boolean {
            ensureOpened()
            closeExistingStream()

            val ret = CWrapper.mz_zip_locate_entry(file!!, filename, null)
            return when (ret) {
                MZ_OK -> {
                    readEntry()
                    seekRequested = true
                    true
                }
                MZ_END_OF_LIST -> {
                    _entry = null
                    seekRequested = true
                    false
                }
                else -> {
                    _entry = null
                    throw IOException("Failed to move to $filename: error code $ret")
                }
            }
        }

        @Throws(IOException::class)
        fun nextEntry(): MiniZipEntry? {
            ensureOpened()

            // If the user did not request an entry, read the next entry
            if (!seekRequested) {
                goToNextEntry()
            }

            seekRequested = false

            return _entry
        }
    }

    private class MiniZipInputStream @Throws(IOException::class)
    internal constructor(file: Pointer /* void * */) : InputStream() {
        var file: Pointer? = file /* void * */

        val isOpen: Boolean
            get() = file != null

        init {
            val ret = CWrapper.mz_zip_entry_read_open(file, 0.toShort(), null)
            if (ret != MZ_OK) {
                throw IOException("Failed to open inner file: error code $ret")
            }
        }

        @Throws(IOException::class)
        override fun close() {
            if (file != null) {
                val ret = CWrapper.mz_zip_entry_close(file!!)
                file = null

                if (ret != MZ_OK) {
                    throw IOException("Failed to close inner file: error code $ret")
                }
            }
        }

        @Throws(IOException::class)
        private fun ensureOpened() {
            if (file == null) {
                throw IOException("Stream is closed")
            }
        }

        @Throws(IOException::class)
        override fun read(): Int {
            ensureOpened()

            val buf = Memory(1)
            val ret = CWrapper.mz_zip_entry_read(file!!, buf, buf.size().toInt())
            return when {
                ret > 0 -> buf.getByte(0).toInt()
                ret == 0 -> -1
                else -> throw IOException("Failed to read byte: error code $ret")
            }
        }

        @Throws(IOException::class)
        override fun read(b: ByteArray, off: Int, len: Int): Int {
            if (off < 0 || len < 0 || len > b.size - off) {
                throw IndexOutOfBoundsException()
            } else if (len == 0) {
                return 0
            }

            ensureOpened()

            val buf = Memory(len.toLong())
            val ret = CWrapper.mz_zip_entry_read(file!!, buf, buf.size().toInt())
            return when {
                ret > 0 -> {
                    buf.read(0, b, off, ret)
                    ret
                }
                ret == 0 -> -1
                else -> throw IOException("Failed to read data: error code $ret")
            }
        }
    }
}
