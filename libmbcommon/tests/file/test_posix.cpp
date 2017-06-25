/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>

#include "mbcommon/file.h"
#include "mbcommon/file/posix.h"
#include "mbcommon/file/posix_p.h"
#include "mbcommon/file/vtable_p.h"

struct FilePosixTest : testing::Test
{
    MbFile *_file;
    SysVtable _vtable;
    // Dummy fp that's never dereferenced
    FILE *_fp = reinterpret_cast<FILE *>(-1);

#ifdef _WIN32
    int _n_wfopen = 0;
#else
    int _n_fopen = 0;
#endif
    int _n_fstat = 0;
    int _n_fclose = 0;
    int _n_ferror = 0;
    int _n_fileno = 0;
    int _n_fread = 0;
    int _n_fseeko = 0;
    int _n_ftello = 0;
    int _n_fwrite = 0;
    int _n_ftruncate64 = 0;

    bool _stream_error = false;

    FilePosixTest() : _file(mb_file_new())
    {
        // These all fail by default
#ifdef _WIN32
        _vtable.fn_wfopen = _wfopen;
#else
        _vtable.fn_fopen = _fopen;
#endif
        _vtable.fn_fstat = _fstat;
        _vtable.fn_fclose = _fclose;
        _vtable.fn_ferror = _ferror;
        _vtable.fn_fileno = _fileno;
        _vtable.fn_fread = _fread;
        _vtable.fn_fseeko = _fseeko;
        _vtable.fn_ftello = _ftello;
        _vtable.fn_fwrite = _fwrite;
        _vtable.fn_ftruncate64 = _ftruncate64;

        _vtable.userdata = this;
    }

    virtual ~FilePosixTest()
    {
        mb_file_free(_file);
    }

#ifdef _WIN32
    static FILE * _wfopen(void *userdata, const wchar_t *filename,
                          const wchar_t *mode)
    {
        (void) filename;
        (void) mode;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_wfopen;

        errno = EIO;
        return nullptr;
    }
#else
    static FILE * _fopen(void *userdata, const char *filename, const char *mode)
    {
        (void) filename;
        (void) mode;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fopen;

        errno = EIO;
        return nullptr;
    }
#endif

    static int _fstat(void *userdata, int fildes, struct stat *buf)
    {
        (void) fildes;
        (void) buf;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fstat;

        errno = EIO;
        return -1;
    }

    static int _fclose(void *userdata, FILE *stream)
    {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fclose;

        errno = EIO;
        return -1;
    }

    static int _ferror(void *userdata, FILE *stream)
    {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ferror;

        return test->_stream_error;
    }

    static int _fileno(void *userdata, FILE *stream)
    {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return -1;
    }

    static size_t _fread(void *userdata, void *ptr, size_t size, size_t nmemb,
                         FILE *stream)
    {
        (void) ptr;
        (void) size;
        (void) nmemb;
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fread;

        // Make ferror return true
        test->_stream_error = true;
        errno = EIO;

        return 0;
    }

    static int _fseeko(void *userdata, FILE *stream, off_t offset, int whence)
    {
        (void) stream;
        (void) offset;
        (void) whence;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fseeko;

        return -1;
    }

    static off_t _ftello(void *userdata, FILE *stream)
    {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ftello;

        return -1;
    }

    static size_t _fwrite(void *userdata, const void *ptr, size_t size,
                          size_t nmemb, FILE *stream)
    {
        (void) ptr;
        (void) size;
        (void) nmemb;
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fwrite;

        // Make ferror return true
        test->_stream_error = true;
        errno = EIO;

        return 0;
    }

    static int _ftruncate64(void *userdata, int fd, off_t length)
    {
        (void) fd;
        (void) length;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ftruncate64;

        errno = EIO;
        return -1;
    }
};

TEST_F(FilePosixTest, OpenNoVtable)
{
    memset(&_vtable, 0, sizeof(_vtable));
    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, false), MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INTERNAL_ERROR);
}

TEST_F(FilePosixTest, OpenFilenameMbsSuccess)
{
#ifdef _WIN32
    _vtable.fn_wfopen = [](void *userdata, const wchar_t *filename,
                           const wchar_t *mode) -> FILE * {
        (void) filename;
        (void) mode;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_wfopen;

        return test->_fp;
    };
#else
    _vtable.fn_fopen = [](void *userdata, const char *filename,
                          const char *mode) -> FILE * {
        (void) filename;
        (void) mode;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fopen;

        return test->_fp;
    };
#endif

    ASSERT_EQ(_mb_file_open_FILE_filename(&_vtable, _file, "x",
                                          MB_FILE_OPEN_READ_ONLY),
              MB_FILE_OK);
#ifdef _WIN32
    ASSERT_EQ(_n_wfopen, 1);
#else
    ASSERT_EQ(_n_fopen, 1);
#endif
}

TEST_F(FilePosixTest, OpenFilenameMbsFailure)
{
    ASSERT_EQ(_mb_file_open_FILE_filename(&_vtable, _file, "x",
                                          MB_FILE_OPEN_READ_ONLY),
              MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
#ifdef _WIN32
    ASSERT_EQ(_n_wfopen, 1);
#else
    ASSERT_EQ(_n_fopen, 1);
#endif
}

TEST_F(FilePosixTest, OpenFilenameMbsInvalidMode)
{
    ASSERT_EQ(_mb_file_open_FILE_filename(&_vtable, _file, "x", -1),
              MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INVALID_ARGUMENT);
}

TEST_F(FilePosixTest, OpenFilenameWcsSuccess)
{
#ifdef _WIN32
    _vtable.fn_wfopen = [](void *userdata, const wchar_t *filename,
                           const wchar_t *mode) -> FILE * {
        (void) filename;
        (void) mode;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_wfopen;

        return test->_fp;
    };
#else
    _vtable.fn_fopen = [](void *userdata, const char *filename,
                          const char *mode) -> FILE * {
        (void) filename;
        (void) mode;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fopen;

        return test->_fp;
    };
#endif

    ASSERT_EQ(_mb_file_open_FILE_filename_w(&_vtable, _file, L"x",
                                            MB_FILE_OPEN_READ_ONLY),
              MB_FILE_OK);
#ifdef _WIN32
    ASSERT_EQ(_n_wfopen, 1);
#else
    ASSERT_EQ(_n_fopen, 1);
#endif
}

TEST_F(FilePosixTest, OpenFilenameWcsFailure)
{
    ASSERT_EQ(_mb_file_open_FILE_filename_w(&_vtable, _file, L"x",
                                            MB_FILE_OPEN_READ_ONLY),
              MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
#ifdef _WIN32
    ASSERT_EQ(_n_wfopen, 1);
#else
    ASSERT_EQ(_n_fopen, 1);
#endif
}

TEST_F(FilePosixTest, OpenFilenameWcsInvalidMode)
{
    ASSERT_EQ(_mb_file_open_FILE_filename_w(&_vtable, _file, L"x", -1),
              MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INVALID_ARGUMENT);
}

TEST_F(FilePosixTest, OpenFstatFailed)
{
    _vtable.fn_fileno = [](void *userdata, FILE *stream) {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, false), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    ASSERT_EQ(_n_fileno, 1);
    ASSERT_EQ(_n_fstat, 1);
}

TEST_F(FilePosixTest, OpenDirectory)
{
    _vtable.fn_fileno = [](void *userdata, FILE *stream) {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return 0;
    };
    _vtable.fn_fstat = [](void *userdata, int fildes, struct stat *buf)
    {
        (void) fildes;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, false), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EISDIR);
    ASSERT_EQ(_n_fileno, 1);
    ASSERT_EQ(_n_fstat, 1);
}

TEST_F(FilePosixTest, OpenFile)
{
    _vtable.fn_fileno = [](void *userdata, FILE *stream) {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return 0;
    };
    _vtable.fn_fstat = [](void *userdata, int fildes, struct stat *buf)
    {
        (void) fildes;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, false), MB_FILE_OK);
    ASSERT_EQ(_n_fileno, 1);
    ASSERT_EQ(_n_fstat, 1);
}

TEST_F(FilePosixTest, CloseUnownedFile)
{
    _vtable.fn_fclose = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fclose;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, false), MB_FILE_OK);

    // Ensure that the close callback is not called
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_n_fclose, 0);
}

TEST_F(FilePosixTest, CloseOwnedFile)
{
    _vtable.fn_fclose = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fclose;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the close callback is called
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_n_fclose, 1);
}

TEST_F(FilePosixTest, CloseFailure)
{
    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the close callback is called
    ASSERT_EQ(mb_file_close(_file), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    ASSERT_EQ(_n_fclose, 1);
}

TEST_F(FilePosixTest, ReadSuccess)
{
    _vtable.fn_fread = [](void *userdata, void *ptr, size_t size, size_t nmemb,
                          FILE *stream) -> size_t {
        (void) ptr;
        (void) size;
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fread;

        return nmemb;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(_n_fread, 1);
}

TEST_F(FilePosixTest, ReadEof)
{
    _vtable.fn_fread = [](void *userdata, void *ptr, size_t size, size_t nmemb,
                          FILE *stream) -> size_t {
        (void) ptr;
        (void) size;
        (void) nmemb;
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fread;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 0);
    ASSERT_EQ(_n_fread, 1);
}

TEST_F(FilePosixTest, ReadFailure)
{
    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_FAILED);
    ASSERT_EQ(_n_fread, 1);
    ASSERT_EQ(_n_ferror, 1);
    ASSERT_EQ(mb_file_error(_file), -EIO);
}

TEST_F(FilePosixTest, ReadFailureEINTR)
{
    _vtable.fn_fread = [](void *userdata, void *ptr, size_t size, size_t nmemb,
                          FILE *stream) -> size_t {
        (void) ptr;
        (void) size;
        (void) nmemb;
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fread;

        // Make ferror return true
        test->_stream_error = true;
        errno = EINTR;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_RETRY);
    ASSERT_EQ(_n_fread, 1);
    ASSERT_EQ(_n_ferror, 1);
    ASSERT_EQ(mb_file_error(_file), -EINTR);
}

TEST_F(FilePosixTest, WriteSuccess)
{
    _vtable.fn_fwrite = [](void *userdata, const void *ptr, size_t size,
                           size_t nmemb, FILE *stream) -> size_t {
        (void) ptr;
        (void) size;
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fwrite;

        return nmemb;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(_n_fwrite, 1);
}

TEST_F(FilePosixTest, WriteEof)
{
    _vtable.fn_fwrite = [](void *userdata, const void *ptr, size_t size,
                           size_t nmemb, FILE *stream) -> size_t {
        (void) ptr;
        (void) size;
        (void) nmemb;
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fwrite;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 0);
    ASSERT_EQ(_n_fwrite, 1);
}

TEST_F(FilePosixTest, WriteFailure)
{
    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_FAILED);
    ASSERT_EQ(_n_fwrite, 1);
    ASSERT_EQ(_n_ferror, 1);
    ASSERT_EQ(mb_file_error(_file), -EIO);
}

TEST_F(FilePosixTest, WriteFailureEINTR)
{
    _vtable.fn_fwrite = [](void *userdata, const void *ptr, size_t size,
                           size_t nmemb, FILE *stream) -> size_t {
        (void) ptr;
        (void) size;
        (void) nmemb;
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fwrite;

        // Make ferror return true
        test->_stream_error = true;
        errno = EINTR;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_RETRY);
    ASSERT_EQ(_n_fwrite, 1);
    ASSERT_EQ(_n_ferror, 1);
    ASSERT_EQ(mb_file_error(_file), -EINTR);
}

TEST_F(FilePosixTest, SeekSuccess)
{
    _vtable.fn_fseeko = [](void *userdata, FILE *stream, off_t offset,
                           int whence) -> int {
        (void) stream;
        (void) offset;
        (void) whence;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fseeko;

        return 0;
    };
    _vtable.fn_ftello = [](void *userdata, FILE *stream) -> off_t {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ftello;

        switch (test->_n_ftello) {
        case 1:
            return 0;
        case 2:
            return 10;
        default:
            return -1;
        }
    };
    _vtable.fn_fileno = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return 0;
    };
    _vtable.fn_fstat = [](void *userdata, int fildes, struct stat *buf) {
        (void) fildes;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    uint64_t offset;
    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, &offset), MB_FILE_OK);
    ASSERT_EQ(offset, 10);
    ASSERT_EQ(_n_fseeko, 1);
    ASSERT_EQ(_n_ftello, 2);
}

#ifndef __ANDROID__
#define LFS_SIZE (10ULL * 1024 * 1024 * 1024)
TEST_F(FilePosixTest, SeekSuccessLargeFile)
{
    _vtable.fn_fseeko = [](void *userdata, FILE *stream, off_t offset,
                           int whence) -> int {
        (void) stream;
        (void) offset;
        (void) whence;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fseeko;

        return 0;
    };
    _vtable.fn_ftello = [](void *userdata, FILE *stream) -> off_t {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ftello;

        return LFS_SIZE;
    };
    _vtable.fn_fileno = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return 0;
    };
    _vtable.fn_fstat = [](void *userdata, int fildes, struct stat *buf) {
        (void) fildes;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    // Ensure that the types (off_t, etc.) are large enough for LFS
    uint64_t offset;
    ASSERT_EQ(mb_file_seek(_file, LFS_SIZE, SEEK_SET, &offset), MB_FILE_OK);
    ASSERT_EQ(offset, LFS_SIZE);
    ASSERT_EQ(_n_fseeko, 1);
    ASSERT_EQ(_n_ftello, 2);
}
#undef LFS_SIZE
#endif

TEST_F(FilePosixTest, SeekFseekFailed)
{
    _vtable.fn_fseeko = [](void *userdata, FILE *stream, off_t offset,
                           int whence) -> int {
        (void) stream;
        (void) offset;
        (void) whence;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fseeko;

        errno = EIO;
        return -1;
    };
    _vtable.fn_ftello = [](void *userdata, FILE *stream) -> off_t {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ftello;

        return 0;
    };
    _vtable.fn_fileno = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return 0;
    };
    _vtable.fn_fstat = [](void *userdata, int fildes, struct stat *buf) {
        (void) fildes;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, nullptr), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    ASSERT_EQ(_n_fseeko, 1);
    ASSERT_EQ(_n_ftello, 1);
}

TEST_F(FilePosixTest, SeekFtellFailed)
{
    _vtable.fn_fseeko = [](void *userdata, FILE *stream, off_t offset,
                        int whence) -> int {
        (void) stream;
        (void) offset;
        (void) whence;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fseeko;

        return 0;
    };
    _vtable.fn_ftello = [](void *userdata, FILE *stream) -> off_t {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ftello;

        errno = EIO;
        return -1;
    };
    _vtable.fn_fileno = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return 0;
    };
    _vtable.fn_fstat = [](void *userdata, int fildes, struct stat *buf) {
        (void) fildes;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, nullptr), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    ASSERT_EQ(_n_fseeko, 0);
    ASSERT_EQ(_n_ftello, 1);
}

TEST_F(FilePosixTest, SeekSecondFtellFailed)
{
    _vtable.fn_fseeko = [](void *userdata, FILE *stream, off_t offset,
                           int whence) -> int {
        (void) stream;
        (void) offset;
        (void) whence;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fseeko;

        return 0;
    };
    _vtable.fn_ftello = [](void *userdata, FILE *stream) -> off_t {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ftello;

        switch (test->_n_ftello) {
        case 1:
            return 0;
        default:
            errno = EIO;
            return -1;
        }
    };
    _vtable.fn_fileno = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return 0;
    };
    _vtable.fn_fstat = [](void *userdata, int fildes, struct stat *buf) {
        (void) fildes;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, nullptr), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    // fseeko will be called a second time to restore the original position
    ASSERT_EQ(_n_fseeko, 2);
    ASSERT_EQ(_n_ftello, 2);
}

TEST_F(FilePosixTest, SeekSecondFtellFatal)
{
    _vtable.fn_fseeko = [](void *userdata, FILE *stream, off_t offset,
                        int whence) -> int {
        (void) stream;
        (void) offset;
        (void) whence;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fseeko;

        switch (test->_n_fseeko) {
        case 1:
            return 0;
        default:
            errno = EIO;
            return -1;
        }
        return 0;
    };
    _vtable.fn_ftello = [](void *userdata, FILE *stream) -> off_t {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ftello;

        switch (test->_n_ftello) {
        case 1:
            return 0;
        default:
            errno = EIO;
            return -1;
        }
    };
    _vtable.fn_fileno = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        return 0;
    };
    _vtable.fn_fstat = [](void *userdata, int fildes, struct stat *buf) {
        (void) fildes;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, nullptr), MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    // fseeko will be called a second time to restore the original position
    ASSERT_EQ(_n_fseeko, 2);
    ASSERT_EQ(_n_ftello, 2);
}

TEST_F(FilePosixTest, SeekUnsupported)
{
    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, nullptr), MB_FILE_UNSUPPORTED);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_UNSUPPORTED);
}

TEST_F(FilePosixTest, TruncateSuccess)
{
    _vtable.fn_fileno = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        // Fail when opening to avoid fstat check
        switch (test->_n_fileno) {
        case 1:
            return -1;
        default:
            return 0;
        }
    };
    _vtable.fn_ftruncate64 = [](void *userdata, int fd, off_t length) -> int {
        (void) fd;
        (void) length;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_ftruncate64;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_OK);
    ASSERT_EQ(_n_fileno, 2);
    ASSERT_EQ(_n_ftruncate64, 1);
}

TEST_F(FilePosixTest, TruncateUnsupported)
{
    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_UNSUPPORTED);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_UNSUPPORTED);
    ASSERT_EQ(_n_fileno, 2);
    ASSERT_EQ(_n_ftruncate64, 0);
}

TEST_F(FilePosixTest, TruncateFailed)
{
    _vtable.fn_fileno = [](void *userdata, FILE *stream) -> int {
        (void) stream;

        FilePosixTest *test = static_cast<FilePosixTest *>(userdata);
        ++test->_n_fileno;

        // Fail when opening to avoid fstat check
        switch (test->_n_fileno) {
        case 1:
            return -1;
        default:
            return 0;
        }
    };

    ASSERT_EQ(_mb_file_open_FILE(&_vtable, _file, _fp, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    ASSERT_EQ(_n_fileno, 2);
    ASSERT_EQ(_n_ftruncate64, 1);
}
