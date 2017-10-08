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

#include "mbcommon/file.h"
#include "mbcommon/file/win32.h"
#include "mbcommon/file/win32_p.h"

template <typename T>
class SetWin32ErrorAndReturnAction
{
public:
    SetWin32ErrorAndReturnAction(int win32_error, T result)
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
testing::PolymorphicAction<SetWin32ErrorAndReturnAction<T>>
SetWin32ErrorAndReturn(int errval, T result)
{
    return testing::MakePolymorphicAction(
            SetWin32ErrorAndReturnAction<T>(errval, result));
}

struct MockWin32FileFuncs : public mb::Win32FileFuncs
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
    MOCK_METHOD1(fn_SetEndOfFile, BOOL(HANDLE hFile));
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
        ON_CALL(*this, fn_CloseHandle(testing::_))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
        ON_CALL(*this, fn_CreateFileW(testing::_, testing::_, testing::_,
                                      testing::_, testing::_, testing::_,
                                      testing::_))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      INVALID_HANDLE_VALUE));
        ON_CALL(*this, fn_ReadFile(testing::_, testing::_, testing::_,
                                   testing::_, testing::_))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
        ON_CALL(*this, fn_SetEndOfFile(testing::_))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
        ON_CALL(*this, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                           testing::_))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
        ON_CALL(*this, fn_WriteFile(testing::_, testing::_, testing::_,
                                    testing::_, testing::_))
                .WillByDefault(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE,
                                                      FALSE));
    }

    void set_failed_win32_error()
    {
    }
};

class TestableWin32FilePrivate : public mb::Win32FilePrivate
{
public:
    TestableWin32FilePrivate(mb::Win32FileFuncs *funcs)
        : mb::Win32FilePrivate(funcs)
    {
    }
};

class TestableWin32File : public mb::Win32File
{
public:
    MB_DECLARE_PRIVATE(TestableWin32File)

    TestableWin32File(mb::Win32FileFuncs *funcs)
        : mb::Win32File(new TestableWin32FilePrivate(funcs))
    {
    }

    TestableWin32File(mb::Win32FileFuncs *funcs, HANDLE handle, bool owned,
                      bool append)
        : mb::Win32File(new TestableWin32FilePrivate(funcs), handle, owned,
                        append)
    {
    }

    TestableWin32File(mb::Win32FileFuncs *funcs,
                      const std::string &filename, mb::FileOpenMode mode)
        : mb::Win32File(new TestableWin32FilePrivate(funcs), filename, mode)
    {
    }

    TestableWin32File(mb::Win32FileFuncs *funcs,
                      const std::wstring &filename, mb::FileOpenMode mode)
        : mb::Win32File(new TestableWin32FilePrivate(funcs), filename, mode)
    {
    }

    ~TestableWin32File()
    {
    }
};

struct FileWin32Test : testing::Test
{
    testing::NiceMock<MockWin32FileFuncs> _funcs;
};

TEST_F(FileWin32Test, OpenFilenameMbsSuccess)
{
    EXPECT_CALL(_funcs, fn_CreateFileW(testing::_, testing::_, testing::_,
                                       testing::_, testing::_, testing::_,
                                       testing::_))
            .Times(1)
            .WillOnce(testing::Return(reinterpret_cast<HANDLE>(1)));

    TestableWin32File file(&_funcs);
    ASSERT_TRUE(file.open("x", mb::FileOpenMode::READ_ONLY));
}

TEST_F(FileWin32Test, OpenFilenameMbsFailure)
{
    EXPECT_CALL(_funcs, fn_CreateFileW(testing::_, testing::_, testing::_,
                                       testing::_, testing::_, testing::_,
                                       testing::_))
            .Times(1);

    TestableWin32File file(&_funcs);
    ASSERT_FALSE(file.open("x", mb::FileOpenMode::READ_ONLY));
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

#ifndef NDEBUG
TEST_F(FileWin32Test, OpenFilenameMbsInvalidMode)
{
    ASSERT_DEATH({
        TestableWin32File file(&_funcs);
        ASSERT_FALSE(file.open("x", static_cast<mb::FileOpenMode>(-1)));
    }, "Invalid mode");
}
#endif

TEST_F(FileWin32Test, OpenFilenameWcsSuccess)
{
    EXPECT_CALL(_funcs, fn_CreateFileW(testing::_, testing::_, testing::_,
                                       testing::_, testing::_, testing::_,
                                       testing::_))
            .Times(1)
            .WillOnce(testing::Return(reinterpret_cast<HANDLE>(1)));

    TestableWin32File file(&_funcs);
    ASSERT_TRUE(file.open(L"x", mb::FileOpenMode::READ_ONLY));
}

TEST_F(FileWin32Test, OpenFilenameWcsFailure)
{
    EXPECT_CALL(_funcs, fn_CreateFileW(testing::_, testing::_, testing::_,
                                       testing::_, testing::_, testing::_,
                                       testing::_))
            .Times(1);

    TestableWin32File file(&_funcs);
    ASSERT_FALSE(file.open(L"x", mb::FileOpenMode::READ_ONLY));
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

#ifndef NDEBUG
TEST_F(FileWin32Test, OpenFilenameWcsInvalidMode)
{
    ASSERT_DEATH({
        TestableWin32File file(&_funcs);
        ASSERT_FALSE(file.open(L"x", static_cast<mb::FileOpenMode>(-1)));
    }, "Invalid mode");
}
#endif

TEST_F(FileWin32Test, CloseUnownedFile)
{
    // Ensure that the close callback is not called
    EXPECT_CALL(_funcs, fn_CloseHandle(testing::_))
            .Times(0);

    TestableWin32File file(&_funcs, nullptr, false, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST_F(FileWin32Test, CloseOwnedFile)
{
    // Ensure that the close callback is called
    EXPECT_CALL(_funcs, fn_CloseHandle(testing::_))
            .Times(1)
            .WillOnce(testing::Return(TRUE));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST_F(FileWin32Test, CloseFailure)
{
    // Ensure that the close callback is called
    EXPECT_CALL(_funcs, fn_CloseHandle(testing::_))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.close());
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, ReadSuccess)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_ReadFile(testing::_, testing::_, testing::_,
                                    testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<3>(1),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    char c;
    size_t n;
    ASSERT_TRUE(file.read(&c, 1, n));
    ASSERT_EQ(n, 1u);
}

#if SIZE_MAX > UINT_MAX
TEST_F(FileWin32Test, ReadSuccessMaxSize)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_ReadFile(testing::_, testing::_, testing::_,
                                    testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<3>(UINT_MAX),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    size_t n;
    ASSERT_TRUE(file.read(, nullptr, static_cast<size_t>(UINT_MAX) + 1, n));
    ASSERT_EQ(n, UINT_MAX);
}
#endif

TEST_F(FileWin32Test, ReadEof)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_ReadFile(testing::_, testing::_, testing::_,
                                    testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<3>(0),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    char c;
    size_t n;
    ASSERT_TRUE(file.read(&c, 1, n));
    ASSERT_EQ(n, 0u);
}

TEST_F(FileWin32Test, ReadFailure)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_ReadFile(testing::_, testing::_, testing::_,
                                    testing::_, testing::_))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    char c;
    size_t n;
    ASSERT_FALSE(file.read(&c, 1, n));
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, WriteSuccess)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_WriteFile(testing::_, testing::_, testing::_,
                                     testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<3>(1),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    size_t n;
    ASSERT_TRUE(file.write("x", 1, n));
    ASSERT_EQ(n, 1u);
}

#if SIZE_MAX > UINT_MAX
TEST_F(FileWin32Test, WriteSuccessMaxSize)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_WriteFile(testing::_, testing::_, testing::_,
                                     testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<3>(UINT_MAX),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    size_t n;
    ASSERT_TRUE(file.write(nullptr, static_cast<size_t>(UINT_MAX) + 1, n));
    ASSERT_EQ(n, UINT_MAX);
}
#endif

TEST_F(FileWin32Test, WriteEof)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_WriteFile(testing::_, testing::_, testing::_,
                                     testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<3>(0),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    size_t n;
    ASSERT_TRUE(file.write("x", 1, n));
    ASSERT_EQ(n, 0u);
}

TEST_F(FileWin32Test, WriteFailure)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_WriteFile(testing::_, testing::_, testing::_,
                                     testing::_, testing::_))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    size_t n;
    ASSERT_FALSE(file.write("x", 1, n));
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, WriteAppendSuccess)
{
    LARGE_INTEGER offset;
    offset.QuadPart = 0;

    // Ensure that the seek and write callbacks are called
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset),
                                     testing::Return(TRUE)));
    EXPECT_CALL(_funcs, fn_WriteFile(testing::_, testing::_, testing::_,
                                     testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<3>(1),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, true);
    ASSERT_TRUE(file.is_open());

    size_t n;
    ASSERT_TRUE(file.write("x", 1, n));
    ASSERT_EQ(n, 1u);
}

TEST_F(FileWin32Test, WriteAppendSeekFailure)
{
    // Ensure that the seek callback is called
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(1);
    EXPECT_CALL(_funcs, fn_WriteFile(testing::_, testing::_, testing::_,
                                     testing::_, testing::_))
            .Times(0);

    TestableWin32File file(&_funcs, nullptr, true, true);
    ASSERT_TRUE(file.is_open());

    size_t n;
    ASSERT_FALSE(file.write("x", 1, n));
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, SeekSuccess)
{
    LARGE_INTEGER offset;
    offset.QuadPart = 10;

    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    uint64_t new_offset;
    ASSERT_TRUE(file.seek(10, SEEK_SET, &new_offset));
    ASSERT_EQ(new_offset, 10u);
}

#define LFS_SIZE (10ULL * 1024 * 1024 * 1024)
TEST_F(FileWin32Test, SeekSuccessLargeFile)
{
    LARGE_INTEGER offset;
    offset.QuadPart = LFS_SIZE;

    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    // Ensure that the types (off_t, etc.) are large enough for LFS
    uint64_t new_offset;
    ASSERT_TRUE(file.seek(LFS_SIZE, SEEK_SET, &new_offset));
    ASSERT_EQ(new_offset, LFS_SIZE);
}
#undef LFS_SIZE

TEST_F(FileWin32Test, SeekFailed)
{
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.seek(10, SEEK_SET, nullptr));
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, TruncateSuccess)
{
    LARGE_INTEGER offset1;
    offset1.QuadPart = 0;
    LARGE_INTEGER offset2;
    offset2.QuadPart = 1024;

    EXPECT_CALL(_funcs, fn_SetEndOfFile(testing::_))
            .Times(1)
            .WillOnce(testing::Return(TRUE));
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(3)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset1),
                                     testing::Return(TRUE)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset2),
                                     testing::Return(TRUE)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset1),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.truncate(1024));
}

TEST_F(FileWin32Test, TruncateFailed)
{
    LARGE_INTEGER offset1;
    offset1.QuadPart = 0;
    LARGE_INTEGER offset2;
    offset2.QuadPart = 1024;

    EXPECT_CALL(_funcs, fn_SetEndOfFile(testing::_))
            .Times(1);
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(3)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset1),
                                     testing::Return(TRUE)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset2),
                                     testing::Return(TRUE)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset1),
                                     testing::Return(TRUE)));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.truncate(1024));
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, TruncateFirstSeekFailed)
{
    EXPECT_CALL(_funcs, fn_SetEndOfFile(testing::_))
            .Times(0);
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(1);

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.truncate(1024));
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, TruncateSecondSeekFailed)
{
    LARGE_INTEGER offset1;
    offset1.QuadPart = 0;

    EXPECT_CALL(_funcs, fn_SetEndOfFile(testing::_))
            .Times(0);
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(2)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset1),
                                     testing::Return(TRUE)))
            .WillRepeatedly(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE, FALSE));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.truncate(1024));
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}

TEST_F(FileWin32Test, TruncateThirdSeekFailed)
{
    LARGE_INTEGER offset1;
    offset1.QuadPart = 0;
    LARGE_INTEGER offset2;
    offset2.QuadPart = 1024;

    EXPECT_CALL(_funcs, fn_SetEndOfFile(testing::_))
            .Times(1)
            .WillOnce(testing::Return(TRUE));
    EXPECT_CALL(_funcs, fn_SetFilePointerEx(testing::_, testing::_, testing::_,
                                            testing::_))
            .Times(3)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset1),
                                     testing::Return(TRUE)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(offset2),
                                     testing::Return(TRUE)))
            .WillRepeatedly(SetWin32ErrorAndReturn(ERROR_INVALID_HANDLE, FALSE));

    TestableWin32File file(&_funcs, nullptr, true, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.truncate(1024));
    ASSERT_TRUE(file.is_fatal());
    ASSERT_EQ(file.error().value(), ERROR_INVALID_HANDLE);
}
