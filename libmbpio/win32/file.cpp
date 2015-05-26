/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libmbpio/win32/file.h"

#include <stdlib.h>
#include <tchar.h>

#include <windows.h>

#include "libmbpio/private/utf8.h"
#include "libmbpio/win32/error.h"

namespace io
{
namespace win32
{

class FileWin32::Impl
{
public:
    HANDLE handle = nullptr;
    int error;
    DWORD win32Error;
    std::wstring win32ErrorString;
};

FileWin32::FileWin32() : m_impl(new Impl())
{
}

FileWin32::~FileWin32()
{
    if (isOpen()) {
        close();
    }
}

bool FileWin32::open(const char *filename, int mode)
{
    if (!filename) {
        m_impl->error = ErrorInvalidFilename;
        return false;
    }

    std::wstring wFilename = utf8::utf8ToUtf16(filename);

    DWORD dwDesiredAccess = 0;
    DWORD dwShareMode = 0;
    DWORD dwCreationDisposition = 0;
    DWORD dwFlagsAndAttributes = 0;
    HANDLE handle;

    switch (mode) {
    case OpenRead:
        dwDesiredAccess = GENERIC_READ;
        dwCreationDisposition = OPEN_EXISTING;
        dwShareMode = FILE_SHARE_READ;
        break;
    case OpenWrite:
        dwDesiredAccess = GENERIC_WRITE;
        dwCreationDisposition = CREATE_ALWAYS;
        break;
    case OpenAppend:
        dwDesiredAccess = FILE_APPEND_DATA;
        dwCreationDisposition = OPEN_ALWAYS;
        break;
    default:
        m_impl->error = ErrorInvalidOpenMode;
        return false;
    }

    handle = CreateFileW(
        (LPCWSTR) wFilename.c_str(),    // lpFileName
        dwDesiredAccess,                // dwDesiredAccess
        dwShareMode,                    // dwShareMode
        nullptr,                        // lpSecurityAttributes
        dwCreationDisposition,          // dwCreationDisposition
        dwFlagsAndAttributes,           // dwFlagsAndAttributes
        nullptr                         // hTemplateFile
    );

    if (handle == INVALID_HANDLE_VALUE) {
        m_impl->error = ErrorPlatformError;
        m_impl->win32Error = GetLastError();
        m_impl->win32ErrorString = errorToWString(m_impl->win32Error);
        return false;
    }

    m_impl->handle = handle;

    return true;
}

bool FileWin32::open(const std::string &filename, int mode)
{
    return open(filename.c_str(), mode);
}

bool FileWin32::close()
{
    if (!m_impl->handle) {
        m_impl->error = ErrorFileIsNotOpen;
        return false;
    }

    if (!CloseHandle(m_impl->handle)) {
        m_impl->error = ErrorPlatformError;
        m_impl->win32Error = GetLastError();
        m_impl->win32ErrorString = errorToWString(m_impl->win32Error);
        return false;
    }

    return true;
}

bool FileWin32::isOpen()
{
    return m_impl->handle != nullptr;
}

bool FileWin32::read(void *buf, uint64_t size, uint64_t *bytesRead)
{
    unsigned long n = 0;

    bool ret = ReadFile(
        m_impl->handle, // hFile
        buf,            // lpBuffer
        size,           // nNumberOfBytesToRead
        &n,             // lpNumberOfBytesRead
        nullptr         // lpOverlapped
    );

    *bytesRead = n;

    if (!ret) {
        m_impl->error = ErrorPlatformError;
        m_impl->win32Error = GetLastError();
        m_impl->win32ErrorString = errorToWString(m_impl->win32Error);
        return false;
    } else if (n == 0) {
        m_impl->error = ErrorEndOfFile;
        return false;
    }

    return true;
}

bool FileWin32::write(const void *buf, uint64_t size, uint64_t *bytesWritten)
{
    unsigned long n = 0;

    bool ret = WriteFile(
        m_impl->handle, // hFile
        buf,            // lpBuffer
        size,           // nNumberOfBytesToWrite
        &n,             // lpNumberOfBytesWritten
        nullptr         // lpOverlapped
    );

    *bytesWritten = n;

    if (!ret) {
        m_impl->error = ErrorPlatformError;
        m_impl->win32Error = GetLastError();
        m_impl->win32ErrorString = errorToWString(m_impl->win32Error);
        return false;
    }

    return true;
}

static win32_wrap_SetFilePointer(HANDLE handle, LARGE_INTEGER pos,
                                 LARGE_INTEGER *newPos, DWORD dwMoveMethod)
{
    LONG lHigh = pos.HighPart;
    DWORD dwNewPos = SetFilePointer(
        handle,         // hFile
        pos.LowPart,    // lDistanceToMove
        &lHigh,         // lpDistanceToMoveHigh
        dwMoveMethod    // dwMoveMethod
    );

    if (dwNewPos == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
        return false;
    }

    if (newPos) {
        newPos->LowPart = dwNewPos;
        newPos->HighPart = lHigh;
    }

    return true;
}

bool FileWin32::tell(uint64_t *pos)
{
    LARGE_INTEGER fpos;
    fpos.QuadPart = 0;

    if (!win32_wrap_SetFilePointer(m_impl->handle, fpos, &fpos, FILE_CURRENT)) {
        m_impl->error = ErrorPlatformError;
        m_impl->win32Error = GetLastError();
        m_impl->win32ErrorString = errorToWString(m_impl->win32Error);
        return false;
    }

    *pos = fpos.QuadPart;
    return true;
}

bool FileWin32::seek(int64_t offset, int origin)
{
    DWORD dwMoveMethod;
    switch (origin) {
    case SeekCurrent:
        dwMoveMethod = FILE_CURRENT;
        break;
    case SeekBegin:
        dwMoveMethod = FILE_BEGIN;
        break;
    case SeekEnd:
        dwMoveMethod = FILE_END;
        break;
    default:
        m_impl->error = ErrorInvalidSeekOrigin;
        return false;
    }

    LARGE_INTEGER pos;
    pos.QuadPart = offset;

    if (!win32_wrap_SetFilePointer(
            m_impl->handle, pos, nullptr, dwMoveMethod)) {
        m_impl->error = ErrorPlatformError;
        m_impl->win32Error = GetLastError();
        m_impl->win32ErrorString = errorToWString(m_impl->win32Error);
        return false;
    }

    return true;
}

int FileWin32::error()
{
    return m_impl->error;
}

std::string FileWin32::platformErrorString()
{
    return utf8::utf16ToUtf8(m_impl->win32ErrorString);
}

}
}
