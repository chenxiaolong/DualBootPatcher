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

#include <algorithm>
#include <memory>
#include <vector>

#include <cinttypes>

#include "mbcommon/file/memory.h"
#include "mbcommon/file_error.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "file/mock_test_file.h"

using namespace mb;
using namespace mb::detail;
using namespace testing;

struct FileUtilTest : Test
{
    NiceMock<MockFile> _file;

    void SetUp() override
    {
        EXPECT_CALL(_file, is_open())
                .WillRepeatedly(Return(true));
    }
};

TEST_F(FileUtilTest, ReadRetryNormal)
{
    EXPECT_CALL(_file, read(_, _))
            .Times(5)
            .WillRepeatedly(Return(2u));

    char buf[10];
    ASSERT_EQ(file_read_retry(_file, buf, sizeof(buf)), oc::success(10u));
}

TEST_F(FileUtilTest, ReadRetryEOF)
{
    EXPECT_CALL(_file, read(_, _))
            .Times(5)
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(0u));

    char buf[10];
    ASSERT_EQ(file_read_retry(_file, buf, sizeof(buf)), oc::success(8u));
}

TEST_F(FileUtilTest, ReadRetryInterrupted)
{
    auto eintr = std::make_error_code(std::errc::interrupted);

    EXPECT_CALL(_file, read(_, _))
            .Times(5)
            .WillOnce(Return(eintr))
            .WillOnce(Return(eintr))
            .WillOnce(Return(eintr))
            .WillOnce(Return(eintr))
            .WillOnce(Return(10u));

    char buf[10];
    ASSERT_EQ(file_read_retry(_file, buf, sizeof(buf)), oc::success(10u));
}

TEST_F(FileUtilTest, ReadRetryFailure)
{
    EXPECT_CALL(_file, read(_, _))
            .Times(1)
            .WillOnce(Return(std::error_code()));

    char buf[10];
    ASSERT_EQ(file_read_retry(_file, buf, sizeof(buf)),
              oc::failure(std::error_code()));
}

TEST_F(FileUtilTest, WriteRetryNormal)
{
    EXPECT_CALL(_file, write(_, _))
            .Times(5)
            .WillRepeatedly(Return(2u));

    ASSERT_EQ(file_write_retry(_file, "xxxxxxxxxx", 10u), oc::success(10u));
}

TEST_F(FileUtilTest, WriteRetryEOF)
{
    EXPECT_CALL(_file, write(_, _))
            .Times(5)
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(0u));

    ASSERT_EQ(file_write_retry(_file, "xxxxxxxxxx", 10u), oc::success(8u));
}

TEST_F(FileUtilTest, WriteRetryInterrupted)
{
    auto eintr = std::make_error_code(std::errc::interrupted);

    EXPECT_CALL(_file, write(_, _))
            .Times(5)
            .WillOnce(Return(eintr))
            .WillOnce(Return(eintr))
            .WillOnce(Return(eintr))
            .WillOnce(Return(eintr))
            .WillOnce(Return(10u));

    ASSERT_EQ(file_write_retry(_file, "xxxxxxxxxx", 10u), oc::success(10u));
}

TEST_F(FileUtilTest, WriteRetryFailure)
{
    EXPECT_CALL(_file, write(_, _))
            .Times(1)
            .WillOnce(Return(std::error_code()));

    ASSERT_EQ(file_write_retry(_file, "xxxxxxxxxx", 10u),
              oc::failure(std::error_code()));
}

TEST_F(FileUtilTest, ReadExactNormal)
{
    EXPECT_CALL(_file, read(_, _))
            .Times(5)
            .WillRepeatedly(Return(2u));

    char buf[10];
    ASSERT_TRUE(file_read_exact(_file, buf, sizeof(buf)));
}

TEST_F(FileUtilTest, ReadExactEOF)
{
    EXPECT_CALL(_file, read(_, _))
            .Times(5)
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(0u));

    char buf[10];
    ASSERT_EQ(file_read_exact(_file, buf, sizeof(buf)),
              oc::failure(FileError::UnexpectedEof));
}

TEST_F(FileUtilTest, ReadExactPartialFail)
{
    EXPECT_CALL(_file, read(_, _))
            .Times(5)
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(std::error_code()));

    char buf[10];
    ASSERT_EQ(file_read_exact(_file, buf, sizeof(buf)),
              oc::failure(std::error_code()));
}

TEST_F(FileUtilTest, WriteExactNormal)
{
    EXPECT_CALL(_file, write(_, _))
            .Times(5)
            .WillRepeatedly(Return(2u));

    ASSERT_TRUE(file_write_exact(_file, "xxxxxxxxxx", 10));
}

TEST_F(FileUtilTest, WriteExactEOF)
{
    EXPECT_CALL(_file, write(_, _))
            .Times(5)
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(0u));

    ASSERT_EQ(file_write_exact(_file, "xxxxxxxxxx", 10),
              oc::failure(FileError::UnexpectedEof));
}

TEST_F(FileUtilTest, WriteExactPartialFail)
{
    EXPECT_CALL(_file, write(_, _))
            .Times(5)
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(std::error_code()));

    ASSERT_EQ(file_write_exact(_file, "xxxxxxxxxx", 10),
              oc::failure(std::error_code()));
}

TEST_F(FileUtilTest, ReadDiscardNormal)
{
    EXPECT_CALL(_file, read(_, _))
            .Times(5)
            .WillRepeatedly(Return(2u));

    ASSERT_EQ(file_read_discard(_file, 10), oc::success(10u));
}

TEST_F(FileUtilTest, ReadDiscardEOF)
{
    EXPECT_CALL(_file, read(_, _))
            .Times(5)
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(0u));

    ASSERT_EQ(file_read_discard(_file, 10), oc::success(8u));
}

TEST_F(FileUtilTest, ReadDiscardPartialFail)
{
    EXPECT_CALL(_file, read(_, _))
            .Times(5)
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(2u))
            .WillOnce(Return(std::error_code()));

    ASSERT_EQ(file_read_discard(_file, 10), oc::failure(std::error_code()));
}

TEST(FileSearchTest, CheckZeroPatternSize)
{
    MemoryFile file(const_cast<char *>(""), 0);
    ASSERT_TRUE(file.is_open());

    FileSearcher searcher(&file, nullptr, 0);
    // gtest fails to compile with ASSERT_EQ due to operator<<() shenanigans
    ASSERT_TRUE(searcher.next() == oc::success(std::nullopt));
}

TEST(FileSearchTest, FindAtBeginningOfBuffer)
{
    std::string buf = "abcdxxxx";

    MemoryFile file(buf.data(), buf.size());
    ASSERT_TRUE(file.is_open());

    FileSearcher searcher(&file, "abcd", 4);
    // gtest fails to compile with ASSERT_EQ due to operator<<() shenanigans
    ASSERT_TRUE(searcher.next() == oc::success(0));
    ASSERT_TRUE(searcher.next() == oc::success(std::nullopt));
}

TEST(FileSearchTest, FindAtEndOfBuffer)
{
    std::string buf = "xxxxabcd";

    MemoryFile file(buf.data(), buf.size());
    ASSERT_TRUE(file.is_open());

    FileSearcher searcher(&file, "abcd", 4);
    // gtest fails to compile with ASSERT_EQ due to operator<<() shenanigans
    ASSERT_TRUE(searcher.next() == oc::success(4));
    ASSERT_TRUE(searcher.next() == oc::success(std::nullopt));
}

TEST(FileSearchTest, FindOnBoundaryOfBuffer)
{
    std::string buf;
    buf.resize(DEFAULT_BUFFER_SIZE - 1);
    buf += "abcd";

    MemoryFile file(buf.data(), buf.size());
    ASSERT_TRUE(file.is_open());

    FileSearcher searcher(&file, "abcd", 4);
    // gtest fails to compile with ASSERT_EQ due to operator<<() shenanigans
    ASSERT_TRUE(searcher.next() == oc::success(DEFAULT_BUFFER_SIZE - 1));
    ASSERT_TRUE(searcher.next() == oc::success(std::nullopt));
}

TEST(FileSearchTest, FindAtBeginningOfNextBuffer)
{
    // Next buffer contains pattern size - 1 bytes of data
    std::string buf;
    buf.resize(DEFAULT_BUFFER_SIZE - 3);
    buf += "abcd";

    MemoryFile file(buf.data(), buf.size());
    ASSERT_TRUE(file.is_open());

    FileSearcher searcher(&file, "abcd", 4);
    // gtest fails to compile with ASSERT_EQ due to operator<<() shenanigans
    ASSERT_TRUE(searcher.next() == oc::success(DEFAULT_BUFFER_SIZE - 3));
    ASSERT_TRUE(searcher.next() == oc::success(std::nullopt));
}

TEST(FileSearchTest, FindNonMatching)
{
    std::string buf = "xxxxabcdxxxx";

    MemoryFile file(buf.data(), buf.size());
    ASSERT_TRUE(file.is_open());

    FileSearcher searcher(&file, "abcde", 5);
    // gtest fails to compile with ASSERT_EQ due to operator<<() shenanigans
    ASSERT_TRUE(searcher.next() == oc::success(std::nullopt));
}

TEST(FileSearchTest, FindNonMatchingAtBoundary)
{
    // Next buffer contains pattern size - 1 bytes of data
    std::string buf;
    buf.resize(DEFAULT_BUFFER_SIZE - 4);
    buf += "abcdxxxx";

    MemoryFile file(buf.data(), buf.size());
    ASSERT_TRUE(file.is_open());

    FileSearcher searcher(&file, "abcde", 5);
    // gtest fails to compile with ASSERT_EQ due to operator<<() shenanigans
    ASSERT_TRUE(searcher.next() == oc::success(std::nullopt));
}

TEST(FileMoveTest, DegenerateCasesShouldSucceed)
{
    char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    // src == dest
    ASSERT_TRUE(file_move(file, 0, 0, 3));

    // size == 0
    ASSERT_TRUE(file_move(file, 3, 0, 0));
}

TEST(FileMoveTest, NormalForwardsCopyShouldSucceed)
{
    char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file_move(file, 2, 0, 3), oc::success(3u));
    ASSERT_STREQ(buf, "cdedef");
}

TEST(FileMoveTest, NormalBackwardsCopyShouldSucceed)
{
    char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file_move(file, 0, 2, 3), oc::success(3u));
    ASSERT_STREQ(buf, "ababcf");
}

TEST(FileMoveTest, OutOfBoundsForwardsCopyShouldCopyPartially)
{
    char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file_move(file, 2, 0, 5), oc::success(4u));
    ASSERT_STREQ(buf, "cdefef");
}

TEST(FileMoveTest, OutOfBoundsBackwardsCopyShouldCopyPartially)
{
    char buf[] = "abcdef";

    MemoryFile file(buf, sizeof(buf) - 1);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file_move(file, 0, 2, 5), oc::success(4u));
    ASSERT_STREQ(buf, "ababcd");
}

TEST(FileMoveTest, LargeForwardsCopyShouldSucceed)
{
    std::vector<unsigned char> buf(100000);

    auto middle = buf.begin() + (buf.end() - buf.begin()) / 2;
    std::fill(buf.begin(), middle, 'a');
    std::fill(middle, buf.end(), 'b');

    MemoryFile file(buf.data(), buf.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file_move(file, buf.size() / 2, 0, buf.size() / 2),
              oc::success(buf.size() / 2));

    for (size_t i = 0; i < buf.size(); ++i) {
        ASSERT_EQ(buf[i], 'b');
    }
}

TEST(FileMoveTest, LargeBackwardsCopyShouldSucceed)
{
    std::vector<unsigned char> buf(100000);

    auto middle = buf.begin() + (buf.end() - buf.begin()) / 2;
    std::fill(buf.begin(), middle, 'a');
    std::fill(middle, buf.end(), 'b');

    MemoryFile file(buf.data(), buf.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file_move(file, 0, buf.size() / 2, buf.size() / 2),
              oc::success(buf.size() / 2));

    for (size_t i = 0; i < buf.size(); ++i) {
        ASSERT_EQ(buf[i], 'a');
    }
}

// TODO: Add more tests after integrating gmock
