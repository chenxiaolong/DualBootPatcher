/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>

#include <cinttypes>

#include "mbcommon/file/memory.h"
#include "mbcommon/file_p.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "file/mock_test_file.h"


struct FileUtilTest : testing::Test
{
    testing::NiceMock<MockTestFile> _file;
};

TEST_F(FileUtilTest, ReadFullyNormal)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_, testing::_))
            .Times(5)
            .WillRepeatedly(testing::DoAll(testing::SetArgPointee<2>(2),
                                           testing::Return(mb::FileStatus::OK)));

    // Open file
    ASSERT_EQ(_file.open(), mb::FileStatus::OK);

    char buf[10];
    size_t n;
    ASSERT_EQ(mb::file_read_fully(_file, buf, sizeof(buf), &n),
              mb::FileStatus::OK);
    ASSERT_EQ(n, 10u);
}

TEST_F(FileUtilTest, ReadFullyEOF)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(0),
                                     testing::Return(mb::FileStatus::OK)));

    // Open file
    ASSERT_EQ(_file.open(), mb::FileStatus::OK);

    char buf[10];
    size_t n;
    ASSERT_EQ(mb::file_read_fully(_file, buf, sizeof(buf), &n),
              mb::FileStatus::OK);
    ASSERT_EQ(n, 8u);
}

TEST_F(FileUtilTest, ReadFullyPartialFail)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(0),
                                     testing::Return(mb::FileStatus::FAILED)));

    // Open file
    ASSERT_EQ(_file.open(), mb::FileStatus::OK);

    char buf[10];
    size_t n;
    ASSERT_EQ(mb::file_read_fully(_file, buf, sizeof(buf), &n),
              mb::FileStatus::FAILED);
    ASSERT_EQ(n, 8u);
}

TEST_F(FileUtilTest, WriteFullyNormal)
{
    EXPECT_CALL(_file, on_write(testing::_, testing::_, testing::_))
            .Times(5)
            .WillRepeatedly(testing::DoAll(testing::SetArgPointee<2>(2),
                                           testing::Return(mb::FileStatus::OK)));

    // Open file
    ASSERT_EQ(_file.open(), mb::FileStatus::OK);

    size_t n;
    ASSERT_EQ(mb::file_write_fully(_file, "xxxxxxxxxx", 10, &n),
              mb::FileStatus::OK);
    ASSERT_EQ(n, 10u);
}

TEST_F(FileUtilTest, WriteFullyEOF)
{
    EXPECT_CALL(_file, on_write(testing::_, testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(0),
                                     testing::Return(mb::FileStatus::OK)));

    // Open file
    ASSERT_EQ(_file.open(), mb::FileStatus::OK);

    size_t n;
    ASSERT_EQ(mb::file_write_fully(_file, "xxxxxxxxxx", 10, &n),
              mb::FileStatus::OK);
    ASSERT_EQ(n, 8u);
}

TEST_F(FileUtilTest, WriteFullyPartialFail)
{
    EXPECT_CALL(_file, on_write(testing::_, testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(0),
                                     testing::Return(mb::FileStatus::FAILED)));

    // Open file
    ASSERT_EQ(_file.open(), mb::FileStatus::OK);

    size_t n;
    ASSERT_EQ(mb::file_write_fully(_file, "xxxxxxxxxx", 10, &n),
              mb::FileStatus::FAILED);
    ASSERT_EQ(n, 8u);
}

TEST_F(FileUtilTest, ReadDiscardNormal)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_, testing::_))
            .Times(5)
            .WillRepeatedly(testing::DoAll(testing::SetArgPointee<2>(2),
                                           testing::Return(mb::FileStatus::OK)));

    // Open file
    ASSERT_EQ(_file.open(), mb::FileStatus::OK);

    uint64_t n;
    ASSERT_EQ(mb::file_read_discard(_file, 10, &n), mb::FileStatus::OK);
    ASSERT_EQ(n, 10u);
}

TEST_F(FileUtilTest, ReadDiscardEOF)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(0),
                                     testing::Return(mb::FileStatus::OK)));

    // Open file
    ASSERT_EQ(_file.open(), mb::FileStatus::OK);

    uint64_t n;
    ASSERT_EQ(mb::file_read_discard(_file, 10, &n), mb::FileStatus::OK);
    ASSERT_EQ(n, 8u);
}

TEST_F(FileUtilTest, ReadDiscardPartialFail)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(2),
                                     testing::Return(mb::FileStatus::OK)))
            .WillOnce(testing::DoAll(testing::SetArgPointee<2>(0),
                                     testing::Return(mb::FileStatus::FAILED)));

    // Open file
    ASSERT_EQ(_file.open(), mb::FileStatus::OK);

    uint64_t n;
    ASSERT_EQ(mb::file_read_discard(_file, 10, &n), mb::FileStatus::FAILED);
    ASSERT_EQ(n, 8u);
}

struct FileSearchTest : testing::Test
{
    // Callback counters
    int _n_result = 0;

    static mb::FileStatus _result_cb(mb::File &file, void *userdata,
                                     uint64_t offset)
    {
        (void) file;
        (void) offset;

        FileSearchTest *test = static_cast<FileSearchTest *>(userdata);
        ++test->_n_result;

        return mb::FileStatus::OK;
    }
};

TEST_F(FileSearchTest, CheckInvalidBoundariesFail)
{
    mb::MemoryFile file("", 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_search(file, 20, 10, 0, "x", 1, -1, &_result_cb, this),
              mb::FileStatus::FAILED);
    ASSERT_EQ(file.error(), mb::FileError::INVALID_ARGUMENT);
    ASSERT_NE(file.error_string().find("offset"), std::string::npos);
}

TEST_F(FileSearchTest, CheckZeroMaxMatches)
{
    mb::MemoryFile file("", 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_search(file, -1, -1, 0, "x", 1, 0, &_result_cb, this),
              mb::FileStatus::OK);
}

TEST_F(FileSearchTest, CheckZeroPatternSize)
{
    mb::MemoryFile file("", 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_search(file, -1, -1, 0, nullptr, 0, -1,
                              &_result_cb, this), mb::FileStatus::OK);
}

TEST_F(FileSearchTest, CheckBufferSize)
{
    mb::MemoryFile file("", 0);
    ASSERT_TRUE(file.is_open());

    // Auto buffer size
    ASSERT_EQ(mb::file_search(file, -1, -1, 0, "x", 1, -1,
                              &_result_cb, this), mb::FileStatus::OK);

    // Too small
    ASSERT_EQ(mb::file_search(file, -1, -1, 1, "xxx", 3, -1,
                              &_result_cb, this), mb::FileStatus::FAILED);
    ASSERT_EQ(file.error(), mb::FileError::INVALID_ARGUMENT);
    ASSERT_NE(file.error_string().find("Buffer size"), std::string::npos);

    // Equal to pattern size
    ASSERT_EQ(mb::file_search(file, -1, -1, 1, "x", 1, -1,
                              &_result_cb, this), mb::FileStatus::OK);
}

TEST_F(FileSearchTest, FindNormal)
{
    mb::MemoryFile file("abc", 3);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_search(file, -1, -1, 0, "a", 1, -1, &_result_cb, this),
              mb::FileStatus::OK);
}

TEST(FileMoveTest, DegenerateCasesShouldSucceed)
{
    constexpr char buf[] = "abcdef";
    uint64_t n;

    mb::MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    // src == dest
    ASSERT_EQ(mb::file_move(file, 0, 0, 3, &n), mb::FileStatus::OK);

    // size == 0
    ASSERT_EQ(mb::file_move(file, 3, 0, 0, &n), mb::FileStatus::OK);
}

TEST(FileMoveTest, NormalForwardsCopyShouldSucceed)
{
    constexpr char buf[] = "abcdef";
    uint64_t n;

    mb::MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_move(file, 2, 0, 3, &n), mb::FileStatus::OK);
    ASSERT_EQ(n, 3u);
    ASSERT_STREQ(buf, "cdedef");
}

TEST(FileMoveTest, NormalBackwardsCopyShouldSucceed)
{
    constexpr char buf[] = "abcdef";
    uint64_t n;

    mb::MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_move(file, 0, 2, 3, &n), mb::FileStatus::OK);
    ASSERT_EQ(n, 3u);
    ASSERT_STREQ(buf, "ababcf");
}

TEST(FileMoveTest, OutOfBoundsForwardsCopyShouldCopyPartially)
{
    constexpr char buf[] = "abcdef";
    uint64_t n;

    mb::MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_move(file, 2, 0, 5, &n), mb::FileStatus::OK);
    ASSERT_EQ(n, 4u);
    ASSERT_STREQ(buf, "cdefef");
}

TEST(FileMoveTest, OutOfBoundsBackwardsCopyShouldCopyPartially)
{
    constexpr char buf[] = "abcdef";
    uint64_t n;

    mb::MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_move(file, 0, 2, 5, &n), mb::FileStatus::OK);
    ASSERT_EQ(n, 4u);
    ASSERT_STREQ(buf, "ababcd");
}

TEST(FileMoveTest, LargeForwardsCopyShouldSucceed)
{
    char *buf;
    constexpr size_t buf_size = 100000;
    uint64_t n;

    buf = static_cast<char *>(malloc(buf_size));
    ASSERT_TRUE(!!buf);

    memset(buf, 'a', buf_size / 2);
    memset(buf + buf_size / 2, 'b', buf_size / 2);

    mb::MemoryFile file(buf, buf_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_move(file, buf_size / 2, 0, buf_size / 2, &n),
              mb::FileStatus::OK);
    ASSERT_EQ(n, buf_size / 2);

    for (size_t i = 0; i < buf_size; ++i) {
        ASSERT_EQ(buf[i], 'b');
    }

    free(buf);
}

TEST(FileMoveTest, LargeBackwardsCopyShouldSucceed)
{
    char *buf;
    constexpr size_t buf_size = 100000;
    uint64_t n;

    buf = static_cast<char *>(malloc(buf_size));
    ASSERT_TRUE(!!buf);

    memset(buf, 'a', buf_size / 2);
    memset(buf + buf_size / 2, 'b', buf_size / 2);

    mb::MemoryFile file(buf, buf_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(mb::file_move(file, 0, buf_size / 2, buf_size / 2, &n),
              mb::FileStatus::OK);
    ASSERT_EQ(n, buf_size / 2);

    for (size_t i = 0; i < buf_size; ++i) {
        ASSERT_EQ(buf[i], 'a');
    }

    free(buf);
}

// TODO: Add more tests after integrating gmock
