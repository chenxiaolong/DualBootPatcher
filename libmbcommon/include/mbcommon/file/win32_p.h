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

#pragma once

#include "mbcommon/guard_p.h"

#include <windows.h>

#include "mbcommon/file/win32.h"
#include "mbcommon/file_p.h"

/*! \cond INTERNAL */
namespace mb
{

struct Win32FileFuncs
{
    // windows.h
    virtual BOOL fn_CloseHandle(HANDLE hObject) = 0;
    virtual HANDLE fn_CreateFileW(LPCWSTR lpFileName,
                                  DWORD dwDesiredAccess,
                                  DWORD dwShareMode,
                                  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                  DWORD dwCreationDisposition,
                                  DWORD dwFlagsAndAttributes,
                                  HANDLE hTemplateFile) = 0;
    virtual BOOL fn_ReadFile(HANDLE hFile,
                             LPVOID lpBuffer,
                             DWORD nNumberOfBytesToRead,
                             LPDWORD lpNumberOfBytesRead,
                             LPOVERLAPPED lpOverlapped) = 0;
    virtual BOOL fn_SetEndOfFile(HANDLE hFile) = 0;
    virtual BOOL fn_SetFilePointerEx(HANDLE hFile,
                                     LARGE_INTEGER liDistanceToMove,
                                     PLARGE_INTEGER lpNewFilePointer,
                                     DWORD dwMoveMethod) = 0;
    virtual BOOL fn_WriteFile(HANDLE hFile,
                              LPCVOID lpBuffer,
                              DWORD nNumberOfBytesToWrite,
                              LPDWORD lpNumberOfBytesWritten,
                              LPOVERLAPPED lpOverlapped) = 0;
};

class Win32FilePrivate : public FilePrivate
{
public:
    Win32FilePrivate();
    virtual ~Win32FilePrivate();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(Win32FilePrivate)

    void clear();

    static bool convert_mode(FileOpenMode mode,
                             DWORD &access_out,
                             DWORD &sharing_out,
                             SECURITY_ATTRIBUTES &sa_out,
                             DWORD &creation_out,
                             DWORD &attrib_out,
                             bool &append_out);

    Win32FileFuncs *funcs;

    HANDLE handle;
    bool owned;
    std::wstring filename;

    DWORD access;
    DWORD sharing;
    SECURITY_ATTRIBUTES sa;
    DWORD creation;
    DWORD attrib;

    bool append;

    LPWSTR error;

protected:
    Win32FilePrivate(Win32FileFuncs *funcs);
};

}
/*! \endcond */
