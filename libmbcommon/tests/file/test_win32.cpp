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

#include <gmock/gmock.h>

#include <climits>

#include "mbcommon/error_code.h"
#include "mbcommon/file.h"
#include "mbcommon/file/win32.h"
#include "mbcommon/file_error.h"

using namespace mb;
using namespace mb::detail;
using namespace testing;

template <typename T>
class SetWin32ErrorAndReturnAction
{
public:
    SetWin32ErrorAndReturnAction(DWORD win32_error, T result)
        : _error(win32_error), _result(result)
    {
    }

    template <typename Result, typename ArgumentTuple>
    Result Perform(const ArgumentTuple &args) const
    {
        (void) args;

        SetLastError(_error);
        return _result;
    }

private:
    const DWORD _error;
    const T _result;

    GTEST_DISALLOW_ASSIGN_(SetWin32ErrorAndReturnAction);
};

template <typename T>
PolymorphicAction<SetWin32ErrorAndReturnAction<T>>
SetWin32ErrorAndReturn(DWORD errval, T result)
{
    return MakePolymorphicAction(
            SetWin32ErrorAndReturnAction<T>(errval, result));
}

struct MockWin32FileFuncs : public Win32FileFuncs
{
    // windows.h
    MOCK_METHOD1(fn_CloseHandle, BOOL(HANDLE hObject));
    MOCK_METHOD7(fn_CreateFileW, HANDLE(LPCWSTR lpFileName,
                                        DWORD dwDesiredAccess,
                                        DWORD dwShareMode,
                                        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                        DWORD dwCreationDisposition,
                                        DWORD dwFlagsAndAttributes,
                                        HANDLE hTemplateFile));
    MOCK_METHOD5(fn_ReadFile, BOOL(HANDLE hFile,
                                   LPVOID lpBuffer,
                                   DWORD nNumberOfBytesToRead,
                                   LPDWORD lpNumberOfBytesRead,
                                   LPOVERLAPPED lpOverlapped));
    MOCK_METHOD4(fn_SetFileInformationByHandle, BOOL(HANDLE hFile,
                                                     FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
                                                     LPVOID lpFileInformation,
                                                     DWORD dwBufferSize));
    MOCK_METHOD4(fn_SetFilePointerEx, BOOL(HANDLE hFile,
                                           LARGE_INTEGER liDistanceToMove,
                                           PLARGE_INTEGER lpNewFilePointer,
                                           DWORD dwMoveMethod));
    MOCK_METHOD5(fn_WriteFile, BOOL(HANDLE hFile,
                                    LPCVOID lpBuffer,
                                    DWORD nNumberOfBytesToWrite,
                                    LPDWORD lpNumberOfBytesWritten,
                                    LPOVERLAPPED lpOverlapped));

    MockWin32FileFuncs()
    {
        // Fail everything by default
        ON_CALL(*this, fn_CloseHandle(_))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
        ON_CALL(*this, fn_CreateFileW(_, _, _, _, _, _, _))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      INVALID_HANDLE_VALUE));
        ON_CALL(*this, fn_ReadFile(_, _, _, _, _))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
        ON_CALL(*this, fn_SetFileInformationByHandle(_, _, _, _))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
        ON_CALL(*this, fn_SetFilePointerEx(_, _, _, _))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
        ON_CALL(*this, fn_WriteFile(_, _, _, _, _))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
    }
};

class TestableWin32File : public Win32File
{
public:
    TestableWin32File(Win32FileFuncs *funcs)
        : Win32File(funcs)
    {
    }

    TestableWin32File(Win32FileFuncs *funcs, HANDLE handle, bool owned,
                      bool append)
        : Win32File(funcs, handle, owned, append)
    {
    }

    TestableWin32File(Win32FileFuncs *funcs,
                      const std::string &filename, FileOpenMode mode)
        : Win32File(funcs, filename, mode)
    {
    }

    TestableWin32File(Win32FileFuncs *funcs,
                      const std::wstring &filename, FileOpenMode mode)
        : Win32File(funcs, filename, mode)
    {
    }

    ~TestableWin32File()
    {
    }
};

struct FileWin32Test : Test
{
    NiceMock<MockWin32FileFuncs> _funcs;
};

TEST_F(FileWin32Test, CheckInvalidStates)
{
    TestableWin32File file(&_funcs);

    auto error = oc::failure(FileError::InvalidState);

    ASSERT_EQ(file.close(), error);
    ASSERT_EQ(file.read(nullptr, 0), error);
    ASSERT_EQ(file.write(nullptr, 0), error);
    ASSERT_EQ(file.seek(0, SEEK_SET), error);
    ASSERT_EQ(file.truncate(1024), error);

    EXPECT_CALL(_funcs, fn_CreateFileW(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(reinterpret_cast<HANDLE>(1)));

    ASSERT_TRUE(file.open("x", FileOpenMode::ReadOnly));
    ASSERT_EQ(file.open("x", FileOpenMode::ReadOnly), error);
    ASSERT_EQ(file.open(L"x", FileOpenMode::ReadOnly), error);
    ASSERT_EQ(file.open(nullptr, false, false), error);
}

TEST_F(FileWin32Test, OpenFilenameMbsSuccess)
{
    EXPECT_CALL(_funcs, fn_CreateFileW(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(reinterpret_cast<HANDLE>(1)));

    TestableWin32File file(&_funcs);
    ASSERT_TRUE(file.open("x", FileOpenMode::ReadOnly));
}

TEST_F(FileWin32Test, OpenFilenameMbsFailure)
{
    EXPECT_CALL(_funcs, fn_CreateFileW(_, _, _, _, _, _, _))
            .Times(1);

    TestableWin32File file(&_funcs);
    ASSERT_EQ(file.open("x", FileOpenMode::ReadOnly),
              oc::failure(ec_from_win32(ERROR_INVALID_HANDLE)));
}

#ifndef NDEBUG
TEST_F(FileWin32Test, OpenFilenameMbsInvalidMode)
{
    ASSERT_DEATH({
        TestableWin32File file(&_funcs);
        ASSERT_FALSE(file.open("x", static_cast<FileOpenMode>(-1)));
    }, "Invalid mode");
}
#endif

TEST_F(FileWin32Test, OpenFilenameWcsSuccess)
{
    EXPECT_CALL(_funcs, fn_CreateFileW(_, _, _, _, _, _, _))
            .Times(1)
            .WillOnce(Return(reinterpret_cast<HANDLE>(1)));

    TestableWin32File file(&_funcs);
    ASSERT_TRUE(file.open(L"x", FileOpenMode::ReadOnly));
}

TEST_F(FileWin32Test, OpenFilenameWcsFailure)
{
    EXPECT_CALL(_funcs, fn_CreateFileW(_, _, _, _, _, _, _))
            .Times(1);

    TestableWin32File file(&_funcs);
    ASSERT_EQ(file.open(L"x", FileOpenMode::ReadOnly),
              oc::failure(ec_from_win32(ERROR_INVALID_HANDLE)));
}

#ifndef NDEBUG
TEST_F(FileWin32Test, OpenFilenameWcsInvalidMode)
{
    ASSERT_DEATH({
        TestableWin32File file(&_funcs);
        ASSERT_FALSE(file.open(L"x", static_cast<FileOpenMode>(-1)));
    }, "Invalid mode");
}
#endif

TEST_F(FileWin32Test, CloseUnownedFile)
{
    // Ensure that the close callback is not called
    EXPECT_CALL(_funcs, fn_CloseHandle(_))
            .Times(0);

    TestableWin32File file(&_funcs, nullptr, false, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST_F(FileWin32Test, CloseOwnedFile)
{
    // Ensure that the close callback is called
    EXPECT_CALL(_funcs, fn_CloseHandle(_))
            .Times(1)
            .WillOnce(Return(TRUE));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST_F(FileWin32Test, CloseFailure)
{
    // Ensure that the close callback is called
    EXPECT_CALL(_funcs, fn_CloseHandle(_))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.close(), oc::failure(ec_from_win32(ERROR_INVALID_HANDLE)));
}

TEST_F(FileWin32Test, ReadSuccess)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_ReadFile(_, _, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<3>(1), Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    char c;
    ASSERT_EQ(file.read(&c, 1), oc::success(1u));
}

#if SIZE_MAX > UINT_MAX
TEST_F(FileWin32Test, ReadSuccessMaxSize)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_ReadFile(_, _, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<3>(UINT_MAX), Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.read(nullptr, static_cast<size_t>(UINT_MAX) + 1),
              oc::success(UINT_MAX));
}
#endif

TEST_F(FileWin32Test, ReadEof)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_ReadFile(_, _, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<3>(0), Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    char c;
    ASSERT_EQ(file.read(&c, 1), oc::success(0u));
}

TEST_F(FileWin32Test, ReadFailure)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_ReadFile(_, _, _, _, _))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    char c;
    ASSERT_EQ(file.read(&c, 1),
              oc::failure(ec_from_win32(ERROR_INVALID_HANDLE)));
}

TEST_F(FileWin32Test, WriteSuccess)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_WriteFile(_, _, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<3>(1), Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1), oc::success(1u));
}

#if SIZE_MAX > UINT_MAX
TEST_F(FileWin32Test, WriteSuccessMaxSize)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_WriteFile(_, _, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<3>(UINT_MAX), Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write(nullptr, static_cast<size_t>(UINT_MAX) + 1),
              oc::success(UINT_MAX));
}
#endif

TEST_F(FileWin32Test, WriteEof)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_WriteFile(_, _, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<3>(0), Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1), oc::success(0u));
}

TEST_F(FileWin32Test, WriteFailure)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_WriteFile(_, _, _, _, _))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1),
              oc::failure(ec_from_win32(ERROR_INVALID_HANDLE)));
}

TEST_F(FileWin32Test, WriteAppendSuccess)
{
    LARGE_INTEGER offset;
    offset.QuadPart = 0;

    // Ensure that the seek and write callbacks are called
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(_, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<2>(offset), Return(TRUE)));
    EXPECT_CALL(_funcs, fn_WriteFile(_, _, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<3>(1), Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1), oc::success(1u));
}

TEST_F(FileWin32Test, WriteAppendSeekFailure)
{
    // Ensure that the seek callback is called
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(_, _, _, _))
            .Times(1);
    EXPECT_CALL(_funcs, fn_WriteFile(_, _, _, _, _))
            .Times(0);

    TestableWin32File file(&_funcs, nullptr, true, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1),
              oc::failure(ec_from_win32(ERROR_INVALID_HANDLE)));
}

TEST_F(FileWin32Test, SeekSuccess)
{
    LARGE_INTEGER offset;
    offset.QuadPart = 10;

    EXPECT_CALL(_funcs, fn_SetFilePointerEx(_, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<2>(offset), Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.seek(10, SEEK_SET), oc::success(10u));
}

#define LFS_SIZE (10ULL * 1024 * 1024 * 1024)
TEST_F(FileWin32Test, SeekSuccessLargeFile)
{
    LARGE_INTEGER offset;
    offset.QuadPart = LFS_SIZE;

    EXPECT_CALL(_funcs, fn_SetFilePointerEx(_, _, _, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<2>(offset), Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    // Ensure that the types (off_t, etc.) are large enough for LFS
    ASSERT_EQ(file.seek(LFS_SIZE, SEEK_SET), oc::success(LFS_SIZE));
}
#undef LFS_SIZE

TEST_F(FileWin32Test, SeekFailed)
{
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(_, _, _, _))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.seek(10, SEEK_SET),
              oc::failure(ec_from_win32(ERROR_INVALID_HANDLE)));
}

TEST_F(FileWin32Test, TruncateSuccess)
{
    EXPECT_CALL(_funcs, fn_SetFileInformationByHandle(_, _, _, _))
            .Times(1)
            .WillOnce(Return(TRUE));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.truncate(1024));
}

TEST_F(FileWin32Test, TruncateFailed)
{
    EXPECT_CALL(_funcs, fn_SetFileInformationByHandle(_, _, _, _))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.truncate(1024),
              oc::failure(ec_from_win32(ERROR_INVALID_HANDLE)));
}
