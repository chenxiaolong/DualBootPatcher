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

#include <windows.h>

/*! \cond INTERNAL */
namespace mb
{
namespace detail
{

struct Win32FileFuncs
{
    virtual ~Win32FileFuncs();

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
    virtual BOOL fn_SetFileInformationByHandle(HANDLE hFile,
                                               FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
                                               LPVOID lpFileInformation,
                                               DWORD dwBufferSize) = 0;
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

}
}
/*! \endcond */
