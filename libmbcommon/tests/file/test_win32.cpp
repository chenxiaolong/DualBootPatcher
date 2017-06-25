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

#include <gtest/gtest.h>

#include <climits>

#include "mbcommon/file.h"
#include "mbcommon/file/vtable_p.h"
#include "mbcommon/file/win32.h"
#include "mbcommon/file/win32_p.h"

struct FileWin32Test : testing::Test
{
    MbFile *_file;
    SysVtable _vtable;

    int _n_CloseHandle = 0;
    int _n_CreateFileW = 0;
    int _n_ReadFile = 0;
    int _n_SetEndOfFile = 0;
    int _n_SetFilePointerEx = 0;
    int _n_WriteFile = 0;

    FileWin32Test() : _file(mb_file_new())
    {
        // These all fail by default
        _vtable.fn_CloseHandle = _CloseHandle;
        _vtable.fn_CreateFileW = _CreateFileW;
        _vtable.fn_ReadFile = _ReadFile;
        _vtable.fn_SetEndOfFile = _SetEndOfFile;
        _vtable.fn_SetFilePointerEx = _SetFilePointerEx;
        _vtable.fn_WriteFile = _WriteFile;

        _vtable.userdata = this;
    }

    virtual ~FileWin32Test()
    {
        mb_file_free(_file);
    }

    static BOOL _CloseHandle(void *userdata, HANDLE hObject)
    {
        (void) hObject;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_CloseHandle;

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    static HANDLE _CreateFileW(void *userdata, LPCWSTR lpFileName,
                               DWORD dwDesiredAccess, DWORD dwShareMode,
                               LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                               DWORD dwCreationDisposition,
                               DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
    {
        (void) lpFileName;
        (void) dwDesiredAccess;
        (void) dwShareMode;
        (void) lpSecurityAttributes;
        (void) dwCreationDisposition;
        (void) dwFlagsAndAttributes;
        (void) hTemplateFile;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_CreateFileW;

        SetLastError(ERROR_INVALID_HANDLE);
        return INVALID_HANDLE_VALUE;
    }

    static BOOL _ReadFile(void *userdata, HANDLE hFile, LPVOID lpBuffer,
                          DWORD nNumberOfBytesToRead,
                          LPDWORD lpNumberOfBytesRead,
                          LPOVERLAPPED lpOverlapped)
    {
        (void) hFile;
        (void) lpBuffer;
        (void) nNumberOfBytesToRead;
        (void) lpNumberOfBytesRead;
        (void) lpOverlapped;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_ReadFile;

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    static BOOL _SetEndOfFile(void *userdata, HANDLE hFile)
    {
        (void) hFile;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetEndOfFile;

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    static BOOL _SetFilePointerEx(void *userdata, HANDLE hFile,
                                  LARGE_INTEGER liDistanceToMove,
                                  PLARGE_INTEGER lpNewFilePointer,
                                  DWORD dwMoveMethod)
    {
        (void) hFile;
        (void) liDistanceToMove;
        (void) lpNewFilePointer;
        (void) dwMoveMethod;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetFilePointerEx;

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    static BOOL _WriteFile(void *userdata, HANDLE hFile, LPCVOID lpBuffer,
                           DWORD nNumberOfBytesToWrite,
                           LPDWORD lpNumberOfBytesWritten,
                           LPOVERLAPPED lpOverlapped)
    {
        (void) hFile;
        (void) lpBuffer;
        (void) nNumberOfBytesToWrite;
        (void) lpNumberOfBytesWritten;
        (void) lpOverlapped;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_WriteFile;

        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
};

TEST_F(FileWin32Test, OpenNoVtable)
{
    memset(&_vtable, 0, sizeof(_vtable));
    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, false, false),
              MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INTERNAL_ERROR);
}

TEST_F(FileWin32Test, OpenFilenameMbsSuccess)
{
    _vtable.fn_CreateFileW = [](void *userdata, LPCWSTR lpFileName,
                                DWORD dwDesiredAccess, DWORD dwShareMode,
                                LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                DWORD dwCreationDisposition,
                                DWORD dwFlagsAndAttributes,
                                HANDLE hTemplateFile) -> HANDLE {
        (void) lpFileName;
        (void) dwDesiredAccess;
        (void) dwShareMode;
        (void) lpSecurityAttributes;
        (void) dwCreationDisposition;
        (void) dwFlagsAndAttributes;
        (void) hTemplateFile;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_CreateFileW;

        return reinterpret_cast<HANDLE>(1);
    };

    ASSERT_EQ(_mb_file_open_HANDLE_filename(&_vtable, _file, "x",
                                            MB_FILE_OPEN_READ_ONLY),
              MB_FILE_OK);
    ASSERT_EQ(_n_CreateFileW, 1);
}

TEST_F(FileWin32Test, OpenFilenameMbsFailure)
{
    ASSERT_EQ(_mb_file_open_HANDLE_filename(&_vtable, _file, "x",
                                            MB_FILE_OPEN_READ_ONLY),
              MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
    ASSERT_EQ(_n_CreateFileW, 1);
}

TEST_F(FileWin32Test, OpenFilenameMbsInvalidMode)
{
    ASSERT_EQ(_mb_file_open_HANDLE_filename(&_vtable, _file, "x", -1),
              MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INVALID_ARGUMENT);
}

TEST_F(FileWin32Test, OpenFilenameWcsSuccess)
{
    _vtable.fn_CreateFileW = [](void *userdata, LPCWSTR lpFileName,
                                DWORD dwDesiredAccess, DWORD dwShareMode,
                                LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                DWORD dwCreationDisposition,
                                DWORD dwFlagsAndAttributes,
                                HANDLE hTemplateFile) -> HANDLE {
        (void) lpFileName;
        (void) dwDesiredAccess;
        (void) dwShareMode;
        (void) lpSecurityAttributes;
        (void) dwCreationDisposition;
        (void) dwFlagsAndAttributes;
        (void) hTemplateFile;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_CreateFileW;

        return reinterpret_cast<HANDLE>(1);
    };

    ASSERT_EQ(_mb_file_open_HANDLE_filename_w(&_vtable, _file, L"x",
                                              MB_FILE_OPEN_READ_ONLY),
              MB_FILE_OK);
    ASSERT_EQ(_n_CreateFileW, 1);
}

TEST_F(FileWin32Test, OpenFilenameWcsFailure)
{
    ASSERT_EQ(_mb_file_open_HANDLE_filename_w(&_vtable, _file, L"x",
                                              MB_FILE_OPEN_READ_ONLY),
              MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
    ASSERT_EQ(_n_CreateFileW, 1);
}

TEST_F(FileWin32Test, OpenFilenameWcsInvalidMode)
{
    ASSERT_EQ(_mb_file_open_HANDLE_filename_w(&_vtable, _file, L"x", -1),
              MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INVALID_ARGUMENT);
}

TEST_F(FileWin32Test, CloseUnownedFile)
{
    _vtable.fn_CloseHandle = [](void *userdata, HANDLE hObject) -> BOOL {
        (void) hObject;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_CloseHandle;

        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, false, false),
              MB_FILE_OK);

    // Ensure that the close callback is not called
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_n_CloseHandle, 0);
}

TEST_F(FileWin32Test, CloseOwnedFile)
{
    _vtable.fn_CloseHandle = [](void *userdata, HANDLE hObject) -> BOOL {
        (void) hObject;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_CloseHandle;

        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the close callback is called
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_n_CloseHandle, 1);
}

TEST_F(FileWin32Test, CloseFailure)
{
    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the close callback is called
    ASSERT_EQ(mb_file_close(_file), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
    ASSERT_EQ(_n_CloseHandle, 1);
}

TEST_F(FileWin32Test, ReadSuccess)
{
    _vtable.fn_ReadFile = [](void *userdata, HANDLE hFile, LPVOID lpBuffer,
                             DWORD nNumberOfBytesToRead,
                             LPDWORD lpNumberOfBytesRead,
                             LPOVERLAPPED lpOverlapped) -> BOOL {
        (void) hFile;
        (void) lpBuffer;
        (void) lpOverlapped;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_ReadFile;

        *lpNumberOfBytesRead = nNumberOfBytesToRead;
        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(_n_ReadFile, 1);
}

#if SIZE_MAX > UINT_MAX
TEST_F(FileWin32Test, ReadSuccessMaxSize)
{
    _vtable.fn_ReadFile = [](void *userdata, HANDLE hFile, LPVOID lpBuffer,
                             DWORD nNumberOfBytesToRead,
                             LPDWORD lpNumberOfBytesRead,
                             LPOVERLAPPED lpOverlapped) -> BOOL {
        (void) hFile;
        (void) lpBuffer;
        (void) lpOverlapped;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_ReadFile;

        *lpNumberOfBytesRead = nNumberOfBytesToRead;
        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the read callback is called
    size_t n;
    ASSERT_EQ(mb_file_read(_file, nullptr, static_cast<size_t>(UINT_MAX) + 1,
                           &n), MB_FILE_OK);
    ASSERT_EQ(n, UINT_MAX);
    ASSERT_EQ(_n_ReadFile, 1);
}
#endif

TEST_F(FileWin32Test, ReadEof)
{
    _vtable.fn_ReadFile = [](void *userdata, HANDLE hFile, LPVOID lpBuffer,
                             DWORD nNumberOfBytesToRead,
                             LPDWORD lpNumberOfBytesRead,
                             LPOVERLAPPED lpOverlapped) -> BOOL {
        (void) hFile;
        (void) lpBuffer;
        (void) nNumberOfBytesToRead;
        (void) lpOverlapped;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_ReadFile;

        *lpNumberOfBytesRead = 0;
        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 0);
    ASSERT_EQ(_n_ReadFile, 1);
}

TEST_F(FileWin32Test, ReadFailure)
{
    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the read callback is called
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_FAILED);
    ASSERT_EQ(_n_ReadFile, 1);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, WriteSuccess)
{
    _vtable.fn_WriteFile = [](void *userdata, HANDLE hFile, LPCVOID lpBuffer,
                              DWORD nNumberOfBytesToWrite,
                              LPDWORD lpNumberOfBytesWritten,
                              LPOVERLAPPED lpOverlapped) -> BOOL {
        (void) hFile;
        (void) lpBuffer;
        (void) lpOverlapped;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_WriteFile;

        *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(_n_WriteFile, 1);
}

#if SIZE_MAX > UINT_MAX
TEST_F(FileWin32Test, WriteSuccessMaxSize)
{
    _vtable.fn_WriteFile = [](void *userdata, HANDLE hFile, LPCVOID lpBuffer,
                              DWORD nNumberOfBytesToWrite,
                              LPDWORD lpNumberOfBytesWritten,
                              LPOVERLAPPED lpOverlapped) -> BOOL {
        (void) hFile;
        (void) lpBuffer;
        (void) lpOverlapped;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_WriteFile;

        *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, nullptr, static_cast<size_t>(UINT_MAX) + 1,
                            &n), MB_FILE_OK);
    ASSERT_EQ(n, UINT_MAX);
    ASSERT_EQ(_n_WriteFile, 1);
}
#endif

TEST_F(FileWin32Test, WriteEof)
{
    _vtable.fn_WriteFile = [](void *userdata, HANDLE hFile, LPCVOID lpBuffer,
                              DWORD nNumberOfBytesToWrite,
                              LPDWORD lpNumberOfBytesWritten,
                              LPOVERLAPPED lpOverlapped) -> BOOL {
        (void) hFile;
        (void) lpBuffer;
        (void) nNumberOfBytesToWrite;
        (void) lpOverlapped;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_WriteFile;

        *lpNumberOfBytesWritten = 0;
        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 0);
    ASSERT_EQ(_n_WriteFile, 1);
}

TEST_F(FileWin32Test, WriteFailure)
{
    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the write callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_FAILED);
    ASSERT_EQ(_n_WriteFile, 1);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, WriteAppendSuccess)
{
    _vtable.fn_SetFilePointerEx = [](void *userdata, HANDLE hFile,
                                     LARGE_INTEGER liDistanceToMove,
                                     PLARGE_INTEGER lpNewFilePointer,
                                     DWORD dwMoveMethod) -> BOOL {
        (void) hFile;
        (void) liDistanceToMove;
        (void) lpNewFilePointer;
        (void) dwMoveMethod;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetFilePointerEx;

        lpNewFilePointer->QuadPart = 0;
        return TRUE;
    };
    _vtable.fn_WriteFile = [](void *userdata, HANDLE hFile, LPCVOID lpBuffer,
                              DWORD nNumberOfBytesToWrite,
                              LPDWORD lpNumberOfBytesWritten,
                              LPOVERLAPPED lpOverlapped) -> BOOL {
        (void) hFile;
        (void) lpBuffer;
        (void) lpOverlapped;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_WriteFile;

        *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, true),
              MB_FILE_OK);

    // Ensure that the seek and write callbacks are called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(_n_SetFilePointerEx, 1);
    ASSERT_EQ(_n_WriteFile, 1);
}

TEST_F(FileWin32Test, WriteAppendSeekFailure)
{
    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, true),
              MB_FILE_OK);

    // Ensure that the seek callback is called
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
    ASSERT_EQ(_n_SetFilePointerEx, 1);
    ASSERT_EQ(_n_WriteFile, 0);
}

TEST_F(FileWin32Test, SeekSuccess)
{
    _vtable.fn_SetFilePointerEx = [](void *userdata, HANDLE hFile,
                                     LARGE_INTEGER liDistanceToMove,
                                     PLARGE_INTEGER lpNewFilePointer,
                                     DWORD dwMoveMethod) -> BOOL {
        (void) hFile;
        (void) liDistanceToMove;
        (void) lpNewFilePointer;
        (void) dwMoveMethod;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetFilePointerEx;

        lpNewFilePointer->QuadPart = 10;
        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    uint64_t offset;
    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, &offset), MB_FILE_OK);
    ASSERT_EQ(offset, 10);
    ASSERT_EQ(_n_SetFilePointerEx, 1);
}

#define LFS_SIZE (10ULL * 1024 * 1024 * 1024)
TEST_F(FileWin32Test, SeekSuccessLargeFile)
{
    _vtable.fn_SetFilePointerEx = [](void *userdata, HANDLE hFile,
                                     LARGE_INTEGER liDistanceToMove,
                                     PLARGE_INTEGER lpNewFilePointer,
                                     DWORD dwMoveMethod) -> BOOL {
        (void) hFile;
        (void) liDistanceToMove;
        (void) lpNewFilePointer;
        (void) dwMoveMethod;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetFilePointerEx;

        lpNewFilePointer->QuadPart = LFS_SIZE;
        return TRUE;
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    // Ensure that the types (off_t, etc.) are large enough for LFS
    uint64_t offset;
    ASSERT_EQ(mb_file_seek(_file, LFS_SIZE, SEEK_SET, &offset), MB_FILE_OK);
    ASSERT_EQ(offset, LFS_SIZE);
    ASSERT_EQ(_n_SetFilePointerEx, 1);
}
#undef LFS_SIZE

TEST_F(FileWin32Test, SeekFailed)
{
    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(_file, 10, SEEK_SET, nullptr), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
    ASSERT_EQ(_n_SetFilePointerEx, 1);
}

TEST_F(FileWin32Test, TruncateSuccess)
{
    _vtable.fn_SetEndOfFile = [](void *userdata, HANDLE hFile) -> BOOL {
        (void) hFile;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetEndOfFile;

        return TRUE;
    };
    _vtable.fn_SetFilePointerEx = [](void *userdata, HANDLE hFile,
                                     LARGE_INTEGER liDistanceToMove,
                                     PLARGE_INTEGER lpNewFilePointer,
                                     DWORD dwMoveMethod) -> BOOL {
        (void) hFile;
        (void) liDistanceToMove;
        (void) lpNewFilePointer;
        (void) dwMoveMethod;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetFilePointerEx;

        switch (test->_n_SetFilePointerEx) {
        case 1:
        case 3:
            lpNewFilePointer->QuadPart = 0;
            return TRUE;
        case 2:
            lpNewFilePointer->QuadPart = 1024;
            return TRUE;
        default:
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_OK);
    ASSERT_EQ(_n_SetEndOfFile, 1);
    ASSERT_EQ(_n_SetFilePointerEx, 3);
}

TEST_F(FileWin32Test, TruncateFailed)
{
    _vtable.fn_SetFilePointerEx = [](void *userdata, HANDLE hFile,
                                     LARGE_INTEGER liDistanceToMove,
                                     PLARGE_INTEGER lpNewFilePointer,
                                     DWORD dwMoveMethod) -> BOOL {
        (void) hFile;
        (void) liDistanceToMove;
        (void) lpNewFilePointer;
        (void) dwMoveMethod;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetFilePointerEx;

        switch (test->_n_SetFilePointerEx) {
        case 1:
        case 3:
            lpNewFilePointer->QuadPart = 0;
            return TRUE;
        case 2:
            lpNewFilePointer->QuadPart = 1024;
            return TRUE;
        default:
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
    ASSERT_EQ(_n_SetEndOfFile, 1);
    ASSERT_EQ(_n_SetFilePointerEx, 3);
}

TEST_F(FileWin32Test, TruncateFirstSeekFailed)
{
    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
    ASSERT_EQ(_n_SetEndOfFile, 0);
    ASSERT_EQ(_n_SetFilePointerEx, 1);
}

TEST_F(FileWin32Test, TruncateSecondSeekFailed)
{
    _vtable.fn_SetFilePointerEx = [](void *userdata, HANDLE hFile,
                                     LARGE_INTEGER liDistanceToMove,
                                     PLARGE_INTEGER lpNewFilePointer,
                                     DWORD dwMoveMethod) -> BOOL {
        (void) hFile;
        (void) liDistanceToMove;
        (void) lpNewFilePointer;
        (void) dwMoveMethod;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetFilePointerEx;

        switch (test->_n_SetFilePointerEx) {
        case 1:
            lpNewFilePointer->QuadPart = 0;
            return TRUE;
        default:
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
    ASSERT_EQ(_n_SetEndOfFile, 0);
    ASSERT_EQ(_n_SetFilePointerEx, 2);
}

TEST_F(FileWin32Test, TruncateThirdSeekFailed)
{
    _vtable.fn_SetEndOfFile = [](void *userdata, HANDLE hFile) -> BOOL {
        (void) hFile;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetEndOfFile;

        return TRUE;
    };
    _vtable.fn_SetFilePointerEx = [](void *userdata, HANDLE hFile,
                                     LARGE_INTEGER liDistanceToMove,
                                     PLARGE_INTEGER lpNewFilePointer,
                                     DWORD dwMoveMethod) -> BOOL {
        (void) hFile;
        (void) liDistanceToMove;
        (void) lpNewFilePointer;
        (void) dwMoveMethod;

        FileWin32Test *test = static_cast<FileWin32Test *>(userdata);
        ++test->_n_SetFilePointerEx;

        switch (test->_n_SetFilePointerEx) {
        case 1:
            lpNewFilePointer->QuadPart = 0;
            return TRUE;
        case 2:
            lpNewFilePointer->QuadPart = 1024;
            return TRUE;
        default:
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    };

    ASSERT_EQ(_mb_file_open_HANDLE(&_vtable, _file, nullptr, true, false),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(_file, 1024), MB_FILE_FATAL);
    ASSERT_EQ(mb_file_error(_file), -ERROR_INVALID_HANDLE);
    ASSERT_EQ(_n_SetEndOfFile, 1);
    ASSERT_EQ(_n_SetFilePointerEx, 3);
}
