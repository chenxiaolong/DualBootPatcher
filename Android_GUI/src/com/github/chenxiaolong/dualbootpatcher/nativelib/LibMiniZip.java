/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMiniZip.CWrapper.UnzFile;
import com.sun.jna.Callback;
import com.sun.jna.Memory;
import com.sun.jna.Native;
import com.sun.jna.NativeLong;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;
import com.sun.jna.Structure;
import com.sun.jna.ptr.IntByReference;

import java.io.Closeable;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.List;

/**
 * A direct mapping of unzip.h and zip.h from minizip.
 */
public class LibMiniZip {
    private static final String TAG = LibMiniZip.class.getSimpleName();

    private static final int SEEK_SET = 0;
    private static final int SEEK_CUR = 1;
    private static final int SEEK_END = 2;

    private static final int UNZ_OK = 0;
    private static final int UNZ_END_OF_LIST_OF_FILE = -100;
    private static final int UNZ_ERRNO = /* Z_ERRNO */ -1;
    private static final int UNZ_EOF = 0;
    private static final int UNZ_PARAMERROR = -102;
    private static final int UNZ_BADZIPFILE = -103;
    private static final int UNZ_INTERNALERROR = -104;
    private static final int UNZ_CRCERROR = -105;

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class zlib_filefunc_def extends Structure {
        public /* open_file_func */ Pointer zopen_file;
        public /* opendisk_file_func */ Pointer zopendisk_file;
        public /* read_file_func */ Pointer zread_file;
        public /* write_file_func */ Pointer zwrite_file;
        public /* tell_file_func */ Pointer ztell_file;
        public /* seek_file_func */ Pointer zseek_file;
        public /* close_file_func */ Pointer zclose_file;
        public /* testerror_file_func */ Pointer zerror_file;
        public /* voidpf */ Pointer opaque;

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("zopen_file", "zopendisk_file", "zread_file", "zwrite_file",
                    "ztell_file", "zseek_file", "zclose_file", "zerror_file", "opaque");
        }
    }

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class zlib_filefunc64_def extends Structure {
        public /* open64_file_func */ Pointer zopen64_file;
        public /* opendisk64_file_func */ Pointer zopendisk64_file;
        public /* read_file_func */ Pointer zread_file;
        public /* write_file_func */ Pointer zwrite_file;
        public /* tell64_file_func */ Pointer ztell64_file;
        public /* seek64_file_func */ Pointer zseek64_file;
        public /* close_file_func */ Pointer zclose_file;
        public /* testerror_file_func */ Pointer zerror_file;
        public /* voidpf */ Pointer opaque;

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("zopen64_file", "zopendisk64_file", "zread_file", "zwrite_file",
                    "ztell64_file", "zseek64_file", "zclose_file", "zerror_file", "opaque");
        }
    }

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class tm_unz extends Structure {
        public int tm_sec;
        public int tm_min;
        public int tm_hour;
        public int tm_mday;
        public int tm_mon;
        public int tm_year;

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("tm_sec", "tm_min", "tm_hour", "tm_mday", "tm_mon", "tm_year");
        }
    }

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class unz_global_info64 extends Structure {
        public /* ZPOS64_T */ long number_entry;
        public NativeLong number_disk_with_CD;
        public NativeLong size_comment;

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("number_entry", "number_disk_with_CD", "size_comment");
        }
    }

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class unz_global_info extends Structure {
        public NativeLong number_entry;
        public NativeLong number_disk_with_CD;
        public NativeLong size_comment;

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("number_entry", "number_disk_with_CD", "size_comment");
        }
    }

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class unz_file_info64 extends Structure {
        public NativeLong version;
        public NativeLong version_needed;
        public NativeLong flag;
        public NativeLong compression_method;
        public NativeLong dosDate;
        public NativeLong crc;
        public /* ZPOS64_T */ long compressed_size;
        public /* ZPOS64_T */ long uncompressed_size;
        public NativeLong size_filename;
        public NativeLong size_file_extra;
        public NativeLong size_file_comment;

        public NativeLong disk_num_start;
        public NativeLong internal_fa;
        public NativeLong external_fa;

        public tm_unz tmu_date;
        public /* ZPOS64_T */ long disk_offset;
        public NativeLong size_file_extra_internal;

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("version", "version_needed", "flag", "compression_method",
                    "dosDate", "crc", "compressed_size", "uncompressed_size", "size_filename",
                    "size_file_extra", "size_file_comment", "disk_num_start", "internal_fa",
                    "external_fa", "tmu_date", "disk_offset", "size_file_extra_internal");
        }
    }

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class unz_file_info extends Structure {
        public NativeLong version;
        public NativeLong version_needed;
        public NativeLong flag;
        public NativeLong compression_method;
        public NativeLong dosDate;
        public NativeLong crc;
        public NativeLong compressed_size;
        public NativeLong uncompressed_size;
        public NativeLong size_filename;
        public NativeLong size_file_extra;
        public NativeLong size_file_comment;

        public NativeLong disk_num_start;
        public NativeLong internal_fa;
        public NativeLong external_fa;

        public tm_unz tmu_date;
        public NativeLong disk_offset;

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("version", "version_needed", "flag", "compression_method",
                    "dosDate", "crc", "compressed_size", "uncompressed_size", "size_filename",
                    "size_file_extra", "size_file_comment", "disk_num_start", "internal_fa",
                    "external_fa", "tmu_date", "disk_offset");
        }
    }

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class unz_file_pos extends Structure {
        public NativeLong pos_in_zip_directory;
        public NativeLong num_of_file;

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("pos_in_zip_directory", "num_of_file");
        }
    }

    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class unz64_file_pos extends Structure {
        public /* ZPOS64_T */ long pos_in_zip_directory;
        public /* ZPOS64_T */ long num_of_file;

        @Override
        protected List getFieldOrder() {
            return Arrays.asList("pos_in_zip_directory", "num_of_file");
        }
    }

    @SuppressWarnings({"JniMissingFunction", "unused"})
    static class CWrapper {
        static {
            Native.register(CWrapper.class, "minizip");
        }

        @SuppressWarnings("WeakerAccess")
        public static class UnzFile extends PointerType {}

        // BEGIN: ioandroid.h
        static native void fill_android_filefunc64(zlib_filefunc64_def pzlib_filefunc_def);
        // END: ioandroid.h

        // BEGIN: unz.h
        static native UnzFile unzOpen(String path);
        static native UnzFile unzOpen64(String path);

        static native UnzFile unzOpen2(String path, zlib_filefunc_def pzlib_filefunc_def);
        static native UnzFile unzOpen2_64(String path, zlib_filefunc64_def pzlib_filefunc_def);

        static native int unzClose(UnzFile file);

        static native int unzGetGlobalInfo(UnzFile file, unz_global_info pglobal_info);
        static native int unzGetGlobalInfo64(UnzFile file, unz_global_info64 pglobal_info);

        static native int unzGetGlobalComment(UnzFile file, Pointer comment,
                                              NativeLong comment_size);

        static native int unzOpenCurrentFile(UnzFile file);

        static native int unzOpenCurrentFilePassword(UnzFile file, String password);

        static native int unzOpenCurrentFile2(UnzFile file, IntByReference method,
                                              IntByReference level, int raw);

        static native int unzOpenCurrentFile3(UnzFile file, IntByReference method,
                                              IntByReference level, int raw, String password);

        static native int unzReadCurrentFile(UnzFile file, Pointer buf, /* unsigned */ int len);

        static native int unzGetCurrentFileInfo(UnzFile file, unz_file_info pfile_info,
                                                Pointer filename, NativeLong filename_size,
                                                Pointer extraField, NativeLong extrafield_size,
                                                Pointer comment, NativeLong comment_size);
        static native int unzGetCurrentFileInfo64(UnzFile file, unz_file_info64 pfile_info,
                                                  Pointer filename, NativeLong filename_size,
                                                  Pointer extrafield, NativeLong extrafield_size,
                                                  Pointer comment, NativeLong comment_size);

        static native /* ZPOS64_T */ long unzGetCurrentFileZStreamPos64(UnzFile file);

        static native int unzGetLocalExtrafield(UnzFile file, Pointer buf, /* unsigned */ int len);

        static native int unzCloseCurrentFile(UnzFile file);

        interface unzFileNameComparer extends Callback {
            int invoke(UnzFile file, String filename1, String filename2);
        }

        interface unzIteratorFunction extends Callback {
            int invoke(UnzFile file);
        }

        interface unzIteratorFunction2 extends Callback {
            int invoke(UnzFile file, unz_file_info64 pfile_info, String filename,
                       NativeLong filename_size, Pointer extrafield, NativeLong extrafield_size,
                       String comment, NativeLong comment_size);
        }

        static native int unzGoToFirstFile(UnzFile file);

        static native int unzGoToFirstFile2(UnzFile file, unz_file_info64 pfile_info,
                                            Pointer filename, NativeLong filename_size,
                                            Pointer extrafield, NativeLong extrafield_size,
                                            Pointer comment, NativeLong comment_size);

        static native int unzGoToNextFile(UnzFile file);

        static native int unzGoToNextFile2(UnzFile file, unz_file_info64 pfile_info,
                                           Pointer filename, NativeLong filename_size,
                                           Pointer extrafield, NativeLong extrafield_size,
                                           Pointer comment, NativeLong comment_size);

        static native int unzLocateFile(UnzFile file, String filename,
                                        unzFileNameComparer filename_compare_func);

        static native int unzGetFilePos(UnzFile file, unz_file_pos file_pos);
        static native int unzGoToFilePos(UnzFile file, unz_file_pos file_pos);

        static native int unzGetFilePos64(UnzFile file, unz64_file_pos file_pos);
        static native int unzGoToFilePos64(UnzFile file, unz64_file_pos file_pos);

        static native NativeLong unzGetOffset(UnzFile file);
        static native /* ZPOS64_T */ long unzGetOffset64(UnzFile file);

        static native int unzSetOffset(UnzFile file, NativeLong pos);
        static native int unzSetOffset64(UnzFile file, /* ZPOS64_T */ long pos);

        static native /* z_off_t */ NativeLong unztell(UnzFile file);
        static native /* ZPOS64_T */ long unztell64(UnzFile file);

        static native int unzseek(UnzFile file, /* z_off_t */ NativeLong offset, int origin);
        static native int unzseek64(UnzFile file, /* ZPOS64_T */ long offset, int origin);

        static native int unzeof(UnzFile file);
        // END: unz.h
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
        private final String mPath;
        private zlib_filefunc64_def mFuncs;
        private UnzFile mUnzFile;
        private MiniZipEntry mEntry;
        private MiniZipInputStream mStream;
        private boolean mSeekRequested = true;

        public MiniZipInputFile(@NonNull String path) throws IOException {
            mPath = path;

            mFuncs = new zlib_filefunc64_def();
            CWrapper.fill_android_filefunc64(mFuncs);

            mUnzFile = CWrapper.unzOpen2_64(mPath, mFuncs);
            if (mUnzFile == null) {
                throw new IOException("Failed to open zip for reading: " + mPath);
            }

            // MiniZip calls unzGoToFirstFile() on open, so read the first entry
            readEntry();
        }

        @Override
        public void close() throws IOException {
            if (mUnzFile != null) {
                try {
                    closeExistingStream();
                } finally {
                    CWrapper.unzClose(mUnzFile);
                    mFuncs = null;
                    mUnzFile = null;
                    mEntry = null;
                }
            }
        }

        @SuppressWarnings("FinalizeDoesntCallSuperFinalize")
        @Override
        protected void finalize() throws IOException {
            if (mUnzFile != null) {
                Log.w(TAG, "Zip was open in finalize()!");
            }

            close();
        }

        private void ensureOpened() throws IOException {
            if (mUnzFile == null) {
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
            unz_file_info64 info = new unz_file_info64();

            // First query to get filename size
            int ret = CWrapper.unzGetCurrentFileInfo64(mUnzFile, info, null, new NativeLong(0),
                    null, new NativeLong(0), null, new NativeLong(0));
            if (ret != UNZ_OK) {
                throw new IOException("Failed to get entry metadata: error code " + ret);
            }

            Memory bufName = null;
            Memory bufExtra = null;
            Memory bufComment = null;
            long bufNameSize = 0;
            long bufExtraSize = 0;
            long bufCommentSize = 0;

            if (info.size_filename.longValue() > 0) {
                bufNameSize = info.size_filename.longValue() + 1;
                bufName = new Memory(bufNameSize);
            }
            if (info.size_file_extra.longValue() > 0) {
                // Extra field is not NULL-terminated
                bufExtraSize = info.size_file_extra.longValue();
                bufExtra = new Memory(bufExtraSize);
            }
            if (info.size_file_comment.longValue() > 0) {
                bufCommentSize = info.size_file_comment.longValue() + 1;
                bufComment = new Memory(bufCommentSize);
            }

            ret = CWrapper.unzGetCurrentFileInfo64(mUnzFile, info,
                    bufName, new NativeLong(bufNameSize),
                    bufExtra, new NativeLong(bufExtraSize),
                    bufComment, new NativeLong(bufCommentSize));
            if (ret != UNZ_OK) {
                throw new IOException("Failed to get entry metadata: error code " + ret);
            }

            MiniZipEntry entry = new MiniZipEntry();
            if (bufName != null) {
                entry.setName(bufName.getString(0));
            }
            if (bufExtra != null) {
                entry.setExtra(bufExtra.getByteArray(0, info.size_file_extra.intValue()));
            }
            if (bufComment != null) {
                entry.setComment(bufComment.getString(0));
            }

            entry.setCompressedSize(info.compressed_size);
            entry.setSize(info.uncompressed_size);
            entry.setMethod(info.compression_method.intValue());
            entry.setCrc(info.crc.longValue());

            Calendar calendar = new GregorianCalendar(info.tmu_date.tm_year, info.tmu_date.tm_mon,
                    info.tmu_date.tm_mday, info.tmu_date.tm_hour, info.tmu_date.tm_min,
                    info.tmu_date.tm_sec);

            entry.setTime(calendar.getTime().getTime());

            mEntry = entry;
        }

        public void goToFirstEntry() throws IOException {
            ensureOpened();
            closeExistingStream();

            int ret = CWrapper.unzGoToFirstFile(mUnzFile);
            if (ret == UNZ_OK) {
                readEntry();
                mSeekRequested = true;
            } else {
                mEntry = null;
                throw new IOException("Failed to move to first file: error code " + ret);
            }
        }

        public boolean goToNextEntry() throws IOException {
            ensureOpened();
            closeExistingStream();

            int ret = CWrapper.unzGoToNextFile(mUnzFile);
            if (ret == UNZ_OK) {
                readEntry();
                mSeekRequested = true;
                return true;
            } else if (ret == UNZ_END_OF_LIST_OF_FILE) {
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

            int ret = CWrapper.unzLocateFile(mUnzFile, filename, null);
            if (ret == UNZ_OK) {
                readEntry();
                mSeekRequested = true;
                return true;
            } else if (ret == UNZ_END_OF_LIST_OF_FILE) {
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

            return new MiniZipInputStream(mUnzFile);
        }

        public MiniZipEntry nextEntry() throws IOException {
            ensureOpened();

            // If the user did not request an entry, read the next entry
            if (!mSeekRequested) {
                goToNextEntry();
            }

            mSeekRequested = false;

            return mEntry != null ? mEntry : null;
        }
    }

    private static class MiniZipInputStream extends InputStream {
        private UnzFile mUnzFile;

        MiniZipInputStream(UnzFile unzFile) throws IOException {
            mUnzFile = unzFile;

            int ret = CWrapper.unzOpenCurrentFile(mUnzFile);
            if (ret != UNZ_OK) {
                throw new IOException("Failed to open inner file: error code " + ret);
            }
        }

        @Override
        public void close() throws IOException {
            if (mUnzFile != null) {
                int ret = CWrapper.unzCloseCurrentFile(mUnzFile);
                mUnzFile = null;

                if (ret != UNZ_OK) {
                    throw new IOException("Failed to close inner file: error code " + ret);
                }
            }
        }

        private void ensureOpened() throws IOException {
            if (mUnzFile == null) {
                throw new IOException("Stream is closed");
            }
        }

        @Override
        public int read() throws IOException {
            ensureOpened();

            Memory buf = new Memory(1);
            int ret = CWrapper.unzReadCurrentFile(mUnzFile, buf, (int) buf.size());
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
            int ret = CWrapper.unzReadCurrentFile(mUnzFile, buf, (int) buf.size());
            if (ret > 0) {
                ByteBuffer bb = buf.getByteBuffer(0, ret);
                bb.get(b, off, ret);
                return ret;
            } else if (ret == 0) {
                return -1;
            } else {
                throw new IOException("Failed to read data: error code " + ret);
            }
        }

        public boolean isOpen() {
            return mUnzFile != null;
        }
    }
}
