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

#include <climits>

#include <fcntl.h>

#include "mbcommon/file.h"
#include "mbcommon/file/fd.h"
#include "mbcommon/file/fd_p.h"
#include "mbcommon/file/vtable_p.h"

struct FileFdTest : testing::Test
{
    MbFile *_file;
    SysVtable _vtable;

#ifdef _WIN32
    int _n_wopen = 0;
#else
    int _n_open = 0;
#endif
    int _n_fstat = 0;
    int _n_close = 0;
    int _n_ftruncate64 = 0;
    int _n_lseek64 = 0;
    int _n_read = 0;
    int _n_write = 0;

    FileFdTest() : _file(mb_file_new())
    {
        // These all fail by default
#ifdef _WIN32
        _vtable.fn_wopen = _wopen;
#else
        _vtable.fn_open = _open;
#endif
        _vtable.fn_fstat = _fstat;
        _vtable.fn_close = _close;
        _vtable.fn_ftruncate64 = _ftruncate64;
        _vtable.fn_lseek64 = _lseek64;
        _vtable.fn_read = _read;
        _vtable.fn_write = _write;

        _vtable.userdata = this;
    }

    virtual ~FileFdTest()
    {
        mb_file_free(_file);
    }

#ifdef _WIN32
    static int _wopen(void *userdata, const wchar_t *path, int flags,
                      mode_t mode)
    {
        (void) path;
        (void) flags;
        (void) mode;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_wopen;

        errno = EIO;
        return -1;
    }
#else
    static int _open(void *userdata, const char *path, int flags, mode_t mode)
    {
        (void) path;
        (void) flags;
        (void) mode;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_open;

        errno = EIO;
        return -1;
    }
#endif

    static int _fstat(void *userdata, int fildes, struct stat *buf)
    {
        (void) fildes;
        (void) buf;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_fstat;

        errno = EIO;
        return -1;
    }

    static int _fstat_file(void *userdata, int fildes, struct stat *buf)
    {
        (void) fildes;
        (void) buf;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    }

    static int _close(void *userdata, int fd)
    {
        (void) fd;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_close;

        errno = EIO;
        return -1;
    }

    static int _ftruncate64(void *userdata, int fd, off_t length)
    {
        (void) fd;
        (void) length;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_ftruncate64;

        errno = EIO;
        return -1;
    }

    static off64_t _lseek64(void *userdata, int fd, off64_t offset, int whence)
    {
        (void) fd;
        (void) offset;
        (void) whence;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_lseek64;

        errno = EIO;
        return -1;
    }

    static ssize_t _read(void *userdata, int fd, void *buf, size_t count)
    {
        (void) fd;
        (void) buf;
        (void) count;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_read;

        errno = EIO;
        return -1;
    }

    static ssize_t _write(void *userdata, int fd, const void *buf, size_t count)
    {
        (void) fd;
        (void) buf;
        (void) count;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_write;

        errno = EIO;
        return -1;
    }
};

TEST_F(FileFdTest, OpenNoVtable)
{
    memset(&_vtable, 0, sizeof(_vtable));
    ASSERT_EQ(_mb_file_open_fd( &_vtable, _file, 0, false), MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INTERNAL_ERROR);
}

TEST_F(FileFdTest, OpenFilenameMbsSuccess)
{
#ifdef _WIN32
    _vtable.fn_wopen = [](void *userdata, const wchar_t *path, int flags,
                          mode_t mode) -> int {
        (void) path;
        (void) flags;
        (void) mode;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_wopen;

        return 0;
    };
#else
    _vtable.fn_open = [](void *userdata, const char *path, int flags,
                         mode_t mode) -> int {
        (void) path;
        (void) flags;
        (void) mode;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_open;

        return 0;
    };
#endif
    _vtable.fn_fstat = _fstat_file;

    ASSERT_EQ(_mb_file_open_fd_filename(&_vtable, _file, "x",
                                        MB_FILE_OPEN_READ_ONLY),
              MB_FILE_OK);
#ifdef _WIN32
    ASSERT_EQ(_n_wopen, 1);
#else
    ASSERT_EQ(_n_open, 1);
#endif
}

TEST_F(FileFdTest, OpenFilenameMbsFailure)
{
    _vtable.fn_fstat = _fstat_file;

    ASSERT_EQ(_mb_file_open_fd_filename(&_vtable, _file, "x",
                                        MB_FILE_OPEN_READ_ONLY),
              MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
#ifdef _WIN32
    ASSERT_EQ(_n_wopen, 1);
#else
    ASSERT_EQ(_n_open, 1);
#endif
}

TEST_F(FileFdTest, OpenFilenameMbsInvalidMode)
{
    ASSERT_EQ(_mb_file_open_fd_filename(&_vtable, _file, "x", -1),
              MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INVALID_ARGUMENT);
}

TEST_F(FileFdTest, OpenFilenameWcsSuccess)
{
#ifdef _WIN32
    _vtable.fn_wopen = [](void *userdata, const wchar_t *path, int flags,
                          mode_t mode) -> int {
        (void) path;
        (void) flags;
        (void) mode;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_wopen;

        return 0;
    };
#else
    _vtable.fn_open = [](void *userdata, const char *path, int flags,
                         mode_t mode) -> int {
        (void) path;
        (void) flags;
        (void) mode;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_open;

        return 0;
    };
#endif
    _vtable.fn_fstat = _fstat_file;

    ASSERT_EQ(_mb_file_open_fd_filename_w(&_vtable, _file, L"x",
                                          MB_FILE_OPEN_READ_ONLY),
              MB_FILE_OK);
#ifdef _WIN32
    ASSERT_EQ(_n_wopen, 1);
#else
    ASSERT_EQ(_n_open, 1);
#endif
}

TEST_F(FileFdTest, OpenFilenameWcsFailure)
{
    _vtable.fn_fstat = _fstat_file;

    ASSERT_EQ(_mb_file_open_fd_filename_w(&_vtable, _file, L"x",
                                          MB_FILE_OPEN_READ_ONLY),
              MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
#ifdef _WIN32
    ASSERT_EQ(_n_wopen, 1);
#else
    ASSERT_EQ(_n_open, 1);
#endif
}

TEST_F(FileFdTest, OpenFilenameWcsInvalidMode)
{
    ASSERT_EQ(_mb_file_open_fd_filename_w(&_vtable, _file, L"x", -1),
              MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INVALID_ARGUMENT);
}

TEST_F(FileFdTest, OpenFstatFailed)
{
    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, false), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    ASSERT_EQ(_n_fstat, 1);
}

TEST_F(FileFdTest, OpenDirectory)
{
    _vtable.fn_fstat = [](void *userdata, int fildes, struct stat *buf) -> int {
        (void) fildes;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_fstat;

        buf->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        return 0;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, false), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EISDIR);
    ASSERT_EQ(_n_fstat, 1);
}

TEST_F(FileFdTest, OpenFile)
{
    _vtable.fn_fstat = _fstat_file;

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, false), MB_FILE_OK);
    ASSERT_EQ(_n_fstat, 1);
}

TEST_F(FileFdTest, CloseUnownedFile)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_close = [](void *userdata, int fd) -> int {
        (void) fd;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_close;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, false), MB_FILE_OK);

    // Ensure that the close callback is not called
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_n_close, 0);
}

TEST_F(FileFdTest, CloseOwnedFile)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_close = [](void *userdata, int fd) -> int {
        (void) fd;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_close;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the close callback is called
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileFdTest, CloseFailure)
{
    _vtable.fn_fstat = _fstat_file;

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the close callback is called
    ASSERT_EQ(mb_file_close(_file), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileFdTest, ReadSuccess)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_read = [](void *userdata, int fd, void *buf, size_t count)
            -> ssize_t {
        (void) fd;
        (void) buf;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_read;

        return count;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(_n_read, 1);
}

#if SIZE_MAX > SSIZE_MAX
TEST_F(FileFdTest, ReadSuccessMaxSize)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_read = [](void *userdata, int fd, void *buf, size_t count)
            -> ssize_t {
        (void) fd;
        (void) buf;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_read;

        return count;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the read callback is called
    size_t n;
    ASSERT_EQ(mb_file_read(_file, nullptr, static_cast<size_t>(SSIZE_MAX) + 1,
              &n), MB_FILE_OK);
    ASSERT_EQ(n, SSIZE_MAX);
    ASSERT_EQ(_n_read, 1);
}
#endif

TEST_F(FileFdTest, ReadEof)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_read = [](void *userdata, int fd, void *buf, size_t count)
            -> ssize_t {
        (void) fd;
        (void) buf;
        (void) count;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_read;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 0);
    ASSERT_EQ(_n_read, 1);
}

TEST_F(FileFdTest, ReadFailure)
{
    _vtable.fn_fstat = _fstat_file;

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_FAILED);
    ASSERT_EQ(_n_read, 1);
    ASSERT_EQ(mb_file_error(_file), -EIO);
}

TEST_F(FileFdTest, ReadFailureEINTR)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_read = [](void *userdata, int fd, void *buf, size_t count)
            -> ssize_t {
        (void) fd;
        (void) buf;
        (void) count;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_read;

        errno = EINTR;
        return -1;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_RETRY);
    ASSERT_EQ(_n_read, 1);
    ASSERT_EQ(mb_file_error(_file), -EINTR);
}

TEST_F(FileFdTest, WriteSuccess)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_write = [](void *userdata, int fd, const void *buf, size_t count)
            -> ssize_t {
        (void) fd;
        (void) buf;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_write;

        return count;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(_n_write, 1);
}

#if SIZE_MAX > SSIZE_MAX
TEST_F(FileFdTest, WriteSuccessMaxSize)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_write = [](void *userdata, int fd, const void *buf, size_t count)
            -> ssize_t {
        (void) fd;
        (void) buf;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_write;

        return count;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, nullptr, static_cast<size_t>(SSIZE_MAX) + 1,
                            &n), MB_FILE_OK);
    ASSERT_EQ(n, SSIZE_MAX);
    ASSERT_EQ(_n_write, 1);
}
#endif

TEST_F(FileFdTest, WriteEof)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_write = [](void *userdata, int fd, const void *buf, size_t count)
            -> ssize_t {
        (void) fd;
        (void) buf;
        (void) count;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_write;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 0);
    ASSERT_EQ(_n_write, 1);
}

TEST_F(FileFdTest, WriteFailure)
{
    _vtable.fn_fstat = _fstat_file;

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_FAILED);
    ASSERT_EQ(_n_write, 1);
    ASSERT_EQ(mb_file_error(_file), -EIO);
}

TEST_F(FileFdTest, WriteFailureEINTR)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_write = [](void *userdata, int fd, const void *buf, size_t count)
            -> ssize_t {
        (void) fd;
        (void) buf;
        (void) count;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_write;

        errno = EINTR;
        return -1;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_RETRY);
    ASSERT_EQ(_n_write, 1);
    ASSERT_EQ(mb_file_error(_file), -EINTR);
}

TEST_F(FileFdTest, SeekSuccess)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_lseek64 = [](void *userdata, int fd, off64_t offset, int whence)
            -> off64_t {
        (void) fd;
        (void) offset;
        (void) whence;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_lseek64;

        return 10;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    uint64_t offset;
    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, &offset), MB_FILE_OK);
    ASSERT_EQ(offset, 10);
    ASSERT_EQ(_n_lseek64, 1);
}

#define LFS_SIZE (10ULL * 1024 * 1024 * 1024)
TEST_F(FileFdTest, SeekSuccessLargeFile)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_lseek64 = [](void *userdata, int fd, off64_t offset, int whence)
            -> off64_t {
        (void) fd;
        (void) offset;
        (void) whence;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_lseek64;

        return LFS_SIZE;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    // Ensure that the types (off_t, etc.) are large enough for LFS
    uint64_t offset;
    ASSERT_EQ(mb_file_seek(_file, LFS_SIZE, SEEK_SET, &offset), MB_FILE_OK);
    ASSERT_EQ(offset, LFS_SIZE);
    ASSERT_EQ(_n_lseek64, 1);
}
#undef LFS_SIZE

TEST_F(FileFdTest, SeekFseekFailed)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_lseek64 = [](void *userdata, int fd, off64_t offset, int whence)
            -> off64_t {
        (void) fd;
        (void) offset;
        (void) whence;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_lseek64;

        errno = EIO;
        return -1;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, nullptr), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    ASSERT_EQ(_n_lseek64, 1);
}

TEST_F(FileFdTest, TruncateSuccess)
{
    _vtable.fn_fstat = _fstat_file;

    _vtable.fn_ftruncate64 = [](void *userdata, int fd, off_t length) -> int {
        (void) fd;
        (void) length;

        FileFdTest *test = static_cast<FileFdTest *>(userdata);
        ++test->_n_ftruncate64;

        return 0;
    };

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_OK);
    ASSERT_EQ(_n_ftruncate64, 1);
}

TEST_F(FileFdTest, TruncateFailed)
{
    _vtable.fn_fstat = _fstat_file;

    ASSERT_EQ(_mb_file_open_fd(&_vtable, _file, 0, true), MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -EIO);
    ASSERT_EQ(_n_ftruncate64, 1);
}
