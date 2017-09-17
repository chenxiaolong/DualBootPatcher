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

#include "mbcommon/file/posix.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/locale.h"

#include "mbcommon/file/posix_p.h"

#ifndef __ANDROID__
static_assert(sizeof(off_t) > 4, "Not compiling with LFS support!");
#endif

/*!
 * \file mbcommon/file/posix.h
 * \brief Open file with POSIX stdio `FILE *` API
 */

namespace mb
{

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

    int fn_ftruncate64(int fd, off_t length) override
    {
        return ftruncate64(fd, length);
    }
};
/*! \endcond */

static RealPosixFileFuncs g_default_funcs;

/*! \cond INTERNAL */

PosixFileFuncs::~PosixFileFuncs() = default;

PosixFilePrivate::PosixFilePrivate()
    : PosixFilePrivate(&g_default_funcs)
{
}

PosixFilePrivate::PosixFilePrivate(PosixFileFuncs *funcs)
    : funcs(funcs)
{
    clear();
}

PosixFilePrivate::~PosixFilePrivate() = default;

void PosixFilePrivate::clear()
{
    fp = nullptr;
    owned = false;
    filename.clear();
    mode = nullptr;
    can_seek = false;
}

#ifdef _WIN32
const wchar_t * PosixFilePrivate::convert_mode(FileOpenMode mode)
{
    switch (mode) {
    case FileOpenMode::READ_ONLY:
        return L"rbN";
    case FileOpenMode::READ_WRITE:
        return L"r+bN";
    case FileOpenMode::WRITE_ONLY:
        return L"wbN";
    case FileOpenMode::READ_WRITE_TRUNC:
        return L"w+bN";
    case FileOpenMode::APPEND:
        return L"abN";
    case FileOpenMode::READ_APPEND:
        return L"a+bN";
    default:
        return nullptr;
    }
}
#else
const char * PosixFilePrivate::convert_mode(FileOpenMode mode)
{
    switch (mode) {
    case FileOpenMode::READ_ONLY:
        return "rbe";
    case FileOpenMode::READ_WRITE:
        return "r+be";
    case FileOpenMode::WRITE_ONLY:
        return "wbe";
    case FileOpenMode::READ_WRITE_TRUNC:
        return "w+be";
    case FileOpenMode::APPEND:
        return "abe";
    case FileOpenMode::READ_APPEND:
        return "a+be";
    default:
        return nullptr;
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
    : PosixFile(new PosixFilePrivate())
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
    : PosixFile(new PosixFilePrivate(), fp, owned)
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
    : PosixFile(new PosixFilePrivate(), filename, mode)
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
    : PosixFile(new PosixFilePrivate(), filename, mode)
{
}

/*! \cond INTERNAL */

PosixFile::PosixFile(PosixFilePrivate *priv)
    : File(priv)
{
}

PosixFile::PosixFile(PosixFilePrivate *priv,
                     FILE *fp, bool owned)
    : File(priv)
{
    open(fp, owned);
}

PosixFile::PosixFile(PosixFilePrivate *priv,
                     const std::string &filename, FileOpenMode mode)
    : File(priv)
{
    open(filename, mode);
}

PosixFile::PosixFile(PosixFilePrivate *priv,
                     const std::wstring &filename, FileOpenMode mode)
    : File(priv)
{
    open(filename, mode);
}

/*! \endcond */

PosixFile::~PosixFile()
{
    close();
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
 * \return Whether the file is successfully opened
 */
bool PosixFile::open(FILE *fp, bool owned)
{
    MB_PRIVATE(PosixFile);
    if (priv) {
        priv->fp = fp;
        priv->owned = owned;
    }
    return File::open();
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
 * \return Whether the file is successfully opened
 */
bool PosixFile::open(const std::string &filename, FileOpenMode mode)
{
    MB_PRIVATE(PosixFile);
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

        // Convert mode to fopen-compatible mode string
        auto mode_str = priv->convert_mode(mode);
        if (!mode_str) {
            set_error(make_error_code(FileError::InvalidMode),
                      "Invalid mode: %d", static_cast<int>(mode));
            return false;
        }

        priv->fp = nullptr;
        priv->owned = true;
        priv->filename = std::move(native_filename);
        priv->mode = mode_str;
    }
    return File::open();
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
 * \return Whether the file is successfully opened
 */
bool PosixFile::open(const std::wstring &filename, FileOpenMode mode)
{
    MB_PRIVATE(PosixFile);
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

        // Convert mode to fopen-compatible mode string
        auto mode_str = priv->convert_mode(mode);
        if (!mode_str) {
            set_error(make_error_code(FileError::InvalidMode),
                      "Invalid mode: %d", static_cast<int>(mode));
            return false;
        }

        priv->fp = nullptr;
        priv->owned = true;
        priv->filename = std::move(native_filename);
        priv->mode = mode_str;
    }
    return File::open();
}

bool PosixFile::on_open()
{
    MB_PRIVATE(PosixFile);

    if (!priv->filename.empty()) {
#ifdef _WIN32
        priv->fp = priv->funcs->fn_wfopen(
#else
        priv->fp = priv->funcs->fn_fopen(
#endif
                priv->filename.c_str(), priv->mode);
        if (!priv->fp) {
            set_error(std::error_code(errno, std::generic_category()),
                      "Failed to open file");
            return false;
        }
    }

    struct stat sb;
    int fd;

    // Assume file is unseekable by default
    priv->can_seek = false;

    fd = priv->funcs->fn_fileno(priv->fp);
    if (fd >= 0) {
        if (priv->funcs->fn_fstat(fd, &sb) < 0) {
            set_error(std::error_code(errno, std::generic_category()),
                      "Failed to stat file");
            return false;
        }

        if (S_ISDIR(sb.st_mode)) {
            set_error(std::make_error_code(std::errc::is_a_directory),
                      "Failed to open file");
            return false;
        }

        // Enable seekability based on file type because lseek(fd, 0, SEEK_CUR)
        // does not always fail on non-seekable file descriptors
        if (S_ISREG(sb.st_mode)
#ifdef __linux__
                || S_ISBLK(sb.st_mode)
#endif
        ) {
            priv->can_seek = true;
        }
    }

    return true;
}

bool PosixFile::on_close()
{
    MB_PRIVATE(PosixFile);

    bool ret = true;

    if (priv->owned && priv->fp && priv->funcs->fn_fclose(priv->fp) == EOF) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to close file");
        ret = false;
    }

    // Reset to allow opening another file
    priv->clear();

    return ret;
}

bool PosixFile::on_read(void *buf, size_t size, size_t &bytes_read)
{
    MB_PRIVATE(PosixFile);

    size_t n = priv->funcs->fn_fread(buf, 1, size, priv->fp);

    if (n < size && priv->funcs->fn_ferror(priv->fp)) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to read file");
        return false;
    }

    bytes_read = n;
    return true;
}

bool PosixFile::on_write(const void *buf, size_t size, size_t &bytes_written)
{
    MB_PRIVATE(PosixFile);

    size_t n = priv->funcs->fn_fwrite(buf, 1, size, priv->fp);

    if (n < size && priv->funcs->fn_ferror(priv->fp)) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to write file");
        return false;
    }

    bytes_written = n;
    return true;
}

bool PosixFile::on_seek(int64_t offset, int whence, uint64_t &new_offset)
{
    MB_PRIVATE(PosixFile);

    off64_t old_pos, new_pos;

    if (!priv->can_seek) {
        set_error(make_error_code(FileError::UnsupportedSeek),
                  "Seek not supported");
        return false;
    }

    // Get current file position
    old_pos = priv->funcs->fn_ftello(priv->fp);
    if (old_pos < 0) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to get file position");
        return false;
    }

    // Try to seek
    if (priv->funcs->fn_fseeko(priv->fp, offset, whence) < 0) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to seek file");
        return false;
    }

    // Get new position
    new_pos = priv->funcs->fn_ftello(priv->fp);
    if (new_pos < 0) {
        // Try to restore old position
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to get file position");
        if (priv->funcs->fn_fseeko(priv->fp, old_pos, SEEK_SET) != 0) {
            set_fatal(true);
        }
        return false;
    }

    new_offset = static_cast<uint64_t>(new_pos);
    return true;
}

bool PosixFile::on_truncate(uint64_t size)
{
    MB_PRIVATE(PosixFile);

    int fd = priv->funcs->fn_fileno(priv->fp);
    if (fd < 0) {
        set_error(make_error_code(FileError::UnsupportedTruncate),
                  "fileno() not supported for fp");
        return false;
    }

    if (priv->funcs->fn_ftruncate64(fd, static_cast<off64_t>(size)) < 0) {
        set_error(std::error_code(errno, std::generic_category()),
                  "Failed to truncate file");
        return false;
    }

    return true;
}

}
