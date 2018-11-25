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

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/locale.h"

static_assert(sizeof(DWORD) == 4, "DWORD is not 32 bits");

/*!
 * \file mbcommon/file/win32.h
 * \brief Open file with Win32 `HANDLE` API
 */

namespace mb
{

using namespace detail;

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

    virtual BOOL fn_SetFileInformationByHandle(HANDLE hFile,
                                               FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
                                               LPVOID lpFileInformation,
                                               DWORD dwBufferSize) override
    {
        return SetFileInformationByHandle(hFile, FileInformationClass,
                                          lpFileInformation, dwBufferSize);
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

Win32FileFuncs::~Win32FileFuncs() = default;

static bool convert_mode(FileOpenMode mode,
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
    case FileOpenMode::ReadOnly:
        access = GENERIC_READ;
        creation = OPEN_EXISTING;
        break;
    case FileOpenMode::ReadWrite:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = OPEN_EXISTING;
        break;
    case FileOpenMode::WriteOnly:
        access = GENERIC_WRITE;
        creation = CREATE_ALWAYS;
        break;
    case FileOpenMode::ReadWriteTrunc:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = CREATE_ALWAYS;
        break;
    case FileOpenMode::Append:
        access = GENERIC_WRITE;
        creation = OPEN_ALWAYS;
        append = true;
        break;
    case FileOpenMode::ReadAppend:
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
    : Win32File(&g_default_funcs)
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
    : Win32File(&g_default_funcs, handle, owned, append)
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
    : Win32File(&g_default_funcs, filename, mode)
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
    : Win32File(&g_default_funcs, filename, mode)
{
}

/*! \cond INTERNAL */

Win32File::Win32File(Win32FileFuncs *funcs)
    : File(), m_funcs(funcs)
{
    clear();
}

Win32File::Win32File(Win32FileFuncs *funcs,
                     HANDLE handle, bool owned, bool append)
    : Win32File(funcs)
{
    (void) open(handle, owned, append);
}

Win32File::Win32File(Win32FileFuncs *funcs,
                     const std::string &filename, FileOpenMode mode)
    : Win32File(funcs)
{
    (void) open(filename, mode);
}

Win32File::Win32File(Win32FileFuncs *funcs,
                     const std::wstring &filename, FileOpenMode mode)
    : Win32File(funcs)
{
    (void) open(filename, mode);
}

/*! \endcond */

Win32File::~Win32File()
{
    (void) close();
}

Win32File::Win32File(Win32File &&other) noexcept
    : File(std::move(other))
    , m_funcs(other.m_funcs)
    , m_handle(other.m_handle)
    , m_owned(other.m_owned)
    , m_filename(std::move(other.m_filename))
    , m_access(other.m_access)
    , m_sharing(other.m_sharing)
    , m_sa(other.m_sa)
    , m_creation(other.m_creation)
    , m_attrib(other.m_attrib)
    , m_append(other.m_append)
{
    other.clear();
}

Win32File & Win32File::operator=(Win32File &&rhs) noexcept
{
    File::operator=(std::move(rhs));

    m_funcs = rhs.m_funcs;
    m_handle = rhs.m_handle;
    m_owned = rhs.m_owned;
    m_filename.swap(rhs.m_filename);
    m_access = rhs.m_access;
    m_sharing = rhs.m_sharing;
    m_sa = rhs.m_sa;
    m_creation = rhs.m_creation;
    m_attrib = rhs.m_attrib;
    m_append = rhs.m_append;

    rhs.clear();

    return *this;
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
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> Win32File::open(HANDLE handle, bool owned, bool append)
{
    if (state() == FileState::New) {
        m_handle = handle;
        m_owned = owned;
        m_append = append;
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
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> Win32File::open(const std::string &filename, FileOpenMode mode)
{
    if (state() == FileState::New) {
        // Convert filename to platform-native encoding
        auto converted = mbs_to_wcs(filename);
        if (!converted) {
            return FileError::CannotConvertEncoding;
        }

        DWORD access;
        DWORD sharing;
        SECURITY_ATTRIBUTES sa;
        DWORD creation;
        DWORD attrib;
        bool append;

        if (!convert_mode(mode, access, sharing, sa, creation, attrib, append)) {
            MB_UNREACHABLE("Invalid mode: %d", static_cast<int>(mode));
        }

        m_handle = INVALID_HANDLE_VALUE;
        m_owned = true;
        m_filename = std::move(converted.value());
        m_access = access;
        m_sharing = sharing;
        m_sa = sa;
        m_creation = creation;
        m_attrib = attrib;
        m_append = append;
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
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> Win32File::open(const std::wstring &filename, FileOpenMode mode)
{
    if (state() == FileState::New) {
        DWORD access;
        DWORD sharing;
        SECURITY_ATTRIBUTES sa;
        DWORD creation;
        DWORD attrib;
        bool append;

        if (!convert_mode(mode, access, sharing, sa, creation, attrib, append)) {
            MB_UNREACHABLE("Invalid mode: %d", static_cast<int>(mode));
        }

        m_handle = INVALID_HANDLE_VALUE;
        m_owned = true;
        m_filename = filename;
        m_access = access;
        m_sharing = sharing;
        m_sa = sa;
        m_creation = creation;
        m_attrib = attrib;
        m_append = append;
    }

    return File::open();
}

oc::result<void> Win32File::on_open()
{
    if (!m_filename.empty()) {
        m_handle = m_funcs->fn_CreateFileW(
                m_filename.c_str(), m_access, m_sharing, &m_sa, m_creation,
                m_attrib, nullptr);
        if (m_handle == INVALID_HANDLE_VALUE) {
            return ec_from_win32();
        }
    }

    return oc::success();
}

oc::result<void> Win32File::on_close()
{
    // Reset to allow opening another file
    auto reset = finally([&] {
        clear();
    });

    if (m_owned && m_handle != INVALID_HANDLE_VALUE
            && !m_funcs->fn_CloseHandle(m_handle)) {
        return ec_from_win32();
    }

    return oc::success();
}

oc::result<size_t> Win32File::on_read(void *buf, size_t size)
{
    DWORD n = 0;

    if (size > UINT_MAX) {
        size = UINT_MAX;
    }

    bool ret = m_funcs->fn_ReadFile(
        m_handle,   // hFile
        buf,        // lpBuffer
        size,       // nNumberOfBytesToRead
        &n,         // lpNumberOfBytesRead
        nullptr     // lpOverlapped
    );

    if (!ret) {
        return ec_from_win32();
    }

    return n;
}

oc::result<size_t> Win32File::on_write(const void *buf, size_t size)
{
    DWORD n = 0;

    // We have to seek manually in append mode because the Win32 API has no
    // native append mode.
    if (m_append) {
        OUTCOME_TRYV(on_seek(0, SEEK_END));
    }

    if (size > UINT_MAX) {
        size = UINT_MAX;
    }

    bool ret = m_funcs->fn_WriteFile(
        m_handle,   // hFile
        buf,        // lpBuffer
        size,       // nNumberOfBytesToWrite
        &n,         // lpNumberOfBytesWritten
        nullptr     // lpOverlapped
    );

    if (!ret) {
        return ec_from_win32();
    }

    return n;
}

oc::result<uint64_t> Win32File::on_seek(int64_t offset, int whence)
{
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
        MB_UNREACHABLE("Invalid whence argument: %d", whence);
    }

    pos.QuadPart = offset;

    bool ret = m_funcs->fn_SetFilePointerEx(
        m_handle,   // hFile
        pos,        // liDistanceToMove
        &new_pos,   // lpNewFilePointer
        move_method // dwMoveMethod
    );

    if (!ret) {
        return ec_from_win32();
    }

    return static_cast<uint64_t>(new_pos.QuadPart);
}

oc::result<void> Win32File::on_truncate(uint64_t size)
{
    FILE_END_OF_FILE_INFO info;
    info.EndOfFile.QuadPart = static_cast<int64_t>(size);

    bool ret = m_funcs->fn_SetFileInformationByHandle(m_handle,
                                                      FileEndOfFileInfo,
                                                      &info, sizeof(info));

    if (!ret) {
        return ec_from_win32();
    }

    return oc::success();
}

void Win32File::clear()
{
    m_handle = INVALID_HANDLE_VALUE;
    m_owned = false;
    m_filename.clear();
    m_access = 0;
    m_sharing = 0;
    m_sa = {};
    m_creation = 0;
    m_attrib = 0;
    m_append = false;
}

}
