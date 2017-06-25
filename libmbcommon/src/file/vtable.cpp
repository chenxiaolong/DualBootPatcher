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

#include "mbcommon/file/vtable_p.h"

#include <fcntl.h>
#include <unistd.h>

MB_BEGIN_C_DECLS

// fcntl.h

#ifdef _WIN32
static int _default_wopen(void *userdata, const wchar_t *path, int flags,
                          mode_t mode)
{
    (void) userdata;
    return _wopen(path, flags, mode);
}
#else
static int _default_open(void *userdata, const char *path, int flags,
                         mode_t mode)
{
    (void) userdata;
    return open(path, flags, mode);
}
#endif

// sys/stat.h

static int _default_fstat(void *userdata, int fildes, struct stat *buf)
{
    (void) userdata;
    return fstat(fildes, buf);
}

// stdio.h

static int _default_fclose(void *userdata, FILE *stream)
{
    (void) userdata;
    return fclose(stream);
}

static int _default_ferror(void *userdata, FILE *stream)
{
    (void) userdata;
    return ferror(stream);
}

static int _default_fileno(void *userdata, FILE *stream)
{
    (void) userdata;
    return fileno(stream);
}

#ifndef _WIN32
static FILE * _default_fopen(void *userdata, const char *path, const char *mode)
{
    (void) userdata;
    return fopen(path, mode);
}
#endif

static size_t _default_fread(void *userdata, void *ptr, size_t size,
                             size_t nmemb, FILE *stream)
{
    (void) userdata;
    return fread(ptr, size, nmemb, stream);
}

static int _default_fseeko(void *userdata, FILE *stream, off_t offset,
                           int whence)
{
    (void) userdata;
    return fseeko(stream, offset, whence);
}

static off_t _default_ftello(void *userdata, FILE *stream)
{
    (void) userdata;
    return ftello(stream);
}

#ifdef _WIN32
static FILE * _default_wfopen(void *userdata, const wchar_t *filename,
                              const wchar_t *mode)
{
    (void) userdata;
    return _wfopen(filename, mode);
}
#endif

static size_t _default_fwrite(void *userdata, const void *ptr, size_t size,
                              size_t nmemb, FILE *stream)
{
    (void) userdata;
    return fwrite(ptr, size, nmemb, stream);
}

// unistd.h

static int _default_close(void *userdata, int fd)
{
    (void) userdata;
    return close(fd);
}

static int _default_ftruncate64(void *userdata, int fd, off_t length)
{
    (void) userdata;
    return ftruncate64(fd, length);
}

static off64_t _default_lseek64(void *userdata, int fd, off64_t offset,
                                int whence)
{
    (void) userdata;
    return lseek64(fd, offset, whence);
}

static ssize_t _default_read(void *userdata, int fd, void *buf, size_t count)
{
    (void) userdata;
    return read(fd, buf, count);
}

static ssize_t _default_write(void *userdata, int fd, const void *buf,
                              size_t count)
{
    (void) userdata;
    return write(fd, buf, count);
}

#ifdef _WIN32
static BOOL _default_CloseHandle(void *userdata, HANDLE hObject)
{
    (void) userdata;
    return CloseHandle(hObject);
}

static HANDLE _default_CreateFileW(void *userdata, LPCWSTR lpFileName,
                                   DWORD dwDesiredAccess,
                                   DWORD dwShareMode,
                                   LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                   DWORD dwCreationDisposition,
                                   DWORD dwFlagsAndAttributes,
                                   HANDLE hTemplateFile)
{
    (void) userdata;
    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode,
                       lpSecurityAttributes, dwCreationDisposition,
                       dwFlagsAndAttributes, hTemplateFile);
}

static BOOL _default_ReadFile(void *userdata, HANDLE hFile, LPVOID lpBuffer,
                              DWORD nNumberOfBytesToRead,
                              LPDWORD lpNumberOfBytesRead,
                              LPOVERLAPPED lpOverlapped)
{
    (void) userdata;
    return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead,
                    lpOverlapped);
}

static BOOL _default_SetEndOfFile(void *userdata, HANDLE hFile)
{
    (void) userdata;
    return SetEndOfFile(hFile);
}

static BOOL _default_SetFilePointerEx(void *userdata, HANDLE hFile,
                                      LARGE_INTEGER liDistanceToMove,
                                      PLARGE_INTEGER lpNewFilePointer,
                                      DWORD dwMoveMethod)
{
    (void) userdata;
    return SetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer,
                            dwMoveMethod);
}

static BOOL _default_WriteFile(void *userdata, HANDLE hFile, LPCVOID lpBuffer,
                               DWORD nNumberOfBytesToWrite,
                               LPDWORD lpNumberOfBytesWritten,
                               LPOVERLAPPED lpOverlapped)
{
    (void) userdata;
    return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite,
                     lpNumberOfBytesWritten, lpOverlapped);
}
#endif

void _vtable_fill_system_funcs(SysVtable *vtable)
{
    // fcntl.h
#ifdef _WIN32
    vtable->fn_wopen = _default_wopen;
#else
    vtable->fn_open = _default_open;
#endif
    // sys/stat.h
    vtable->fn_fstat = _default_fstat;
    // stdio.h
    vtable->fn_fclose = _default_fclose;
    vtable->fn_ferror = _default_ferror;
    vtable->fn_fileno = _default_fileno;
#ifndef _WIN32
    vtable->fn_fopen = _default_fopen;
#endif
    vtable->fn_fread = _default_fread;
    vtable->fn_fseeko = _default_fseeko;
    vtable->fn_ftello = _default_ftello;
#ifdef _WIN32
    vtable->fn_wfopen = _default_wfopen;
#endif
    vtable->fn_fwrite = _default_fwrite;
    // unistd.h
    vtable->fn_close = _default_close;
    vtable->fn_ftruncate64 = _default_ftruncate64;
    vtable->fn_lseek64 = _default_lseek64;
    vtable->fn_read = _default_read;
    vtable->fn_write = _default_write;
#ifdef _WIN32
    // windows.h
    vtable->fn_CloseHandle = _default_CloseHandle;
    vtable->fn_CreateFileW = _default_CreateFileW;
    vtable->fn_ReadFile = _default_ReadFile;
    vtable->fn_SetEndOfFile = _default_SetEndOfFile;
    vtable->fn_SetFilePointerEx = _default_SetFilePointerEx;
    vtable->fn_WriteFile = _default_WriteFile;
#endif
}

MB_END_C_DECLS
