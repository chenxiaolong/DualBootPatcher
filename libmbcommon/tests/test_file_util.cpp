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
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "file/mock_test_file.h"

using namespace mb;

struct FileUtilTest : testing::Test
{
    testing::NiceMock<MockTestFile> _file;
};

TEST_F(FileUtilTest, ReadFullyNormal)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_))
            .Times(5)
            .WillRepeatedly(testing::Return(2));

    // Open file
    ASSERT_TRUE(_file.open());

    char buf[10];
    auto n = file_read_fully(_file, buf, sizeof(buf));
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 10u);
}

TEST_F(FileUtilTest, ReadFullyEOF)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(0));

    // Open file
    ASSERT_TRUE(_file.open());

    char buf[10];
    auto n = file_read_fully(_file, buf, sizeof(buf));
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 8u);
}

TEST_F(FileUtilTest, ReadFullyPartialFail)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(std::error_code{}));

    // Open file
    ASSERT_TRUE(_file.open());

    char buf[10];
    ASSERT_FALSE(file_read_fully(_file, buf, sizeof(buf)));
}

TEST_F(FileUtilTest, WriteFullyNormal)
{
    EXPECT_CALL(_file, on_write(testing::_, testing::_))
            .Times(5)
            .WillRepeatedly(testing::Return(2));

    // Open file
    ASSERT_TRUE(_file.open());

    auto n = file_write_fully(_file, "xxxxxxxxxx", 10);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 10u);
}

TEST_F(FileUtilTest, WriteFullyEOF)
{
    EXPECT_CALL(_file, on_write(testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(0));

    // Open file
    ASSERT_TRUE(_file.open());

    auto n = file_write_fully(_file, "xxxxxxxxxx", 10);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 8u);
}

TEST_F(FileUtilTest, WriteFullyPartialFail)
{
    EXPECT_CALL(_file, on_write(testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(std::error_code{}));

    // Open file
    ASSERT_TRUE(_file.open());

    ASSERT_FALSE(file_write_fully(_file, "xxxxxxxxxx", 10));
}

TEST_F(FileUtilTest, ReadDiscardNormal)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_))
            .Times(5)
            .WillRepeatedly(testing::Return(2));

    // Open file
    ASSERT_TRUE(_file.open());

    auto n = file_read_discard(_file, 10);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 10u);
}

TEST_F(FileUtilTest, ReadDiscardEOF)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(0));

    // Open file
    ASSERT_TRUE(_file.open());

    auto n = file_read_discard(_file, 10);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 8u);
}

TEST_F(FileUtilTest, ReadDiscardPartialFail)
{
    EXPECT_CALL(_file, on_read(testing::_, testing::_))
            .Times(5)
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(2))
            .WillOnce(testing::Return(std::error_code{}));

    // Open file
    ASSERT_TRUE(_file.open());

    ASSERT_FALSE(file_read_discard(_file, 10));
}

struct FileSearchTest : testing::Test
{
    // Callback counters
    int _n_result = 0;

    static oc::result<FileSearchAction> _result_cb(File &file, void *userdata,
                                                   uint64_t offset)
    {
        (void) file;
        (void) offset;

        FileSearchTest *test = static_cast<FileSearchTest *>(userdata);
        ++test->_n_result;

        return FileSearchAction::Continue;
    }
};

TEST_F(FileSearchTest, CheckInvalidBoundariesFail)
{
    MemoryFile file("", 0);
    ASSERT_TRUE(file.is_open());

    auto result = file_search(file, 20, 10, 0, "x", 1, -1, &_result_cb, this);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), FileError::ArgumentOutOfRange);
}

TEST_F(FileSearchTest, CheckZeroMaxMatches)
{
    MemoryFile file("", 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file_search(file, -1, -1, 0, "x", 1, 0, &_result_cb, this));
}

TEST_F(FileSearchTest, CheckZeroPatternSize)
{
    MemoryFile file("", 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file_search(file, -1, -1, 0, nullptr, 0, -1, &_result_cb,
                            this));
}

TEST_F(FileSearchTest, CheckBufferSize)
{
    MemoryFile file("", 0);
    ASSERT_TRUE(file.is_open());

    // Auto buffer size
    ASSERT_TRUE(file_search(file, -1, -1, 0, "x", 1, -1, &_result_cb, this));

    // Too small
    auto result = file_search(file, -1, -1, 1, "xxx", 3, -1, &_result_cb, this);
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), FileError::ArgumentOutOfRange);

    // Equal to pattern size
    ASSERT_TRUE(file_search(file, -1, -1, 1, "x", 1, -1, &_result_cb, this));
}

TEST_F(FileSearchTest, FindNormal)
{
    MemoryFile file("abc", 3);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file_search(file, -1, -1, 0, "a", 1, -1, &_result_cb, this));
}

TEST(FileMoveTest, DegenerateCasesShouldSucceed)
{
    constexpr char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    // src == dest
    ASSERT_TRUE(file_move(file, 0, 0, 3));

    // size == 0
    ASSERT_TRUE(file_move(file, 3, 0, 0));
}

TEST(FileMoveTest, NormalForwardsCopyShouldSucceed)
{
    constexpr char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    auto n = file_move(file, 2, 0, 3);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 3u);
    ASSERT_STREQ(buf, "cdedef");
}

TEST(FileMoveTest, NormalBackwardsCopyShouldSucceed)
{
    constexpr char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    auto n = file_move(file, 0, 2, 3);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 3u);
    ASSERT_STREQ(buf, "ababcf");
}

TEST(FileMoveTest, OutOfBoundsForwardsCopyShouldCopyPartially)
{
    constexpr char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    auto n = file_move(file, 2, 0, 5);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 4u);
    ASSERT_STREQ(buf, "cdefef");
}

TEST(FileMoveTest, OutOfBoundsBackwardsCopyShouldCopyPartially)
{
    constexpr char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    auto n = file_move(file, 0, 2, 5);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 4u);
    ASSERT_STREQ(buf, "ababcd");
}

TEST(FileMoveTest, LargeForwardsCopyShouldSucceed)
{
    char *buf;
    constexpr size_t buf_size = 100000;

    buf = static_cast<char *>(malloc(buf_size));
    ASSERT_TRUE(!!buf);

    memset(buf, 'a', buf_size / 2);
    memset(buf + buf_size / 2, 'b', buf_size / 2);

    MemoryFile file(buf, buf_size);
    ASSERT_TRUE(file.is_open());

    auto n = file_move(file, buf_size / 2, 0, buf_size / 2);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), buf_size / 2);

    for (size_t i = 0; i < buf_size; ++i) {
        ASSERT_EQ(buf[i], 'b');
    }

    free(buf);
}

TEST(FileMoveTest, LargeBackwardsCopyShouldSucceed)
{
    char *buf;
    constexpr size_t buf_size = 100000;

    buf = static_cast<char *>(malloc(buf_size));
    ASSERT_TRUE(!!buf);

    memset(buf, 'a', buf_size / 2);
    memset(buf + buf_size / 2, 'b', buf_size / 2);

    MemoryFile file(buf, buf_size);
    ASSERT_TRUE(file.is_open());

    auto n = file_move(file, 0, buf_size / 2, buf_size / 2);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), buf_size / 2);

    for (size_t i = 0; i < buf_size; ++i) {
        ASSERT_EQ(buf[i], 'a');
    }

    free(buf);
}

// TODO: Add more tests after integrating gmock
