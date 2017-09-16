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

#include "mbcommon/file/fd.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/locale.h"

#include "mbcommon/file/fd_p.h"

#define DEFAULT_MODE \
    (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

/*!
 * \file mbcommon/file/fd.h
 * \brief Open file with POSIX file descriptors API
 */

namespace mb
{

/*! \cond INTERNAL */
struct RealFdFileFuncs : public FdFileFuncs
{
#ifdef _WIN32
    int fn_wopen(const wchar_t *path, int flags, mode_t mode) override
    {
        return _wopen(path, flags, mode);
    }
#else
    int fn_open(const char *path, int flags, mode_t mode) override
    {
        return open(path, flags, mode);
    }
#endif

    int fn_fstat(int fildes, struct stat *buf) override
    {
        return fstat(fildes, buf);
    }

    int fn_close(int fd) override
    {
        return close(fd);
    }

    int fn_ftruncate64(int fd, off_t length) override
    {
        return ftruncate64(fd, length);
    }

    off64_t fn_lseek64(int fd, off64_t offset, int whence) override
    {
        return lseek64(fd, offset, whence);
    }

    ssize_t fn_read(int fd, void *buf, size_t count) override
    {
        return read(fd, buf, count);
    }

    ssize_t fn_write(int fd, const void *buf, size_t count) override
    {
        return write(fd, buf, count);
    }
};
/*! \endcond */

static RealFdFileFuncs g_default_funcs;

/*! \cond INTERNAL */

FdFilePrivate::FdFilePrivate()
    : FdFilePrivate(&g_default_funcs)
{
}

FdFilePrivate::FdFilePrivate(FdFileFuncs *funcs)
    : funcs(funcs)
{
    clear();
}

FdFilePrivate::~FdFilePrivate()
{
}

void FdFilePrivate::clear()
{
    fd = -1;
    owned = false;
    filename.clear();
    flags = 0;
}

int FdFilePrivate::convert_mode(FileOpenMode mode)
{
    int ret = 0;

#ifdef _WIN32
    ret |= O_NOINHERIT | O_BINARY;
#else
    ret |= O_CLOEXEC;
#endif

    switch (mode) {
    case FileOpenMode::READ_ONLY:
        ret |= O_RDONLY;
        break;
    case FileOpenMode::READ_WRITE:
        ret |= O_RDWR;
        break;
    case FileOpenMode::WRITE_ONLY:
        ret |= O_WRONLY | O_CREAT | O_TRUNC;
        break;
    case FileOpenMode::READ_WRITE_TRUNC:
        ret |= O_RDWR | O_CREAT | O_TRUNC;
        break;
    case FileOpenMode::APPEND:
        ret |= O_WRONLY | O_CREAT | O_APPEND;
        break;
    case FileOpenMode::READ_APPEND:
        ret |= O_RDWR | O_CREAT | O_APPEND;
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

/*! \endcond */

/*!
 * \class FdFile
 *
 * \brief Open file using the file descriptor API on Unix-like systems.
 *
 * This class supports opening large files (64-bit offsets) on supported
 * Unix-like platforms, including Android.
 */

/*!
 * \brief Construct unbound FdFile.
 *
 * The File handle will not be bound to any file. One of the open functions will
 * need to be called to open a file.
 */
FdFile::FdFile()
    : FdFile(new FdFilePrivate())
{
}

/*!
 * \brief Open File handle from file descriptor.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(int, bool)
 *
 * \param fd File descriptor
 * \param owned Whether the file descriptor should be owned by the File handle
 */
FdFile::FdFile(int fd, bool owned)
    : FdFile(new FdFilePrivate(), fd, owned)
{
}

/*!
 * \brief Open File handle from a multi-byte filename.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(const std::string &, FileOpenMode)
 *
 * \param filename MBS filename
 * \param mode Open mode (\ref FileOpenMode)
 */
FdFile::FdFile(const std::string &filename, FileOpenMode mode)
    : FdFile(new FdFilePrivate(), filename, mode)
{
}

/*!
 * \brief Open File handle from a wide-character filename.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(const std::wstring &, FileOpenMode)
 *
 * \param filename WCS filename
 * \param mode Open mode (\ref FileOpenMode)
 */
FdFile::FdFile(const std::wstring &filename, FileOpenMode mode)
    : FdFile(new FdFilePrivate(), filename, mode)
{
}

/*! \cond INTERNAL */

FdFile::FdFile(FdFilePrivate *priv)
    : File(priv)
{
}

FdFile::FdFile(FdFilePrivate *priv,
               int fd, bool owned)
    : File(priv)
{
    open(fd, owned);
}

FdFile::FdFile(FdFilePrivate *priv,
               const std::string &filename, FileOpenMode mode)
    : File(priv)
{
    open(filename, mode);
}

FdFile::FdFile(FdFilePrivate *priv,
               const std::wstring &filename, FileOpenMode mode)
    : File(priv)
{
    open(filename, mode);
}

/*! \endcond */

FdFile::~FdFile()
{
    close();
}

/*!
 * \brief Open from file descriptor.
 *
 * If \p owned is true, then the File handle will take ownership of the file
 * descriptor. In other words, the file descriptor will be closed when the
 * File handle is closed.
 *
 * \param fd File descriptor
 * \param owned Whether the file descriptor should be owned by the File handle
 *
 * \return Whether the file is successfully opened
 */
bool FdFile::open(int fd, bool owned)
{
    MB_PRIVATE(FdFile);
    if (priv) {
        priv->fd = fd;
        priv->owned = owned;
    }
    return File::open();
}

/*!
 * \brief Open from a multi-byte filename.
 *
 * On Unix-like systems, \p filename is directly passed to `open()`. On Windows
 * systems, \p filename is converted to WCS using mbs_to_wcs() before being
 * passed to `_wopen()`.
 *
 * \param filename MBS filename
 * \param mode Open mode (\ref FileOpenMode)
 *
 * \return Whether the file is successfully opened
 */
bool FdFile::open(const std::string &filename, FileOpenMode mode)
{
    MB_PRIVATE(FdFile);
    if (priv) {
        // Convert filename to platform-native encoding
#ifdef _WIN32
        std::wstring native_filename;
        if (!mbs_to_wcs(native_filename, filename)) {
            set_error(make_error_code(FileError::CannotConvertEncoding),
                      "Failed to convert MBS filename to WCS");
            return false;
        }
#else
        auto native_filename = filename;
#endif

        // Convert mode to flags
        int flags = priv->convert_mode(mode);
        if (flags < 0) {
            set_error(make_error_code(FileError::InvalidMode),
                      "Invalid mode: %d", static_cast<int>(mode));
            return false;
        }

        priv->fd = -1;
        priv->owned = true;
        priv->filename = std::move(native_filename);
        priv->flags = flags;
    }
    return File::open();
}

/*!
 * \brief Open from a wide-character filename.
 *
 * On Unix-like systems, \p filename is converted to MBS using wcs_to_mbs()
 * before being passed to `open()`. On Windows systems, \p filename is directly
 * passed to `_wopen()`.
 *
 * \param filename WCS filename
 * \param mode Open mode (\ref FileOpenMode)
 *
 * \return Whether the file is successfully opened
 */
bool FdFile::open(const std::wstring &filename, FileOpenMode mode)
{
    MB_PRIVATE(FdFile);
    if (priv) {
        // Convert filename to platform-native encoding
#ifdef _WIN32
        auto native_filename = filename;
#else
        std::string native_filename;
        if (!wcs_to_mbs(native_filename, filename)) {
            set_error(make_error_code(FileError::CannotConvertEncoding),
                      "Failed to convert WCS filename to MBS");
            return false;
        }
#endif

        // Convert mode to flags
        int flags = priv->convert_mode(mode);
        if (flags < 0) {
            set_error(make_error_code(FileError::InvalidMode),
                      "Invalid mode: %d", static_cast<int>(mode));
            return false;
        }

        priv->fd = -1;
        priv->owned = true;
        priv->filename = std::move(native_filename);
        priv->flags = flags;
    }
    return File::open();
}

bool FdFile::on_open()
{
    MB_PRIVATE(FdFile);

    if (!priv->filename.empty()) {
#ifdef _WIN32
        priv->fd = priv->funcs->fn_wopen(
#else
        priv->fd = priv->funcs->fn_open(
#endif
                priv->filename.c_str(), priv->flags, DEFAULT_MODE);
        if (priv->fd < 0) {
            set_error(std::error_code(errno, std::generic_category()),
                      "Failed to open file");
            return false;
        }
    }

    struct stat sb;

    if (priv->funcs->fn_fstat(priv->fd, &sb) < 0) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to stat file");
        return false;
    }

    if (S_ISDIR(sb.st_mode)) {
        set_error(std::make_error_code(std::errc::is_a_directory),
                  "Failed to open file");
        return false;
    }

    return true;
}

bool FdFile::on_close()
{
    MB_PRIVATE(FdFile);

    bool ret = true;

    if (priv->owned && priv->fd >= 0 && priv->funcs->fn_close(priv->fd) < 0) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to close file");
        ret = false;
    }

    // Reset to allow opening another file
    priv->clear();

    return ret;
}

bool FdFile::on_read(void *buf, size_t size, size_t &bytes_read)
{
    MB_PRIVATE(FdFile);

    if (size > SSIZE_MAX) {
        size = SSIZE_MAX;
    }

    ssize_t n = priv->funcs->fn_read(priv->fd, buf, size);
    if (n < 0) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to read file");
        return false;
    }

    bytes_read = static_cast<size_t>(n);
    return true;
}

bool FdFile::on_write(const void *buf, size_t size, size_t &bytes_written)
{
    MB_PRIVATE(FdFile);

    if (size > SSIZE_MAX) {
        size = SSIZE_MAX;
    }

    ssize_t n = priv->funcs->fn_write(priv->fd, buf, size);
    if (n < 0) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to write file");
        return false;
    }

    bytes_written = static_cast<size_t>(n);
    return true;
}

bool FdFile::on_seek(int64_t offset, int whence, uint64_t &new_offset)
{
    MB_PRIVATE(FdFile);

    off64_t ret = priv->funcs->fn_lseek64(priv->fd, offset, whence);
    if (ret < 0) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to seek file");
        return false;
    }

    new_offset = static_cast<uint64_t>(ret);
    return true;
}

bool FdFile::on_truncate(uint64_t size)
{
    MB_PRIVATE(FdFile);

    if (priv->funcs->fn_ftruncate64(priv->fd, static_cast<off64_t>(size)) < 0) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to truncate file");
        return false;
    }

    return true;
}

}
