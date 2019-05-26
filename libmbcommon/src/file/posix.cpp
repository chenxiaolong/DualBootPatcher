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

#include "mbcommon/file/posix.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbcommon/file_error.h"
#include "mbcommon/finally.h"
#include "mbcommon/locale.h"

#ifndef __ANDROID__
static_assert(sizeof(off_t) > 4, "Not compiling with LFS support!");
#endif

/*!
 * \file mbcommon/file/posix.h
 * \brief Open file with POSIX stdio `FILE *` API
 */

namespace mb
{

using namespace detail;

/*! \cond INTERNAL */
struct RealPosixFileFuncs : public PosixFileFuncs
{
    int fn_fstat(int fildes, struct stat *buf) override
    {
        return fstat(fildes, buf);
    }

    int fn_fclose(FILE *stream) override
    {
        return fclose(stream);
    }

    int fn_ferror(FILE *stream) override
    {
        return ferror(stream);
    }

    int fn_fileno(FILE *stream) override
    {
        return fileno(stream);
    }

#ifdef _WIN32
    FILE * fn_wfopen(const wchar_t *filename, const wchar_t *mode) override
    {
        return _wfopen(filename, mode);
    }
#else
    FILE * fn_fopen(const char *path, const char *mode) override
    {
        return fopen(path, mode);
    }
#endif

    size_t fn_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) override
    {
        return fread(ptr, size, nmemb, stream);
    }

    int fn_fseeko(FILE *stream, off_t offset, int whence) override
    {
        return fseeko(stream, offset, whence);
    }

    off_t fn_ftello(FILE *stream) override
    {
        return ftello(stream);
    }

    size_t fn_fwrite(const void *ptr, size_t size, size_t nmemb,
                     FILE *stream) override
    {
        return fwrite(ptr, size, nmemb, stream);
    }

    int fn_ftruncate64(int fd, off64_t length) override
    {
        return ftruncate64(fd, length);
    }
};
/*! \endcond */

static RealPosixFileFuncs g_default_funcs;

/*! \cond INTERNAL */

PosixFileFuncs::~PosixFileFuncs() = default;

#ifdef _WIN32
static const wchar_t * convert_mode(FileOpenMode mode)
{
    switch (mode) {
    case FileOpenMode::ReadOnly:
        return L"rbN";
    case FileOpenMode::ReadWrite:
        return L"r+bN";
    case FileOpenMode::WriteOnly:
        return L"wbN";
    case FileOpenMode::ReadWriteTrunc:
        return L"w+bN";
    case FileOpenMode::Append:
        return L"abN";
    case FileOpenMode::ReadAppend:
        return L"a+bN";
    default:
        MB_UNREACHABLE("Invalid mode: %d", static_cast<int>(mode));
    }
}
#else
static const char * convert_mode(FileOpenMode mode)
{
    switch (mode) {
    case FileOpenMode::ReadOnly:
        return "rbe";
    case FileOpenMode::ReadWrite:
        return "r+be";
    case FileOpenMode::WriteOnly:
        return "wbe";
    case FileOpenMode::ReadWriteTrunc:
        return "w+be";
    case FileOpenMode::Append:
        return "abe";
    case FileOpenMode::ReadAppend:
        return "a+be";
    default:
        MB_UNREACHABLE("Invalid mode: %d", static_cast<int>(mode));
    }
}
#endif

/*! \endcond */

/*!
 * \class PosixFile
 *
 * \brief Open file using posix `FILE *` API.
 *
 * This class supports opening large files (64-bit offsets) on non-Android
 * Unix-like platforms.
 */

/*!
 * \brief Construct unbound PosixFile.
 *
 * The File handle will not be bound to any file. One of the open functions will
 * need to be called to open a file.
 */
PosixFile::PosixFile()
    : PosixFile(&g_default_funcs)
{
}

/*!
 * \brief Open File handle from `FILE *`.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(FILE *, bool)
 *
 * \param fp `FILE *` instance
 * \param owned Whether the `FILE *` instance should be owned by the File
 *              handle
 */
PosixFile::PosixFile(FILE *fp, bool owned)
    : PosixFile(&g_default_funcs, fp, owned)
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
PosixFile::PosixFile(const std::string &filename, FileOpenMode mode)
    : PosixFile(&g_default_funcs, filename, mode)
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
PosixFile::PosixFile(const std::wstring &filename, FileOpenMode mode)
    : PosixFile(&g_default_funcs, filename, mode)
{
}

/*! \cond INTERNAL */

PosixFile::PosixFile(PosixFileFuncs *funcs)
    : File(), m_funcs(funcs)
{
    clear();
}

PosixFile::PosixFile(PosixFileFuncs *funcs,
                     FILE *fp, bool owned)
    : PosixFile(funcs)
{
    (void) open(fp, owned);
}

PosixFile::PosixFile(PosixFileFuncs *funcs,
                     const std::string &filename, FileOpenMode mode)
    : PosixFile(funcs)
{
    (void) open(filename, mode);
}

PosixFile::PosixFile(PosixFileFuncs *funcs,
                     const std::wstring &filename, FileOpenMode mode)
    : PosixFile(funcs)
{
    (void) open(filename, mode);
}

/*! \endcond */

PosixFile::~PosixFile()
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
PosixFile::PosixFile(PosixFile &&other) noexcept
{
    clear();

    std::swap(m_funcs, other.m_funcs);
    std::swap(m_fp, other.m_fp);
    std::swap(m_owned, other.m_owned);
    std::swap(m_filename, other.m_filename);
    std::swap(m_mode, other.m_mode);
    std::swap(m_can_seek, other.m_can_seek);
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
PosixFile & PosixFile::operator=(PosixFile &&rhs) noexcept
{
    if (this != &rhs) {
        (void) close();

        std::swap(m_funcs, rhs.m_funcs);
        std::swap(m_fp, rhs.m_fp);
        std::swap(m_owned, rhs.m_owned);
        std::swap(m_filename, rhs.m_filename);
        std::swap(m_mode, rhs.m_mode);
        std::swap(m_can_seek, rhs.m_can_seek);
    }

    return *this;
}

/*!
 * \brief Open from `FILE *`.
 *
 * If \p owned is true, then the File handle will take ownership of the
 * `FILE *` instance. In other words, the `FILE *` instance will be closed when
 * the File handle is closed.
 *
 * \param fp `FILE *` instance
 * \param owned Whether the `FILE *` instance should be owned by the File
 *              handle
 *
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> PosixFile::open(FILE *fp, bool owned)
{
    if (is_open()) return FileError::InvalidState;

    m_fp = fp;
    m_owned = owned;

    return open();
}

/*!
 * \brief Open from a multi-byte filename.
 *
 * On Unix-like systems, \p filename is directly passed to `fopen()`. On Windows
 * systems, \p filename is converted to WCS using mbs_to_wcs() before being
 * passed to `_wfopen()`.
 *
 * \param filename MBS filename
 * \param mode Open mode (\ref FileOpenMode)
 *
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> PosixFile::open(const std::string &filename, FileOpenMode mode)
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

    m_fp = nullptr;
    m_owned = true;
    m_filename = std::move(native_filename);
    m_mode = convert_mode(mode);

    return open();
}

/*!
 * \brief Open from a wide-character filename.
 *
 * On Unix-like systems, \p filename is converted to MBS using wcs_to_mbs()
 * before being passed to `fopen()`. On Windows systems, \p filename is directly
 * passed to `_wfopen()`.
 *
 * \param filename WCS filename
 * \param mode Open mode (\ref FileOpenMode)
 *
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> PosixFile::open(const std::wstring &filename, FileOpenMode mode)
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

    m_fp = nullptr;
    m_owned = true;
    m_filename = std::move(native_filename);
    m_mode = convert_mode(mode);

    return open();
}

oc::result<void> PosixFile::open()
{
    auto reset = finally([&] {
        (void) close();
    });

    if (!m_filename.empty()) {
#ifdef _WIN32
        m_fp = m_funcs->fn_wfopen(
#else
        m_fp = m_funcs->fn_fopen(
#endif
                m_filename.c_str(), m_mode);
        if (!m_fp) {
            return ec_from_errno();
        }
    }

    // Assume file is unseekable by default
    m_can_seek = false;

    int fd = m_funcs->fn_fileno(m_fp);
    if (fd >= 0) {
        struct stat sb;

        if (m_funcs->fn_fstat(fd, &sb) < 0) {
            return ec_from_errno();
        }

        if (S_ISDIR(sb.st_mode)) {
            return std::make_error_code(std::errc::is_a_directory);
        }

        // Enable seekability based on file type because lseek(fd, 0, SEEK_CUR)
        // does not always fail on non-seekable file descriptors
        if (S_ISREG(sb.st_mode)
#ifdef __linux__
                || S_ISBLK(sb.st_mode)
#endif
        ) {
            m_can_seek = true;
        }
    }

    reset.dismiss();

    return oc::success();
}

oc::result<void> PosixFile::close()
{
    if (!is_open()) return FileError::InvalidState;

    // Reset to allow opening another file
    auto reset = finally([&] {
        clear();
    });

    if (m_owned && m_fp && m_funcs->fn_fclose(m_fp) == EOF) {
        return ec_from_errno();
    }

    return oc::success();
}

oc::result<size_t> PosixFile::read(void *buf, size_t size)
{
    if (!is_open()) return FileError::InvalidState;

    size_t n = m_funcs->fn_fread(buf, 1, size, m_fp);

    if (n < size && m_funcs->fn_ferror(m_fp)) {
        return ec_from_errno();
    }

    return n;
}

oc::result<size_t> PosixFile::write(const void *buf, size_t size)
{
    if (!is_open()) return FileError::InvalidState;

    size_t n = m_funcs->fn_fwrite(buf, 1, size, m_fp);

    if (n < size && m_funcs->fn_ferror(m_fp)) {
        return ec_from_errno();
    }

    return n;
}

oc::result<uint64_t> PosixFile::seek(int64_t offset, int whence)
{
    if (!is_open()) return FileError::InvalidState;

    if (!m_can_seek) {
        return FileError::UnsupportedSeek;
    }

    // Try to seek
    if (m_funcs->fn_fseeko(m_fp, static_cast<off_t>(offset), whence) < 0) {
        return ec_from_errno();
    }

    // Get new position
    off_t new_pos = m_funcs->fn_ftello(m_fp);
    if (new_pos < 0) {
        return ec_from_errno();
    }

    return static_cast<uint64_t>(new_pos);
}

oc::result<void> PosixFile::truncate(uint64_t size)
{
    if (!is_open()) return FileError::InvalidState;

    int fd = m_funcs->fn_fileno(m_fp);
    if (fd < 0) {
        // fileno() not supported for fp
        return FileError::UnsupportedTruncate;
    }

    if (m_funcs->fn_ftruncate64(fd, static_cast<off64_t>(size)) < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

bool PosixFile::is_open()
{
    return m_fp;
}

void PosixFile::clear() noexcept
{
    m_fp = nullptr;
    m_owned = false;
    m_filename.clear();
    m_mode = nullptr;
    m_can_seek = false;
}

}
