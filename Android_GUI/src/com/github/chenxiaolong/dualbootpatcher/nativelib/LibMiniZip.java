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

package com.github.chenxiaolong.dualbootpatcher.nativelib;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;

import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.NativeLong;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.ptr.PointerByReference;

import org.apache.commons.io.Charsets;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;

/**
 * A FFI layer for minizip. It contains the bare minimum needed for reading a ZIP file from Java.
 */
public class LibMiniZip {
    private static final String TAG = LibMiniZip.class.getSimpleName();

    // BEGIN: mz.h
    private static final int MZ_OK = 0;
    private static final int MZ_END_OF_LIST = -100;

    private static final int MZ_OPEN_MODE_READ = 0x01;
    // END: mz.h

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class mz_zip_file extends Structure {
        public /* uint16_t */ short version_madeby;
        public /* uint16_t */ short version_needed;
        public /* uint16_t */ short flag;
        public /* uint16_t */ short compression_method;
        public /* time_t */ NativeLong modified_date;
        public /* time_t */ NativeLong accessed_date;
        public /* time_t */ NativeLong creation_date;
        public /* uint32_t */ int crc;
        public /* uint64_t */ long compressed_size;
        public /* uint64_t */ long uncompressed_size;
        public /* uint16_t */ short filename_size;
        public /* uint16_t */ short extrafield_size;
        public /* uint16_t */ short comment_size;
        public /* uint32_t */ int disk_number;
        public /* uint64_t */ long disk_offset;
        public /* uint16_t */ short internal_fa;
        public /* uint32_t */ int external_fa;

        public /* char * */ Pointer filename;
        public /* uint8_t * */ Pointer extrafield;
        public /* char * */ Pointer comment;

        public mz_zip_file(Pointer p) {
            super(p);
            read();
        }

        @Override
        protected List<String> getFieldOrder() {
            return Arrays.asList("version_madeby", "version_needed", "flag",
                    "compression_method", "modified_date", "accessed_date", "creation_date", "crc",
                    "compressed_size", "uncompressed_size", "filename_size", "extrafield_size",
                    "comment_size", "disk_number", "disk_offset", "internal_fa", "external_fa",
                    "filename", "extrafield", "comment");
        }
    }

    @SuppressWarnings({"JniMissingFunction"})
    static class CWrapper {
        static {
            Native.register(CWrapper.class, "minizip");
        }

        // BEGIN: mz_strm.h
        static native /* int32_t */ int mz_stream_open(/* void * */ Pointer stream,
                                                       String path, /* int32_t */ int mode);

        static native /* int32_t */ int mz_stream_is_open(/* void * */ Pointer stream);

        static native /* int32_t */ int mz_stream_close(/* void * */ Pointer stream);

        static native /* int32_t */ int mz_stream_set_base(/* void * */ Pointer stream,
                                                           /* void * */ Pointer base);

        static native void mz_stream_delete(/* void ** */ PointerByReference stream);
        // END: mz_strm.h

        // BEGIN: mz_strm_android.h
        static native /* void * */ Pointer mz_stream_android_create(/* void ** */
                                                                    PointerByReference stream);
        // END: mz_strm_android.h

        // BEGIN: mz_strm_buf.h
        static native /* void * */ Pointer mz_stream_buffered_create(/* void ** */
                                                                     PointerByReference stream);
        // END: mz_strm_buf.h

        // BEGIN: mz_zip.h
        static native /* void * */ Pointer mz_zip_open(/* void * */ Pointer stream,
                                                       /* int32_t */ int mode);

        static native /* int32_t */ int mz_zip_close(/* void * */ Pointer handle);

        static native /* int32_t */ int mz_zip_entry_read_open(/* void * */ Pointer handle,
                                                               /* int16_t */ short raw,
                                                               /* const char * */ String password);

        static native /* int32_t */ int mz_zip_entry_read(/* void * */ Pointer handle,
                                                          /* void * */ Pointer buf,
                                                          /* uint32_t */ int len);

        static native /* int32_t */ int  mz_zip_entry_get_info(/* void * */ Pointer handle,
                                                               /* mz_zip_file ** */
                                                               PointerByReference file_info);

        static native /* int32_t */ int mz_zip_entry_close(/* void * */ Pointer handle);

        static native /* int32_t */ int mz_zip_goto_first_entry(/* void * */ Pointer handle);

        static native /* int32_t */ int mz_zip_goto_next_entry(/* void * */ Pointer handle);

        static native /* int32_t */ int mz_zip_locate_entry(/* void * */ Pointer handle,
                                                            String filename,
                                                            /* mz_filename_compare_cb */
                                                            Pointer filename_compare_cb);
    }

    @SuppressWarnings("WeakerAccess")
    public static class MiniZipEntry {
        String mComment;
        long mCompressedSize = -1;
        long mCrc = -1;
        byte[] mExtra;
        int mMethod = -1;
        String mName;
        long mSize = -1;
        long mTime = -1;

        @Nullable
        public String getComment() {
            return mComment;
        }

        public void setComment(@Nullable String comment) {
            mComment = comment;
        }

        public long getCompressedSize() {
            return mCompressedSize;
        }

        public void setCompressedSize(long size) {
            mCompressedSize = size;
        }

        public long getCrc() {
            return mCrc;
        }

        public void setCrc(long crc) {
            mCrc = crc;
        }

        public byte[] getExtra() {
            return mExtra;
        }

        public void setExtra(byte[] extra) {
            mExtra = extra;
        }

        public int getMethod() {
            return mMethod;
        }

        public void setMethod(int method) {
            mMethod = method;
        }

        public String getName() {
            return mName;
        }

        public void setName(String name) {
            mName = name;
        }

        public long getSize() {
            return mSize;
        }

        public void setSize(long size) {
            mSize = size;
        }

        public long getTime() {
            return mTime;
        }

        public void setTime(long time) {
            mTime = time;
        }
    }

    public static class MiniZipInputFile implements Closeable {
        private Pointer mFileStream;
        private Pointer mBufStream;
        private Pointer mFile;

        private MiniZipEntry mEntry;
        private MiniZipInputStream mStream;
        private boolean mSeekRequested = true;

        public MiniZipInputFile(@NonNull String path) throws IOException {
            try {
                // Create file stream
                PointerByReference ref = new PointerByReference();
                if (CWrapper.mz_stream_android_create(ref) == null) {
                    throw new IOException("Failed to create new file stream");
                }
                mFileStream = ref.getValue();

                // Create buffered stream
                ref = new PointerByReference();
                if (CWrapper.mz_stream_buffered_create(ref) == null) {
                    throw new IOException("Failed to create new buffered stream");
                }
                mBufStream = ref.getValue();

                // Set base stream for buffered stream
                int ret = CWrapper.mz_stream_set_base(mBufStream, mFileStream);
                if (ret != MZ_OK) {
                    throw new IOException("Failed to set base stream for buffered stream: " +
                            "error code " + ret);
                }

                // Open stream
                ret = CWrapper.mz_stream_open(mBufStream, path, MZ_OPEN_MODE_READ);
                if (ret != MZ_OK) {
                    throw new IOException("Failed to open stream: error code " + ret);
                }

                // Open zip
                mFile = CWrapper.mz_zip_open(mBufStream, MZ_OPEN_MODE_READ);
                if (mFile == null) {
                    throw new IOException("Failed to open zip for reading: " + path);
                }

                // Read the first entry
                goToFirstEntry();
            } catch (IOException e) {
                close();
                throw e;
            }
        }

        @Override
        public void close() throws IOException {
            try {
                closeExistingStream();
            } finally {
                if (mFile != null) {
                    CWrapper.mz_zip_close(mFile);
                    mFile = null;
                }
                if (mBufStream != null) {
                    if (CWrapper.mz_stream_is_open(mBufStream) == MZ_OK) {
                        CWrapper.mz_stream_close(mBufStream);
                    }
                    CWrapper.mz_stream_delete(new PointerByReference(mBufStream));
                    mBufStream = null;
                }
                if (mFileStream != null) {
                    CWrapper.mz_stream_delete(new PointerByReference(mFileStream));
                    mFileStream = null;
                }
                mEntry = null;
            }
        }

        @SuppressWarnings("FinalizeDoesntCallSuperFinalize")
        @Override
        protected void finalize() throws IOException {
            if (mFile != null) {
                Log.w(TAG, "Zip was open in finalize()!");
            }

            close();
        }

        private void ensureOpened() throws IOException {
            if (mFile == null) {
                throw new IOException("Zip file is not open");
            }
        }

        private void ensureSeeked() {
            if (mEntry == null) {
                // Bad calling code should not be recoverable
                throw new IllegalStateException("Zip file not seeked to entry");
            }
        }

        private void closeExistingStream() throws IOException {
            if (mStream != null) {
                try {
                    if (mStream.isOpen()) {
                        mStream.close();
                    }
                } finally {
                    mStream = null;
                }
            }
        }

        private void readEntry() throws IOException {
            PointerByReference ref = new PointerByReference();
            int ret = CWrapper.mz_zip_entry_get_info(mFile, ref);
            if (ret != MZ_OK) {
                throw new IOException("Failed to get entry metadata: error code " + ret);
            }
            mz_zip_file info = new mz_zip_file(ref.getValue());

            MiniZipEntry entry = new MiniZipEntry();

            if (info.filename_size <= 0) {
                throw new IOException("Invalid ZIP entry name");
            }

            byte[] buf = info.filename.getByteArray(0, info.filename_size);
            entry.setName(new String(buf, Charsets.UTF_8));

            if (info.extrafield_size > 0) {
                entry.setExtra(info.extrafield.getByteArray(0, info.extrafield_size));
            }
            if (info.comment_size > 0) {
                buf = info.comment.getByteArray(0,  info.comment_size);
                entry.setComment(new String(buf, Charsets.UTF_8));
            }

            entry.setCompressedSize(info.compressed_size);
            entry.setSize(info.uncompressed_size);
            entry.setMethod(info.compression_method);
            entry.setCrc(info.crc);
            entry.setTime(1000L * info.modified_date.longValue());

            mEntry = entry;
        }

        public void goToFirstEntry() throws IOException {
            ensureOpened();
            closeExistingStream();

            int ret = CWrapper.mz_zip_goto_first_entry(mFile);
            if (ret == MZ_OK) {
                readEntry();
                mSeekRequested = true;
            } else if (ret == MZ_END_OF_LIST) {
                // Empty zip
                mEntry = null;
                mSeekRequested = true;
            } else {
                mEntry = null;
                throw new IOException("Failed to move to first file: error code " + ret);
            }
        }

        public boolean goToNextEntry() throws IOException {
            ensureOpened();
            closeExistingStream();

            int ret = CWrapper.mz_zip_goto_next_entry(mFile);
            if (ret == MZ_OK) {
                readEntry();
                mSeekRequested = true;
                return true;
            } else if (ret == MZ_END_OF_LIST) {
                mEntry = null;
                mSeekRequested = true;
                return false;
            } else {
                mEntry = null;
                throw new IOException("Failed to move to next file: error code " + ret);
            }
        }

        public boolean goToEntry(@NonNull String filename) throws IOException {
            ensureOpened();
            closeExistingStream();

            int ret = CWrapper.mz_zip_locate_entry(mFile, filename, null);
            if (ret == MZ_OK) {
                readEntry();
                mSeekRequested = true;
                return true;
            } else if (ret == MZ_END_OF_LIST) {
                mEntry = null;
                mSeekRequested = true;
                return false;
            } else {
                mEntry = null;
                throw new IOException("Failed to move to " + filename + ": error code " + ret);
            }
        }

        @NonNull
        public MiniZipEntry getEntry() throws IOException {
            ensureOpened();
            ensureSeeked();

            return mEntry;
        }

        @NonNull
        public InputStream getInputStream() throws IOException {
            ensureOpened();
            ensureSeeked();
            closeExistingStream();

            return new MiniZipInputStream(mFile);
        }

        public MiniZipEntry nextEntry() throws IOException {
            ensureOpened();

            // If the user did not request an entry, read the next entry
            if (!mSeekRequested) {
                goToNextEntry();
            }

            mSeekRequested = false;

            return mEntry;
        }
    }

    private static class MiniZipInputStream extends InputStream {
        private /* void * */ Pointer mFile;

        MiniZipInputStream(/* void * */ Pointer file) throws IOException {
            mFile = file;

            int ret = CWrapper.mz_zip_entry_read_open(mFile, (short) 0, null);
            if (ret != MZ_OK) {
                throw new IOException("Failed to open inner file: error code " + ret);
            }
        }

        @Override
        public void close() throws IOException {
            if (mFile != null) {
                int ret = CWrapper.mz_zip_entry_close(mFile);
                mFile = null;

                if (ret != MZ_OK) {
                    throw new IOException("Failed to close inner file: error code " + ret);
                }
            }
        }

        private void ensureOpened() throws IOException {
            if (mFile == null) {
                throw new IOException("Stream is closed");
            }
        }

        @Override
        public int read() throws IOException {
            ensureOpened();

            Memory buf = new Memory(1);
            int ret = CWrapper.mz_zip_entry_read(mFile, buf, (int) buf.size());
            if (ret > 0) {
                return buf.getByte(0);
            } else if (ret == 0) {
                return -1;
            } else {
                throw new IOException("Failed to read byte: error code " + ret);
            }
        }

        @Override
        public int read(@NonNull byte[] b, int off, int len) throws IOException {
            if (off < 0 || len < 0 || len > b.length - off) {
                throw new IndexOutOfBoundsException();
            } else if (len == 0) {
                return 0;
            }

            ensureOpened();

            Memory buf = new Memory(len);
            int ret = CWrapper.mz_zip_entry_read(mFile, buf, (int) buf.size());
            if (ret > 0) {
                buf.read(0, b, off, ret);
                return ret;
            } else if (ret == 0) {
                return -1;
            } else {
                throw new IOException("Failed to read data: error code " + ret);
            }
        }

        public boolean isOpen() {
            return mFile != null;
        }
    }
}
