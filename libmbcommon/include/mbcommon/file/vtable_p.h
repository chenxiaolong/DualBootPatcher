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

#include <cstddef>
#include <cstdio>

#include <sys/stat.h>

#ifdef _WIN32
#  ifdef __cplusplus
#    include <cwchar>
#  else
#    include <wchar.h>
#  endif
#  include <windows.h>
#endif

#include "mbcommon/common.h"

/*! \cond INTERNAL */
MB_BEGIN_C_DECLS

// Used for mocking system functions for unit testing. The "fn_" prefixes are
// ugly, but they're there because some systems (eg. Android API 17) define the
// symbols as macros.
struct SysVtable
{
    // fcntl.h
#ifdef _WIN32
    typedef int (*PosixWopenFn)(void *userdata, const wchar_t *path, int flags,
                                mode_t mode);
#else
    typedef int (*PosixOpenFn)(void *userdata, const char *path, int flags,
                               mode_t mode);
#endif
#ifdef _WIN32
    PosixWopenFn fn_wopen;
#else
    PosixOpenFn fn_open;
#endif

    // sys/stat.h
    typedef int (*PosixFstatFn)(void *userdata, int fildes, struct stat *buf);
    PosixFstatFn fn_fstat;

    // stdio.h
    typedef int (*PosixFcloseFn)(void *userdata, FILE *stream);
    typedef int (*PosixFerrorFn)(void *userdata, FILE *stream);
    typedef int (*PosixFilenoFn)(void *userdata, FILE *stream);
#ifndef _WIN32
    typedef FILE * (*PosixFopenFn)(void *userdata, const char *path,
                                   const char *mode);
#endif
    typedef size_t (*PosixFreadFn)(void *userdata, void *ptr, size_t size,
                                   size_t nmemb, FILE *stream);
    typedef int (*PosixFseekoFn)(void *userdata, FILE *stream, off_t offset,
                                 int whence);
    typedef off_t (*PosixFtelloFn)(void *userdata, FILE *stream);
#ifdef _WIN32
    typedef FILE * (*PosixWfopenFn)(void *userdata, const wchar_t *filename,
                                    const wchar_t *mode);
#endif
    typedef size_t (*PosixFwriteFn)(void *userdata, const void *ptr,
                                    size_t size, size_t nmemb, FILE *stream);
    PosixFcloseFn fn_fclose;
    PosixFerrorFn fn_ferror;
    PosixFilenoFn fn_fileno;
#ifndef _WIN32
    PosixFopenFn fn_fopen;
#endif
    PosixFreadFn fn_fread;
    PosixFseekoFn fn_fseeko;
    PosixFtelloFn fn_ftello;
#ifdef _WIN32
    PosixWfopenFn fn_wfopen;
#endif
    PosixFwriteFn fn_fwrite;

    // unistd.h
    typedef int (*PosixCloseFn)(void *userdata, int fd);
    typedef int (*PosixFtruncate64Fn)(void *userdata, int fd, off_t length);
    typedef off64_t (*PosixLseek64Fn)(void *userdata, int fd, off64_t offset,
                                      int whence);
    typedef ssize_t (*PosixReadFn)(void *userdata, int fd, void *buf,
                                   size_t count);
    typedef ssize_t (*PosixWriteFn)(void *userdata, int fd, const void *buf,
                                    size_t count);
    PosixCloseFn fn_close;
    PosixFtruncate64Fn fn_ftruncate64;
    PosixLseek64Fn fn_lseek64;
    PosixReadFn fn_read;
    PosixWriteFn fn_write;

#ifdef _WIN32
    // windows.h
    typedef BOOL (*Win32CloseHandleFn)(void *userdata, HANDLE hObject);
    typedef HANDLE (*Win32CreateFileWFn)(void *userdata, LPCWSTR lpFileName,
                                         DWORD dwDesiredAccess,
                                         DWORD dwShareMode,
                                         LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                         DWORD dwCreationDisposition,
                                         DWORD dwFlagsAndAttributes,
                                         HANDLE hTemplateFile);
    typedef BOOL (*Win32ReadFileFn)(void *userdata, HANDLE hFile,
                                    LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                                    LPDWORD lpNumberOfBytesRead,
                                    LPOVERLAPPED lpOverlapped);
    typedef BOOL (*Win32SetEndOfFileFn)(void *userdata, HANDLE hFile);
    typedef BOOL (*Win32SetFilePointerExFn)(void *userdata, HANDLE hFile,
                                            LARGE_INTEGER liDistanceToMove,
                                            PLARGE_INTEGER lpNewFilePointer,
                                            DWORD dwMoveMethod);
    typedef BOOL (*Win32WriteFileFn)(void *userdata, HANDLE hFile,
                                     LPCVOID lpBuffer,
                                     DWORD nNumberOfBytesToWrite,
                                     LPDWORD lpNumberOfBytesWritten,
                                     LPOVERLAPPED lpOverlapped);
    Win32CloseHandleFn fn_CloseHandle;
    Win32CreateFileWFn fn_CreateFileW;
    Win32ReadFileFn fn_ReadFile;
    Win32SetEndOfFileFn fn_SetEndOfFile;
    Win32SetFilePointerExFn fn_SetFilePointerEx;
    Win32WriteFileFn fn_WriteFile;
#endif

    void *userdata;
};

// Default system implementations
void _vtable_fill_system_funcs(SysVtable *vtable);

MB_END_C_DECLS
/*! \endcond */
