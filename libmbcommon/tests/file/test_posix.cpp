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

using namespace mb;
using namespace mb::detail;

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
        ON_CALL(*this, fn_wfopen(testing::_, testing::_))
                .WillByDefault(testing::SetErrnoAndReturn(EIO, nullptr));
#else
        ON_CALL(*this, fn_fopen(testing::_, testing::_))
                .WillByDefault(testing::SetErrnoAndReturn(EIO, nullptr));
#endif
        ON_CALL(*this, fn_fstat(testing::_, testing::_))
                .WillByDefault(testing::SetErrnoAndReturn(EIO, -1));
        ON_CALL(*this, fn_fclose(testing::_))
                .WillByDefault(testing::SetErrnoAndReturn(EIO, -1));
        ON_CALL(*this, fn_ferror(testing::_))
                .WillByDefault(testing::ReturnPointee(&stream_error));
        ON_CALL(*this, fn_fileno(testing::_))
                .WillByDefault(testing::Return(-1));
        ON_CALL(*this, fn_fread(testing::_, testing::_, testing::_, testing::_))
                .WillByDefault(testing::DoAll(
                        testing::InvokeWithoutArgs(
                                this, &MockPosixFileFuncs::set_ferror_fail),
                        testing::SetErrnoAndReturn(EIO, 0)));
        ON_CALL(*this, fn_fseeko(testing::_, testing::_, testing::_))
                .WillByDefault(testing::SetErrnoAndReturn(EIO, -1));
        ON_CALL(*this, fn_ftello(testing::_))
                .WillByDefault(testing::SetErrnoAndReturn(EIO, -1));
        ON_CALL(*this, fn_fwrite(testing::_, testing::_, testing::_, testing::_))
                .WillByDefault(testing::DoAll(
                        testing::InvokeWithoutArgs(
                                this, &MockPosixFileFuncs::set_ferror_fail),
                        testing::SetErrnoAndReturn(EIO, 0)));
        ON_CALL(*this, fn_ftruncate64(testing::_, testing::_))
                .WillByDefault(testing::SetErrnoAndReturn(EIO, -1));
    }

    void set_ferror_fail()
    {
        stream_error = true;
    }

    void open_with_success()
    {
#ifdef _WIN32
        ON_CALL(*this, fn_wfopen(testing::_, testing::_))
                .WillByDefault(testing::Return(g_fp));
#else
        ON_CALL(*this, fn_fopen(testing::_, testing::_))
                .WillByDefault(testing::Return(g_fp));
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

struct FilePosixTest : testing::Test
{
    testing::NiceMock<MockPosixFileFuncs> _funcs;
};

TEST_F(FilePosixTest, OpenFilenameMbsSuccess)
{
    _funcs.open_with_success();

#ifdef _WIN32
    EXPECT_CALL(_funcs, fn_wfopen(testing::_, testing::_))
            .Times(1);
#else
    EXPECT_CALL(_funcs, fn_fopen(testing::_, testing::_))
            .Times(1);
#endif

    TestablePosixFile file(&_funcs);
    ASSERT_TRUE(file.open("x", FileOpenMode::ReadOnly));
}

TEST_F(FilePosixTest, OpenFilenameMbsFailure)
{
#ifdef _WIN32
    EXPECT_CALL(_funcs, fn_wfopen(testing::_, testing::_))
            .Times(1);
#else
    EXPECT_CALL(_funcs, fn_fopen(testing::_, testing::_))
            .Times(1);
#endif

    TestablePosixFile file(&_funcs);
    auto result = file.open("x", FileOpenMode::ReadOnly);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), std::errc::io_error);
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
    EXPECT_CALL(_funcs, fn_wfopen(testing::_, testing::_))
            .Times(1);
#else
    EXPECT_CALL(_funcs, fn_fopen(testing::_, testing::_))
            .Times(1);
#endif

    TestablePosixFile file(&_funcs);
    ASSERT_TRUE(file.open(L"x", FileOpenMode::ReadOnly));
}

TEST_F(FilePosixTest, OpenFilenameWcsFailure)
{
#ifdef _WIN32
    EXPECT_CALL(_funcs, fn_wfopen(testing::_, testing::_))
            .Times(1);
#else
    EXPECT_CALL(_funcs, fn_fopen(testing::_, testing::_))
            .Times(1);
#endif

    TestablePosixFile file(&_funcs);
    auto result = file.open(L"x", FileOpenMode::ReadOnly);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), std::errc::io_error);
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
    EXPECT_CALL(_funcs, fn_fstat(testing::_, testing::_))
            .Times(1);
    EXPECT_CALL(_funcs, fn_fileno(testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));

    TestablePosixFile file(&_funcs);
    auto result = file.open(g_fp, false);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), std::errc::io_error);
}

TEST_F(FilePosixTest, OpenDirectory)
{
    struct stat sb{};
    sb.st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;

    EXPECT_CALL(_funcs, fn_fstat(testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<1>(sb),
                                     testing::Return(0)));
    EXPECT_CALL(_funcs, fn_fileno(testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));

    TestablePosixFile file(&_funcs);
    auto result = file.open(g_fp, false);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), std::errc::is_a_directory);
}

TEST_F(FilePosixTest, OpenFile)
{
    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    EXPECT_CALL(_funcs, fn_fstat(testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(testing::SetArgPointee<1>(sb),
                                     testing::Return(0)));
    EXPECT_CALL(_funcs, fn_fileno(testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));

    TestablePosixFile file(&_funcs);
    ASSERT_TRUE(file.open(g_fp, false));
}

TEST_F(FilePosixTest, CloseUnownedFile)
{
    // Ensure that the close callback is not called
    EXPECT_CALL(_funcs, fn_fclose(testing::_))
            .Times(0);

    TestablePosixFile file(&_funcs, g_fp, false);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST_F(FilePosixTest, CloseOwnedFile)
{
    // Ensure that the close callback is called
    EXPECT_CALL(_funcs, fn_fclose(testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST_F(FilePosixTest, CloseFailure)
{
    // Ensure that the close callback is called
    EXPECT_CALL(_funcs, fn_fclose(testing::_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto result = file.close();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), std::errc::io_error);
}

TEST_F(FilePosixTest, ReadSuccess)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_fread(testing::_, testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::ReturnArg<2>());

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    char c;
    auto n = file.read(&c, 1);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 1u);
}

TEST_F(FilePosixTest, ReadEof)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_fread(testing::_, testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    char c;
    auto n = file.read(&c, 1);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 0u);
}

TEST_F(FilePosixTest, ReadFailure)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_fread(testing::_, testing::_, testing::_, testing::_))
            .Times(1);
    EXPECT_CALL(_funcs, fn_ferror(testing::_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    char c;
    auto n = file.read(&c, 1);
    ASSERT_FALSE(n);
    ASSERT_EQ(n.error(), std::errc::io_error);
}

TEST_F(FilePosixTest, ReadFailureEINTR)
{
    // Ensure that the read callback is called
    EXPECT_CALL(_funcs, fn_fread(testing::_, testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(
                    testing::InvokeWithoutArgs(
                            &_funcs, &MockPosixFileFuncs::set_ferror_fail),
                    testing::SetErrnoAndReturn(EINTR, 0)));
    EXPECT_CALL(_funcs, fn_ferror(testing::_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    char c;
    auto n = file.read(&c, 1);
    ASSERT_FALSE(n);
    ASSERT_EQ(n.error(), std::errc::interrupted);
}

TEST_F(FilePosixTest, WriteSuccess)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_fwrite(testing::_, testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::ReturnArg<2>());

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto n = file.write("x", 1);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 1u);
}

TEST_F(FilePosixTest, WriteEof)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_fwrite(testing::_, testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto n = file.write("x", 1);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 0u);
}

TEST_F(FilePosixTest, WriteFailure)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_fwrite(testing::_, testing::_, testing::_, testing::_))
            .Times(1);
    EXPECT_CALL(_funcs, fn_ferror(testing::_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto n = file.write("x", 1);
    ASSERT_FALSE(n);
    ASSERT_EQ(n.error(), std::errc::io_error);
}

TEST_F(FilePosixTest, WriteFailureEINTR)
{
    // Ensure that the write callback is called
    EXPECT_CALL(_funcs, fn_fwrite(testing::_, testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::DoAll(
                    testing::InvokeWithoutArgs(
                            &_funcs, &MockPosixFileFuncs::set_ferror_fail),
                    testing::SetErrnoAndReturn(EINTR, 0)));
    EXPECT_CALL(_funcs, fn_ferror(testing::_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto n = file.write("x", 1);
    ASSERT_FALSE(n);
    ASSERT_EQ(n.error(), std::errc::interrupted);
}

TEST_F(FilePosixTest, SeekSuccess)
{
    EXPECT_CALL(_funcs, fn_fseeko(testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));
    EXPECT_CALL(_funcs, fn_ftello(testing::_))
            .Times(2)
            .WillOnce(testing::Return(0))
            .WillOnce(testing::Return(10));

    ON_CALL(_funcs, fn_fileno(testing::_))
            .WillByDefault(testing::Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::SetArgPointee<1>(sb),
                                          testing::Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto offset = file.seek(10, SEEK_SET);
    ASSERT_TRUE(offset);
    ASSERT_EQ(offset.value(), 10u);
}

#ifndef __ANDROID__
#define LFS_SIZE (10ULL * 1024 * 1024 * 1024)
TEST_F(FilePosixTest, SeekSuccessLargeFile)
{
    EXPECT_CALL(_funcs, fn_fseeko(testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));
    EXPECT_CALL(_funcs, fn_ftello(testing::_))
            .Times(2)
            .WillRepeatedly(testing::Return(LFS_SIZE));

    ON_CALL(_funcs, fn_fileno(testing::_))
            .WillByDefault(testing::Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::SetArgPointee<1>(sb),
                                          testing::Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    // Ensure that the types (off_t, etc.) are large enough for LFS
    auto offset = file.seek(LFS_SIZE, SEEK_SET);
    ASSERT_TRUE(offset);
    ASSERT_EQ(offset.value(), LFS_SIZE);
}
#undef LFS_SIZE
#endif

TEST_F(FilePosixTest, SeekFseekFailed)
{
    EXPECT_CALL(_funcs, fn_fseeko(testing::_, testing::_, testing::_))
            .Times(1);
    EXPECT_CALL(_funcs, fn_ftello(testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));

    ON_CALL(_funcs, fn_fileno(testing::_))
            .WillByDefault(testing::Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::SetArgPointee<1>(sb),
                                          testing::Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto offset = file.seek(10, SEEK_SET);
    ASSERT_FALSE(offset);
    ASSERT_EQ(offset.error(), std::errc::io_error);
}

TEST_F(FilePosixTest, SeekFtellFailed)
{
    EXPECT_CALL(_funcs, fn_fseeko(testing::_, testing::_, testing::_))
            .Times(0);
    EXPECT_CALL(_funcs, fn_ftello(testing::_))
            .Times(1);

    ON_CALL(_funcs, fn_fileno(testing::_))
            .WillByDefault(testing::Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::SetArgPointee<1>(sb),
                                          testing::Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto offset = file.seek(10, SEEK_SET);
    ASSERT_FALSE(offset);
    ASSERT_EQ(offset.error(), std::errc::io_error);
}

TEST_F(FilePosixTest, SeekSecondFtellFailed)
{
    // fseeko will be called a second time to restore the original position
    EXPECT_CALL(_funcs, fn_fseeko(testing::_, testing::_, testing::_))
            .Times(2)
            .WillRepeatedly(testing::Return(0));
    EXPECT_CALL(_funcs, fn_ftello(testing::_))
            .Times(2)
            .WillOnce(testing::Return(0))
            .WillOnce(testing::SetErrnoAndReturn(EIO, -1));

    ON_CALL(_funcs, fn_fileno(testing::_))
            .WillByDefault(testing::Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::SetArgPointee<1>(sb),
                                          testing::Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto offset = file.seek(10, SEEK_SET);
    ASSERT_FALSE(offset);
    ASSERT_EQ(offset.error(), std::errc::io_error);
}

TEST_F(FilePosixTest, SeekSecondFtellFatal)
{
    // fseeko will be called a second time to restore the original position
    EXPECT_CALL(_funcs, fn_fseeko(testing::_, testing::_, testing::_))
            .Times(2)
            .WillOnce(testing::Return(0))
            .WillOnce(testing::SetErrnoAndReturn(EIO, -1));
    EXPECT_CALL(_funcs, fn_ftello(testing::_))
            .Times(2)
            .WillOnce(testing::Return(0))
            .WillOnce(testing::SetErrnoAndReturn(EIO, -1));

    ON_CALL(_funcs, fn_fileno(testing::_))
            .WillByDefault(testing::Return(0));

    struct stat sb{};
    sb.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

    ON_CALL(_funcs, fn_fstat(testing::_, testing::_))
            .WillByDefault(testing::DoAll(testing::SetArgPointee<1>(sb),
                                          testing::Return(0)));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto offset = file.seek(10, SEEK_SET);
    ASSERT_FALSE(offset);
    ASSERT_EQ(offset.error(), std::errc::io_error);
    ASSERT_TRUE(file.is_fatal());
}

TEST_F(FilePosixTest, SeekUnsupported)
{
    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto offset = file.seek(10, SEEK_SET);
    ASSERT_FALSE(offset);
    ASSERT_EQ(offset.error(), FileError::UnsupportedSeek);
}

TEST_F(FilePosixTest, TruncateSuccess)
{
    // Fail when opening to avoid fstat check
    EXPECT_CALL(_funcs, fn_fileno(testing::_))
            .Times(2)
            .WillOnce(testing::Return(-1))
            .WillOnce(testing::Return(0));
    EXPECT_CALL(_funcs, fn_ftruncate64(testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::Return(0));

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.truncate(1024));
}

TEST_F(FilePosixTest, TruncateUnsupported)
{
    EXPECT_CALL(_funcs, fn_fileno(testing::_))
            .Times(2);
    EXPECT_CALL(_funcs, fn_ftruncate64(testing::_, testing::_))
            .Times(0);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto result = file.truncate(1024);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), FileError::UnsupportedTruncate);
}

TEST_F(FilePosixTest, TruncateFailed)
{
    // Fail when opening to avoid fstat check
    EXPECT_CALL(_funcs, fn_fileno(testing::_))
            .Times(2)
            .WillOnce(testing::Return(-1))
            .WillOnce(testing::Return(0));
    EXPECT_CALL(_funcs, fn_ftruncate64(testing::_, testing::_))
            .Times(1);

    TestablePosixFile file(&_funcs, g_fp, true);
    ASSERT_TRUE(file.is_open());

    auto result = file.truncate(1024);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), std::errc::io_error);
}
