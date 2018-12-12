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

#include "mbcommon/file.h"
#include "mbcommon/file/posix.h"
#include "mbcommon/file_error.h"

using namespace mb;
using namespace mb::detail;
using namespace testing;

// Dummy fp that's never dereferenced
static FILE *g_fp = reinterpret_cast<FILE *>(-1);

struct MockPosixFileFuncs : public PosixFileFuncs
{
    // sys/stat.h
    MOCK_METHOD2(fn_fstat, int(int fildes, struct stat *buf));

    // stdio.h
    MOCK_METHOD1(fn_fclose, int(FILE *stream));
    MOCK_METHOD1(fn_ferror, int(FILE *stream));
    MOCK_METHOD1(fn_fileno, int(FILE *stream));
#ifdef _WIN32
    MOCK_METHOD2(fn_wfopen, FILE *(const wchar_t *filename,
                                   const wchar_t *mode));
#else
    MOCK_METHOD2(fn_fopen, FILE *(const char *path, const char *mode));
#endif
    MOCK_METHOD4(fn_fread, size_t(void *ptr, size_t size, size_t nmemb,
                                  FILE *stream));
    MOCK_METHOD3(fn_fseeko, int(FILE *stream, off_t offset, int whence));
    MOCK_METHOD1(fn_ftello, off_t(FILE *stream));
    MOCK_METHOD4(fn_fwrite, size_t(const void *ptr, size_t size, size_t nmemb,
                                   FILE *stream));

    // unistd.h
    MOCK_METHOD2(fn_ftruncate64, int(int fd, off64_t length));

    bool stream_error = false;

    MockPosixFileFuncs()
    {
        // Fail everything by default
#ifdef _WIN32
        ON_CALL(*this, fn_wfopen(_, _))
                .WillByDefault(SetErrnoAndReturn(EIO, nullptr));
#else
        ON_CALL(*this, fn_fopen(_, _))
                .WillByDefault(SetErrnoAndReturn(EIO, nullptr));
#endif
        ON_CALL(*this, fn_fstat(_, _))
                .WillByDefault(SetErrnoAndReturn(EIO, -1));
        ON_CALL(*this, fn_fclose(_))
                .WillByDefault(SetErrnoAndReturn(EIO, -1));
        ON_CALL(*this, fn_ferror(_))
                .WillByDefault(ReturnPointee(&stream_error));
        ON_CALL(*this, fn_fileno(_))
                .WillByDefault(Return(-1));
        ON_CALL(*this, fn_fread(_, _, _, _))
                .WillByDefault(DoAll(
                        InvokeWithoutArgs(
                                this, &MockPosixFileFuncs::set_ferror_fail),
                        SetErrnoAndReturn(EIO, 0)));
        ON_CALL(*this, fn_fseeko(_, _, _))
                .WillByDefault(SetErrnoAndReturn(EIO, -1));
        ON_CALL(*this, fn_ftello(_))
                .WillByDefault(SetErrnoAndReturn(EIO, -1));
        ON_CALL(*this, fn_fwrite(_, _, _, _))
                .WillByDefault(DoAll(
                        InvokeWithoutArgs(
                                this, &MockPosixFileFuncs::set_ferror_fail),
                        SetErrnoAndReturn(EIO, 0)));
        ON_CALL(*this, fn_ftruncate64(_, _))
                .WillByDefault(SetErrnoAndReturn(EIO, -1));
    }

    void set_ferror_fail()
    {
        stream_error = true;
    }

    void open_with_success()
    {
#ifdef _WIN32
        ON_CALL(*this, fn_wfopen(_, _))
                .WillByDefault(Return(g_fp));
#else
        ON_CALL(*this, fn_fopen(_, _))
                .WillByDefault(Return(g_fp));
#endif
    }
};

class TestablePosixFile : public PosixFile
{
public:
    TestablePosixFile(PosixFileFuncs *funcs)
        : PosixFile(funcs)
    {
    }

    TestablePosixFile(PosixFileFuncs *funcs, FILE *fp, bool owned)
        : PosixFile(funcs, fp, owned)
    {
    }

    TestablePosixFile(PosixFileFuncs *funcs,
                      const std::string &filename, FileOpenMode mode)
        : PosixFile(funcs, filename, mode)
    {
    }

    TestablePosixFile(PosixFileFuncs *funcs,
                      const std::wstring &filename, FileOpenMode mode)
        : PosixFile(funcs, filename, mode)
    {
    }

    ~TestablePosixFile()
    {
    }
};

struct FilePosixTest : Test
{
    NiceMock<MockPosixFileFuncs> _funcs;
};

TEST_F(FilePosixTest, CheckInvalidStates)
{
    TestablePosixFile file(&_funcs);

    auto error = oc::failure(FileError::InvalidState);

    ASSERT_EQ(file.close(), error);
    ASSERT_EQ(file.read(nullptr, 0), error);
    ASSERT_EQ(file.write(nullptr, 0), error);
    ASSERT_EQ(file.seek(0, SEEK_SET), error);
    ASSERT_EQ(file.truncate(1024), error);

    _funcs.open_with_success();

#ifdef _WIN32
    EXPECT_CALL(_funcs, fn_wfopen(_, _))
            .Times(1);
#else
    EXPECT_CALL(_funcs, fn_fopen(_, _))
            .Times(1);
#endif

    ASSERT_TRUE(file.open("x", FileOpenMode::ReadOnly));
    ASSERT_EQ(file.open("x", FileOpenMode::ReadOnly), error);
    ASSERT_EQ(file.open(L"x", FileOpenMode::ReadOnly), error);
    ASSERT_EQ(file.open(g_fp, false), error);
}

TEST_F(FilePosixTest, OpenFilenameMbsSuccess)
{
    _funcs.open_with_success();

#ifdef _WIN32
    EXPECT_CALL(_funcs, fn_wfopen(_, _))
            .Times(1);
#else
    EXPECT_CALL(_funcs, fn_fopen(_, _))
            .Times(1);
#endif

    TestablePosixFile file(&_funcs);
    ASSERT_TRUE(file.open("x", FileOpenMode::ReadOnly));
}

TEST_F(FilePosixTest, OpenFilenameMbsFailure)
{
#ifdef _WIN32
    EXPECT_CALL(_funcs, fn_wfopen(_, _))
            .Times(1);
#else
    EXPECT_CALL(_funcs, fn_fopen(_, _))
            .Times(1);
#endif

    TestablePosixFile file(&_funcs);
    ASSERT_EQ(file.open("x", FileOpenMode::ReadOnly),
              oc::failure(std::errc::io_error));
}

#ifndef NDEBUG
TEST_F(FilePosixTest, OpenFilenameMbsInvalidMode)
{
    ASSERT_DEATH({
        TestablePosixFile file(&_funcs);
        ASSERT_FALSE(file.open("x", static_cast<FileOpenMode>(-1)));
    }, "Invalid mode");
}
#endif

TEST_F(FilePosixTest, OpenFilenameWcsSuccess)
{
    _funcs.open_with_success();

#ifdef _WIN32
    EXPECT_CALL(_funcs, fn_wfopen(_, _))
            .Times(1);
#else
    EXPECT_CALL(_funcs, fn_fopen(_, _))
            .Times(1);
#endif

    TestablePosixFile file(&_funcs);
    ASSERT_TRUE(file.open(L"x", FileOpenMode::ReadOnly));
}

TEST_F(FilePosixTest, OpenFilenameWcsFailure)
{
#ifdef _WIN32
    EXPECT_CALL(_funcs, fn_wfopen(_, _))
            .Times(1);
#else
    EXPECT_CALL(_funcs, fn_fopen(_, _))
            .Times(1);
#endif

    TestablePosixFile file(&_funcs);
    ASSERT_EQ(file.open(L"x", FileOpenMode::ReadOnly),
              oc::failure(std::errc::io_error));
}

#ifndef NDEBUG
TEST_F(FilePosixTest, OpenFilenameWcsInvalidMode)
{
    ASSERT_DEATH({
        TestablePosixFile file(&_funcs);
        ASSERT_FALSE(file.open(L"x", static_cast<FileOpenMode>(-1)));
    }, "Invalid mode");
}
#endif

TEST_F(FilePosixTest, OpenFstatFailed)
{
    EXPECT_CALL(_funcs, fn_fstat(_, _))
            .Times(1);
    EXPECT_CALL(_funcs, fn_fileno(_))
            .Times(1)
            .WillOnce(Return(0));

    TestablePosixFile file(&_funcs);
    ASSERT_EQ(file.open(g_fp, false), oc::failure(std::errc::io_error));
}

TEST_F(FilePosixTest, OpenDirectory)
{
    struct stat sb{};
    sb.st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;

    EXPECT_CALL(_funcs, fn_fstat(_, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<1>(sb), Return(0)));
    EXPECT_CALL(_funcs, fn_fileno(_))
            .Times(1)
            .WillOnce(Return(0));

    TestablePosixFile file(&_funcs);
    ASSERT_EQ(file.open(g_fp, false), oc::failure(std::errc::is_a_directory));
}

TEST_F(FilePosixTest, OpenFile)
{
    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    EXPECT_CALL(_funcs, fn_fstat(_, _))
            .Times(1)
            .WillOnce(DoAll(SetArgPointee<1>(sb), Return(0)));
    EXPECT_CALL(_funcs, fn_fileno(_))
            .Times(1)
            .WillOnce(Return(0));

    TestablePosixFile file(&_funcs);
    ASSERT_TRUE(file.open(g_fp, false));
}

TEST_F(FilePosixTest, CloseUnownedFile)
{
    // Ensure that the close callback is not called
    EXPECT_CALL(_funcs, fn_fclose(_))
            .Times(0);

    TestablePosixFile file(&_funcs, g_fp, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST_F(FilePosixTest, CloseOwnedFile)
{
    // Ensure that the close callback is called
    EXPECT_CALL(_funcs, fn_fclose(_))
            .Times(1)
            .WillOnce(Return(0));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST_F(FilePosixTest, CloseFailure)
{
    // Ensure that the close callback is called
    EXPECT_CALL(_funcs, fn_fclose(_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.close(), oc::failure(std::errc::io_error));
}

TEST_F(FilePosixTest, ReadSuccess)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_fread(_, _, _, _))
            .Times(1)
            .WillOnce(ReturnArg<2>());

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    char c;
    ASSERT_EQ(file.read(&c, 1), oc::success(1u));
}

TEST_F(FilePosixTest, ReadEof)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_fread(_, _, _, _))
            .Times(1)
            .WillOnce(Return(0));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    char c;
    ASSERT_EQ(file.read(&c, 1), oc::success(0u));
}

TEST_F(FilePosixTest, ReadFailure)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_fread(_, _, _, _))
            .Times(1);
    EXPECT_CALL(_funcs, fn_ferror(_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    char c;
    ASSERT_EQ(file.read(&c, 1), oc::failure(std::errc::io_error));
}

TEST_F(FilePosixTest, ReadFailureEINTR)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_fread(_, _, _, _))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeWithoutArgs(
                            &_funcs, &MockPosixFileFuncs::set_ferror_fail),
                    SetErrnoAndReturn(EINTR, 0)));
    EXPECT_CALL(_funcs, fn_ferror(_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    char c;
    ASSERT_EQ(file.read(&c, 1), oc::failure(std::errc::interrupted));
}

TEST_F(FilePosixTest, WriteSuccess)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_fwrite(_, _, _, _))
            .Times(1)
            .WillOnce(ReturnArg<2>());

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1), oc::success(1u));
}

TEST_F(FilePosixTest, WriteEof)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_fwrite(_, _, _, _))
            .Times(1)
            .WillOnce(Return(0));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1), oc::success(0u));
}

TEST_F(FilePosixTest, WriteFailure)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_fwrite(_, _, _, _))
            .Times(1);
    EXPECT_CALL(_funcs, fn_ferror(_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1), oc::failure(std::errc::io_error));
}

TEST_F(FilePosixTest, WriteFailureEINTR)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_fwrite(_, _, _, _))
            .Times(1)
            .WillOnce(DoAll(
                    InvokeWithoutArgs(
                            &_funcs, &MockPosixFileFuncs::set_ferror_fail),
                    SetErrnoAndReturn(EINTR, 0)));
    EXPECT_CALL(_funcs, fn_ferror(_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1), oc::failure(std::errc::interrupted));
}

TEST_F(FilePosixTest, SeekSuccess)
{
    EXPECT_CALL(_funcs, fn_fseeko(_, _, _))
            .Times(1)
            .WillOnce(Return(0));
    EXPECT_CALL(_funcs, fn_ftello(_))
            .Times(1)
            .WillOnce(Return(10));

    ON_CALL(_funcs, fn_fileno(_))
            .WillByDefault(Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(_, _))
            .WillByDefault(DoAll(SetArgPointee<1>(sb), Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.seek(10, SEEK_SET), oc::success(10u));
}

#ifndef __ANDROID__
#define LFS_SIZE (10ULL * 1024 * 1024 * 1024)
TEST_F(FilePosixTest, SeekSuccessLargeFile)
{
    EXPECT_CALL(_funcs, fn_fseeko(_, _, _))
            .Times(1)
            .WillOnce(Return(0));
    EXPECT_CALL(_funcs, fn_ftello(_))
            .Times(1)
            .WillOnce(Return(LFS_SIZE));

    ON_CALL(_funcs, fn_fileno(_))
            .WillByDefault(Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(_, _))
            .WillByDefault(DoAll(SetArgPointee<1>(sb), Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    // Ensure that the types (off_t, etc.) are large enough for LFS
    ASSERT_EQ(file.seek(LFS_SIZE, SEEK_SET), oc::success(LFS_SIZE));
}
#undef LFS_SIZE
#endif

TEST_F(FilePosixTest, SeekFseekFailed)
{
    EXPECT_CALL(_funcs, fn_fseeko(_, _, _))
            .Times(1);
    EXPECT_CALL(_funcs, fn_ftello(_))
            .Times(0);

    ON_CALL(_funcs, fn_fileno(_))
            .WillByDefault(Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(_, _))
            .WillByDefault(DoAll(SetArgPointee<1>(sb), Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.seek(10, SEEK_SET), oc::failure(std::errc::io_error));
}

TEST_F(FilePosixTest, SeekFtellFailed)
{
    EXPECT_CALL(_funcs, fn_fseeko(_, _, _))
            .Times(1)
            .WillOnce(Return(0));
    EXPECT_CALL(_funcs, fn_ftello(_))
            .Times(1);

    ON_CALL(_funcs, fn_fileno(_))
            .WillByDefault(Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(_, _))
            .WillByDefault(DoAll(SetArgPointee<1>(sb), Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.seek(10, SEEK_SET), oc::failure(std::errc::io_error));
}

TEST_F(FilePosixTest, SeekUnsupported)
{
    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.seek(10, SEEK_SET), oc::failure(FileError::UnsupportedSeek));
}

TEST_F(FilePosixTest, TruncateSuccess)
{
    // Fail when opening to avoid fstat check
    EXPECT_CALL(_funcs, fn_fileno(_))
            .Times(2)
            .WillOnce(Return(-1))
            .WillOnce(Return(0));
    EXPECT_CALL(_funcs, fn_ftruncate64(_, _))
            .Times(1)
            .WillOnce(Return(0));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.truncate(1024));
}

TEST_F(FilePosixTest, TruncateUnsupported)
{
    EXPECT_CALL(_funcs, fn_fileno(_))
            .Times(2);
    EXPECT_CALL(_funcs, fn_ftruncate64(_, _))
            .Times(0);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.truncate(1024), oc::failure(FileError::UnsupportedTruncate));
}

TEST_F(FilePosixTest, TruncateFailed)
{
    // Fail when opening to avoid fstat check
    EXPECT_CALL(_funcs, fn_fileno(_))
            .Times(2)
            .WillOnce(Return(-1))
            .WillOnce(Return(0));
    EXPECT_CALL(_funcs, fn_ftruncate64(_, _))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.truncate(1024), oc::failure(std::errc::io_error));
}
