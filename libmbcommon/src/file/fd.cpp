/*
 * Copyright (C) 2016-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/error_code.h"
#include "mbcommon/file_error.h"
#include "mbcommon/finally.h"
#include "mbcommon/locale.h"

/*!
 * \file mbcommon/file/fd.h
 * \brief Open file with POSIX file descriptors API
 */

namespace mb
{

using namespace detail;

static constexpr mode_t DEFAULT_MODE =
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

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

    int fn_ftruncate64(int fd, off64_t length) override
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

FdFileFuncs::~FdFileFuncs() = default;

static int convert_mode(FileOpenMode mode)
{
    int ret = 0;

#ifdef _WIN32
    ret |= O_NOINHERIT | O_BINARY;
#else
    ret |= O_CLOEXEC;
#endif

    switch (mode) {
    case FileOpenMode::ReadOnly:
        ret |= O_RDONLY;
        break;
    case FileOpenMode::ReadWrite:
        ret |= O_RDWR;
        break;
    case FileOpenMode::WriteOnly:
        ret |= O_WRONLY | O_CREAT | O_TRUNC;
        break;
    case FileOpenMode::ReadWriteTrunc:
        ret |= O_RDWR | O_CREAT | O_TRUNC;
        break;
    case FileOpenMode::Append:
        ret |= O_WRONLY | O_CREAT | O_APPEND;
        break;
    case FileOpenMode::ReadAppend:
        ret |= O_RDWR | O_CREAT | O_APPEND;
        break;
    default:
        MB_UNREACHABLE("Invalid mode: %d", static_cast<int>(mode));
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
    : FdFile(&g_default_funcs)
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
    : FdFile(&g_default_funcs, fd, owned)
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
    : FdFile(&g_default_funcs, filename, mode)
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
    : FdFile(&g_default_funcs, filename, mode)
{
}

/*! \cond INTERNAL */

FdFile::FdFile(FdFileFuncs *funcs)
    : File(), m_funcs(funcs)
{
    clear();
}

FdFile::FdFile(FdFileFuncs *funcs,
               int fd, bool owned)
    : FdFile(funcs)
{
    (void) open(fd, owned);
}

FdFile::FdFile(FdFileFuncs *funcs,
               const std::string &filename, FileOpenMode mode)
    : FdFile(funcs)
{
    (void) open(filename, mode);
}

FdFile::FdFile(FdFileFuncs *funcs,
               const std::wstring &filename, FileOpenMode mode)
    : FdFile(funcs)
{
    (void) open(filename, mode);
}

/*! \endcond */

FdFile::~FdFile()
{
    (void) close();
}

/*!
 * \brief Move construct new File handle.
 *
 * \p other will be left in a state as if it was newly constructed with the
 * default constructor.
 *
 * \param other File handle to move from
 */
FdFile::FdFile(FdFile &&other) noexcept
{
    clear();

    std::swap(m_funcs, other.m_funcs);
    std::swap(m_fd, other.m_fd);
    std::swap(m_owned, other.m_owned);
    std::swap(m_filename, other.m_filename);
    std::swap(m_flags, other.m_flags);
}

/*!
 * \brief Move assign a File handle
 *
 * This file handle will be closed and then \p rhs will be moved into this
 * object. \p rhs will be left in a state as if it was newly constructed with
 * the default constructor.
 *
 * \param rhs File handle to move from
 */
FdFile & FdFile::operator=(FdFile &&rhs) noexcept
{
    if (this != &rhs) {
        (void) close();

        std::swap(m_funcs, rhs.m_funcs);
        std::swap(m_fd, rhs.m_fd);
        std::swap(m_owned, rhs.m_owned);
        std::swap(m_filename, rhs.m_filename);
        std::swap(m_flags, rhs.m_flags);
    }

    return *this;
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
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> FdFile::open(int fd, bool owned)
{
    if (is_open()) return FileError::InvalidState;

    m_fd = fd;
    m_owned = owned;

    return open();
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
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> FdFile::open(const std::string &filename, FileOpenMode mode)
{
    if (is_open()) return FileError::InvalidState;

    // Convert filename to platform-native encoding
#ifdef _WIN32
    auto converted = mbs_to_wcs(filename);
    if (!converted) {
        return FileError::CannotConvertEncoding;
    }
    auto &&native_filename = converted.value();
#else
    auto &&native_filename = filename;
#endif

    m_fd = -1;
    m_owned = true;
    m_filename = std::move(native_filename);
    m_flags = convert_mode(mode);

    return open();
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
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> FdFile::open(const std::wstring &filename, FileOpenMode mode)
{
    if (is_open()) return FileError::InvalidState;

    // Convert filename to platform-native encoding
#ifdef _WIN32
    auto &&native_filename = filename;
#else
    auto converted = wcs_to_mbs(filename);
    if (!converted) {
        return FileError::CannotConvertEncoding;
    }
    auto &&native_filename = converted.value();
#endif

    m_fd = -1;
    m_owned = true;
    m_filename = std::move(native_filename);
    m_flags = convert_mode(mode);

    return open();
}

oc::result<void> FdFile::open()
{
    auto reset = finally([&] {
        (void) close();
    });

    if (!m_filename.empty()) {
#ifdef _WIN32
        m_fd = m_funcs->fn_wopen(
#else
        m_fd = m_funcs->fn_open(
#endif
                m_filename.c_str(), m_flags, DEFAULT_MODE);
        if (m_fd < 0) {
            return ec_from_errno();
        }
    }

    struct stat sb;

    if (m_funcs->fn_fstat(m_fd, &sb) < 0) {
        return ec_from_errno();
    }

    if (S_ISDIR(sb.st_mode)) {
        return std::make_error_code(std::errc::is_a_directory);
    }

    reset.dismiss();

    return oc::success();
}

oc::result<void> FdFile::close()
{
    if (!is_open()) return FileError::InvalidState;

    // Reset to allow opening another file
    auto reset = finally([&] {
        clear();
    });

    if (m_owned && m_fd >= 0 && m_funcs->fn_close(m_fd) < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

oc::result<size_t> FdFile::read(void *buf, size_t size)
{
    if (!is_open()) return FileError::InvalidState;

    if (size > SSIZE_MAX) {
        size = SSIZE_MAX;
    }

    ssize_t n = m_funcs->fn_read(m_fd, buf, size);
    if (n < 0) {
        return ec_from_errno();
    }

    return static_cast<size_t>(n);
}

oc::result<size_t> FdFile::write(const void *buf, size_t size)
{
    if (!is_open()) return FileError::InvalidState;

    if (size > SSIZE_MAX) {
        size = SSIZE_MAX;
    }

    ssize_t n = m_funcs->fn_write(m_fd, buf, size);
    if (n < 0) {
        return ec_from_errno();
    }

    return static_cast<size_t>(n);
}

oc::result<uint64_t> FdFile::seek(int64_t offset, int whence)
{
    if (!is_open()) return FileError::InvalidState;

    off64_t ret = m_funcs->fn_lseek64(m_fd, offset, whence);
    if (ret < 0) {
        return ec_from_errno();
    }

    return static_cast<uint64_t>(ret);
}

oc::result<void> FdFile::truncate(uint64_t size)
{
    if (!is_open()) return FileError::InvalidState;

    if (m_funcs->fn_ftruncate64(m_fd, static_cast<off64_t>(size)) < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

bool FdFile::is_open()
{
    return m_fd >= 0;
}

void FdFile::clear() noexcept
{
    m_fd = -1;
    m_owned = false;
    m_filename.clear();
    m_flags = 0;
}

}
