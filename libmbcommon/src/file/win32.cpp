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

#include "mbcommon/file/win32.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

#include "mbcommon/locale.h"

#include "mbcommon/file/win32_p.h"

static_assert(sizeof(DWORD) == 4, "DWORD is not 32 bits");

#define GET_LAST_ERROR() \
    static_cast<int>(GetLastError())
#define WIN32_ERROR_CODE() \
    std::error_code(GET_LAST_ERROR(), std::system_category())

/*!
 * \file mbcommon/file/win32.h
 * \brief Open file with Win32 `HANDLE` API
 */

namespace mb
{

/*! \cond INTERNAL */
struct RealWin32FileFuncs : public Win32FileFuncs
{
    // windows.h
    virtual BOOL fn_CloseHandle(HANDLE hObject) override
    {
        return CloseHandle(hObject);
    }

    virtual HANDLE fn_CreateFileW(LPCWSTR lpFileName,
                                  DWORD dwDesiredAccess,
                                  DWORD dwShareMode,
                                  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                  DWORD dwCreationDisposition,
                                  DWORD dwFlagsAndAttributes,
                                  HANDLE hTemplateFile) override
    {
        return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
                           lpSecurityAttributes, dwCreationDisposition,
                           dwFlagsAndAttributes, hTemplateFile);
    }

    virtual BOOL fn_ReadFile(HANDLE hFile,
                             LPVOID lpBuffer,
                             DWORD nNumberOfBytesToRead,
                             LPDWORD lpNumberOfBytesRead,
                             LPOVERLAPPED lpOverlapped) override
    {
        return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead,
                        lpNumberOfBytesRead, lpOverlapped);
    }

    virtual BOOL fn_SetEndOfFile(HANDLE hFile) override
    {
        return SetEndOfFile(hFile);
    }

    virtual BOOL fn_SetFilePointerEx(HANDLE hFile,
                                     LARGE_INTEGER liDistanceToMove,
                                     PLARGE_INTEGER lpNewFilePointer,
                                     DWORD dwMoveMethod) override
    {
        return SetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer,
                                dwMoveMethod);
    }

    virtual BOOL fn_WriteFile(HANDLE hFile,
                              LPCVOID lpBuffer,
                              DWORD nNumberOfBytesToWrite,
                              LPDWORD lpNumberOfBytesWritten,
                              LPOVERLAPPED lpOverlapped) override
    {
        return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
                         lpNumberOfBytesWritten, lpOverlapped);
    }
};
/*! \endcond */

static RealWin32FileFuncs g_default_funcs;

/*! \cond INTERNAL */

Win32FilePrivate::Win32FilePrivate()
    : Win32FilePrivate(&g_default_funcs)
{
}

Win32FilePrivate::Win32FilePrivate(Win32FileFuncs *funcs)
    : funcs(funcs), error(nullptr)
{
    clear();
}

Win32FilePrivate::~Win32FilePrivate()
{
    LocalFree(error);
}

void Win32FilePrivate::clear()
{
    handle = INVALID_HANDLE_VALUE;
    owned = false;
    filename.clear();
    access = 0;
    sharing = 0;
    sa = {};
    creation = 0;
    attrib = 0;
    append = false;
}

bool Win32FilePrivate::convert_mode(FileOpenMode mode,
                                    DWORD &access_out,
                                    DWORD &sharing_out,
                                    SECURITY_ATTRIBUTES &sa_out,
                                    DWORD &creation_out,
                                    DWORD &attrib_out,
                                    bool &append_out)
{
    DWORD access = 0;
    // Match open()/_wopen() behavior
    DWORD sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
    SECURITY_ATTRIBUTES sa;
    DWORD creation = 0;
    DWORD attrib = 0;
    // Win32 does not have a native append mode
    bool append = false;

    switch (mode) {
    case FileOpenMode::READ_ONLY:
        access = GENERIC_READ;
        creation = OPEN_EXISTING;
        break;
    case FileOpenMode::READ_WRITE:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = OPEN_EXISTING;
        break;
    case FileOpenMode::WRITE_ONLY:
        access = GENERIC_WRITE;
        creation = CREATE_ALWAYS;
        break;
    case FileOpenMode::READ_WRITE_TRUNC:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = CREATE_ALWAYS;
        break;
    case FileOpenMode::APPEND:
        access = GENERIC_WRITE;
        creation = OPEN_ALWAYS;
        append = true;
        break;
    case FileOpenMode::READ_APPEND:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = OPEN_ALWAYS;
        append = true;
        break;
    default:
        return false;
    }

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = false;

    access_out = access;
    sharing_out = sharing;
    sa_out = sa;
    creation_out = creation;
    attrib_out = attrib;
    append_out = append;

    return true;
}

/*! \endcond */

/*!
 * \class Win32File
 *
 * \brief Open file using Win32 API.
 *
 * This class supports opening large files (64-bit offsets) on Windows.
 */

/*!
 * \brief Construct unbound Win32File.
 *
 * The File handle will not be bound to any file. One of the open functions will
 * need to be called to open a file.
 */
Win32File::Win32File()
    : Win32File(new Win32FilePrivate())
{
}

/*!
 * \brief Open File handle from Win32 `HANDLE`.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(HANDLE, bool, bool)
 *
 * \param handle Win32 `HANDLE`
 * \param owned Whether the Win32 `HANDLE` should be owned by the File handle
 * \param append Whether append mode should be enabled
 */
Win32File::Win32File(HANDLE handle, bool owned, bool append)
    : Win32File(new Win32FilePrivate(), handle, owned, append)
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
Win32File::Win32File(const std::string &filename, FileOpenMode mode)
    : Win32File(new Win32FilePrivate(), filename, mode)
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
Win32File::Win32File(const std::wstring &filename, FileOpenMode mode)
    : Win32File(new Win32FilePrivate(), filename, mode)
{
}

/*! \cond INTERNAL */

Win32File::Win32File(Win32FilePrivate *priv)
    : File(priv)
{
}

Win32File::Win32File(Win32FilePrivate *priv,
                     HANDLE handle, bool owned, bool append)
    : File(priv)
{
    open(handle, owned, append);
}

Win32File::Win32File(Win32FilePrivate *priv,
                     const std::string &filename, FileOpenMode mode)
    : File(priv)
{
    open(filename, mode);
}

Win32File::Win32File(Win32FilePrivate *priv,
                     const std::wstring &filename, FileOpenMode mode)
    : File(priv)
{
    open(filename, mode);
}

/*! \endcond */

Win32File::~Win32File()
{
    close();
}

/*!
 * \brief Open from a Win32 `HANDLE`.
 *
 * If \p owned is true, then the File handle will take ownership of the
 * Win32 `HANDLE`. In other words, the Win32 `HANDLE` will be closed when the
 * File handle is closed.
 *
 * The \p append parameter exists because the Win32 API does not have a native
 * append mode.
 *
 * \param handle Win32 `HANDLE`
 * \param owned Whether the Win32 `HANDLE` should be owned by the File handle
 * \param append Whether append mode should be enabled
 *
 * \return Whether the file is successfully opened
 */
bool Win32File::open(HANDLE handle, bool owned, bool append)
{
    MB_PRIVATE(Win32File);
    if (priv) {
        priv->handle = handle;
        priv->owned = owned;
        priv->append = append;
    }
    return File::open();
}

/*!
 * \brief Open from a multi-byte filename.
 *
 * \p filename is converted to WCS using mbs_to_wcs() before being used.
 *
 * \param filename MBS filename
 * \param mode Open mode (\ref FileOpenMode)
 *
 * \return Whether the file is successfully opened
 */
bool Win32File::open(const std::string &filename, FileOpenMode mode)
{
    MB_PRIVATE(Win32File);
    if (priv) {
        // Convert filename to platform-native encoding
        std::wstring native_filename;
        if (!mbs_to_wcs(native_filename, filename)) {
            set_error(make_error_code(FileError::CannotConvertEncoding),
                      "Failed to convert MBS filename to WCS");
            return false;
        }

        DWORD access;
        DWORD sharing;
        SECURITY_ATTRIBUTES sa;
        DWORD creation;
        DWORD attrib;
        bool append;

        if (!priv->convert_mode(mode, access, sharing, sa, creation, attrib,
                                append)) {
            set_error(make_error_code(FileError::InvalidMode),
                      "Invalid mode: %d", static_cast<int>(mode));
            return false;
        }

        priv->handle = INVALID_HANDLE_VALUE;
        priv->owned = true;
        priv->filename = std::move(native_filename);
        priv->access = access;
        priv->sharing = sharing;
        priv->sa = sa;
        priv->creation = creation;
        priv->attrib = attrib;
        priv->append = append;
    }
    return File::open();
}

/*!
 * \brief Open from a wide-character filename.
 *
 * \p filename is used directly without any conversions.
 *
 * \param filename WCS filename
 * \param mode Open mode (\ref FileOpenMode)
 *
 * \return Whether the file is successfully opened
 */
bool Win32File::open(const std::wstring &filename, FileOpenMode mode)
{
    MB_PRIVATE(Win32File);
    if (priv) {
        DWORD access;
        DWORD sharing;
        SECURITY_ATTRIBUTES sa;
        DWORD creation;
        DWORD attrib;
        bool append;

        if (!priv->convert_mode(mode, access, sharing, sa, creation, attrib,
                                append)) {
            set_error(make_error_code(FileError::InvalidMode),
                      "Invalid mode: %d", static_cast<int>(mode));
            return false;
        }

        priv->handle = INVALID_HANDLE_VALUE;
        priv->owned = true;
        priv->filename = filename;
        priv->access = access;
        priv->sharing = sharing;
        priv->sa = sa;
        priv->creation = creation;
        priv->attrib = attrib;
        priv->append = append;
    }
    return File::open();
}

bool Win32File::on_open()
{
    MB_PRIVATE(Win32File);

    if (!priv->filename.empty()) {
        priv->handle = priv->funcs->fn_CreateFileW(
                priv->filename.c_str(), priv->access, priv->sharing, &priv->sa,
                priv->creation, priv->attrib, nullptr);
        if (priv->handle == INVALID_HANDLE_VALUE) {
            set_error(WIN32_ERROR_CODE(), "Failed to open file");
            return false;
        }
    }

    return true;
}

bool Win32File::on_close()
{
    MB_PRIVATE(Win32File);

    bool ret = true;

    if (priv->owned && priv->handle != INVALID_HANDLE_VALUE
            && !priv->funcs->fn_CloseHandle(priv->handle)) {
        set_error(WIN32_ERROR_CODE(), "Failed to close file");
        ret = false;
    }

    // Reset to allow opening another file
    priv->clear();

    return ret;
}

bool Win32File::on_read(void *buf, size_t size, size_t &bytes_read)
{
    MB_PRIVATE(Win32File);

    DWORD n = 0;

    if (size > UINT_MAX) {
        size = UINT_MAX;
    }

    bool ret = priv->funcs->fn_ReadFile(
        priv->handle,   // hFile
        buf,            // lpBuffer
        size,           // nNumberOfBytesToRead
        &n,             // lpNumberOfBytesRead
        nullptr         // lpOverlapped
    );

    if (!ret) {
        set_error(WIN32_ERROR_CODE(), "Failed to read file");
        return false;
    }

    bytes_read = n;
    return true;
}

bool Win32File::on_write(const void *buf, size_t size, size_t &bytes_written)
{
    MB_PRIVATE(Win32File);

    DWORD n = 0;

    // We have to seek manually in append mode because the Win32 API has no
    // native append mode.
    if (priv->append) {
        uint64_t pos;
        if (!on_seek(0, SEEK_END, pos)) {
            return false;
        }
    }

    if (size > UINT_MAX) {
        size = UINT_MAX;
    }

    bool ret = priv->funcs->fn_WriteFile(
        priv->handle,   // hFile
        buf,            // lpBuffer
        size,           // nNumberOfBytesToWrite
        &n,             // lpNumberOfBytesWritten
        nullptr         // lpOverlapped
    );

    if (!ret) {
        set_error(WIN32_ERROR_CODE(), "Failed to write file");
        return false;
    }

    bytes_written = n;
    return true;
}

bool Win32File::on_seek(int64_t offset, int whence, uint64_t &new_offset)
{
    MB_PRIVATE(Win32File);

    DWORD move_method;
    LARGE_INTEGER pos;
    LARGE_INTEGER new_pos;

    switch (whence) {
    case SEEK_CUR:
        move_method = FILE_CURRENT;
        break;
    case SEEK_SET:
        move_method = FILE_BEGIN;
        break;
    case SEEK_END:
        move_method = FILE_END;
        break;
    default:
        set_error(make_error_code(FileError::InvalidWhence),
                  "Invalid whence argument: %d", whence);
        return false;
    }

    pos.QuadPart = offset;

    bool ret = priv->funcs->fn_SetFilePointerEx(
        priv->handle,   // hFile
        pos,            // liDistanceToMove
        &new_pos,       // lpNewFilePointer
        move_method     // dwMoveMethod
    );

    if (!ret) {
        set_error(WIN32_ERROR_CODE(), "Failed to seek file");
        return false;
    }

    new_offset = static_cast<uint64_t>(new_pos.QuadPart);
    return true;
}

bool Win32File::on_truncate(uint64_t size)
{
    MB_PRIVATE(Win32File);

    bool ret = true;
    uint64_t current_pos;
    uint64_t temp;

    // Get current position
    if (!on_seek(0, SEEK_CUR, current_pos)) {
        return false;
    }

    // Move to new position
    if (!on_seek(static_cast<int64_t>(size), SEEK_SET, temp)) {
        return false;
    }

    // Truncate
    if (!priv->funcs->fn_SetEndOfFile(priv->handle)) {
        set_error(WIN32_ERROR_CODE(), "Failed to set EOF position");
        ret = false;
    }

    // Move back to initial position
    if (!on_seek(static_cast<int64_t>(current_pos), SEEK_SET, temp)) {
        // We can't guarantee the file position so the handle shouldn't be used
        // anymore
        set_fatal(true);
        ret = false;
    }

    return ret;
}

}
