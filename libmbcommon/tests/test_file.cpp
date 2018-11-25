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

#include <cinttypes>

#include "mbcommon/file.h"
#include "mbcommon/string.h"

#include "file/mock_test_file.h"

using namespace mb;
using namespace mb::detail;
using namespace testing;

TEST(FileTest, CheckInitialValues)
{
    NiceMock<MockTestFile> file;

    ASSERT_EQ(file.state(), FileState::New);
}

TEST(FileTest, CheckStatesNormal)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_open())
            .Times(1);
    EXPECT_CALL(file, on_close())
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());
    ASSERT_EQ(file.state(), FileState::Opened);

    // Close file
    ASSERT_TRUE(file.close());
    ASSERT_EQ(file.state(), FileState::New);
}

TEST(FileTest, CheckMoveConstructor)
{
    // Can't use gmock for this test since its objects are not movable
    TestFile file1;

    // Open the file
    ASSERT_TRUE(file1.open());

    // Write some data to the file
    ASSERT_TRUE(file1.write("foobar", 6));

    // Construct another file from file1
    TestFile file2(std::move(file1));

    // file2 should become what file1 was
    ASSERT_EQ(file2.state(), FileState::Opened);
    ASSERT_EQ(file2._position, 6u);

    // file1 should be left in an unspecified (but valid) state
    ASSERT_EQ(file1.state(), FileState::Moved);

    // Everything should fail for file1
    ASSERT_FALSE(file1.open());
    ASSERT_FALSE(file1.read(nullptr, 0));
    ASSERT_FALSE(file1.write(nullptr, 0));
    ASSERT_FALSE(file1.seek(0, SEEK_SET));
    ASSERT_FALSE(file1.truncate(0));
    ASSERT_FALSE(file1.close());

    // Bring File1 back to life
    file1 = TestFile();
    ASSERT_EQ(file1.state(), FileState::New);
}

TEST(FileTest, CheckMoveAssignment)
{
    // Can't use gmock for this test since its objects are not movable
    TestFileCounters counters1;
    TestFileCounters counters2;

    TestFile file1(&counters1);
    TestFile file2(&counters2);

    // Open both files
    ASSERT_TRUE(file1.open());
    ASSERT_TRUE(file2.open());

    // Differentiate the two files
    ASSERT_TRUE(file1.write("foobar", 6));
    ASSERT_TRUE(file2.write("hello", 5));

    // Move file1 to file2
    file2 = std::move(file1);

    // file2 should have been closed
    ASSERT_EQ(counters2.n_close, 1u);

    // file2 should become what file1 was
    ASSERT_EQ(file2.state(), FileState::Opened);
    ASSERT_EQ(file2._position, 6u);

    // file1 should be left in an unspecified (but valid) state
    ASSERT_EQ(file1.state(), FileState::Moved);

    // Everything should fail for file1
    ASSERT_FALSE(file1.open());
    ASSERT_FALSE(file1.read(nullptr, 0));
    ASSERT_FALSE(file1.write(nullptr, 0));
    ASSERT_FALSE(file1.seek(0, SEEK_SET));
    ASSERT_FALSE(file1.truncate(0));
    ASSERT_FALSE(file1.close());

    // Bring File1 back to life
    file1 = TestFile();
    ASSERT_EQ(file1.state(), FileState::New);
}

TEST(FileTest, FreeNewFile)
{
    TestFileCounters counters;

    {
        NiceMock<MockTestFile> file(&counters);
    }

    // The close callback should not have been called because nothing was opened
    ASSERT_EQ(counters.n_close, 0u);
}

TEST(FileTest, FreeOpenedFile)
{
    TestFileCounters counters;

    {
        NiceMock<MockTestFile> file(&counters);

        ASSERT_TRUE(file.open());
    }

    // Ensure that the close callback was called
    ASSERT_EQ(counters.n_close, 1u);
}

TEST(FileTest, FreeClosedFile)
{
    TestFileCounters counters;

    {
        NiceMock<MockTestFile> file(&counters);

        EXPECT_CALL(file, on_close())
                .Times(1);

        // Open file
        ASSERT_TRUE(file.open());

        // Close file
        ASSERT_TRUE(file.close());
        ASSERT_EQ(file.state(), FileState::New);
    }

    // Ensure that the close callback was not called again
    ASSERT_EQ(counters.n_close, 1u);
}

TEST(FileTest, OpenReturnFailure)
{
    NiceMock<MockTestFile> file;

    // Set open callback
    EXPECT_CALL(file, on_open())
            .Times(2)
            .WillOnce(Return(std::error_code{}))
            .WillOnce(Invoke(&file, &MockTestFile::orig_on_open));
    EXPECT_CALL(file, on_close())
            .Times(1);

    // Open file
    ASSERT_FALSE(file.open());
    ASSERT_EQ(file.state(), FileState::New);

    // Reopen file
    ASSERT_TRUE(file.open());
    ASSERT_EQ(file.state(), FileState::Opened);

    // The close callback should have been called to clean up resources
}

TEST(FileTest, OpenFileTwice)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_open())
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());
    ASSERT_EQ(file.state(), FileState::Opened);

    // Open again
    auto result = file.open();
    ASSERT_FALSE(result);
    ASSERT_EQ(result.error(), FileError::InvalidState);
}

TEST(FileTest, CloseNewFile)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_close())
            .Times(0);

    ASSERT_TRUE(file.close());
    ASSERT_EQ(file.state(), FileState::New);
}

TEST(FileTest, CloseFileTwice)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_close())
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());

    // Close file
    ASSERT_TRUE(file.close());
    ASSERT_EQ(file.state(), FileState::New);

    // Close file again
    ASSERT_TRUE(file.close());
    ASSERT_EQ(file.state(), FileState::New);
}

TEST(FileTest, CloseReturnFailure)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_close())
            .Times(1)
            .WillOnce(Return(std::error_code{}));

    // Open file
    ASSERT_TRUE(file.open());

    // Close file
    ASSERT_FALSE(file.close());
    ASSERT_EQ(file.state(), FileState::New);
}

TEST(FileTest, ReadCallbackCalled)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_read(_, _))
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());

    // Read from file
    char buf[10];
    auto n = file.read(buf, sizeof(buf));
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), sizeof(buf));
    ASSERT_EQ(memcmp(buf, file._buf.data(), sizeof(buf)), 0);
}

TEST(FileTest, ReadInWrongState)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_read(_, _))
            .Times(0);

    // Read from file
    char c;
    auto n = file.read(&c, 1);
    ASSERT_FALSE(n);
    ASSERT_EQ(n.error(), FileError::InvalidState);
    ASSERT_EQ(file.state(), FileState::New);
}

TEST(FileTest, ReadReturnFailure)
{
    NiceMock<MockTestFile> file;

    // Set read callback
    EXPECT_CALL(file, on_read(_, _))
            .Times(1)
            .WillOnce(Return(std::error_code{}));

    // Open file
    ASSERT_TRUE(file.open());

    // Read from file
    char c;
    ASSERT_FALSE(file.read(&c, 1));
    ASSERT_EQ(file.state(), FileState::Opened);
}

TEST(FileTest, WriteCallbackCalled)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_write(_, _))
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());

    // Write to file
    char buf[] = "Hello, world!";
    size_t size = strlen(buf);
    auto n = file.write(buf, size);
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), size);
    ASSERT_EQ(memcmp(buf, file._buf.data(), size), 0);
}

TEST(FileTest, WriteInWrongState)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_write(_, _))
            .Times(0);

    // Write to file
    char c;
    auto n = file.write(&c, 1);
    ASSERT_FALSE(n);
    ASSERT_EQ(n.error(), FileError::InvalidState);
    ASSERT_EQ(file.state(), FileState::New);
}

TEST(FileTest, WriteReturnFailure)
{
    NiceMock<MockTestFile> file;

    // Set write callback
    EXPECT_CALL(file, on_write(_, _))
            .Times(1)
            .WillOnce(Return(std::error_code{}));

    // Open file
    ASSERT_TRUE(file.open());

    // Write to file
    ASSERT_FALSE(file.write("x", 1));
    ASSERT_EQ(file.state(), FileState::Opened);
}

TEST(FileTest, SeekCallbackCalled)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_seek(_, _))
            .Times(2);

    // Open file
    ASSERT_TRUE(file.open());

    // Seek file
    auto pos = file.seek(0, SEEK_END);
    ASSERT_TRUE(pos);
    ASSERT_EQ(pos.value(), file._buf.size());
    ASSERT_EQ(pos.value(), file._position);

    ASSERT_TRUE(file.seek(-10, SEEK_END));
    ASSERT_EQ(file._position, file._buf.size() - 10);
}

TEST(FileTest, SeekInWrongState)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_seek(_, _))
            .Times(0);

    // Seek file
    auto offset = file.seek(0, SEEK_END);
    ASSERT_FALSE(offset);
    ASSERT_EQ(offset.error(), FileError::InvalidState);
    ASSERT_EQ(file.state(), FileState::New);
}

TEST(FileTest, SeekReturnFailure)
{
    NiceMock<MockTestFile> file;

    // Set seek callback
    EXPECT_CALL(file, on_seek(_, _))
            .Times(1)
            .WillOnce(Return(std::error_code{}));

    // Open file
    ASSERT_TRUE(file.open());

    // Seek file
    ASSERT_FALSE(file.seek(0, SEEK_END));
    ASSERT_EQ(file.state(), FileState::Opened);
}

TEST(FileTest, TruncateCallbackCalled)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_truncate(_))
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());

    // Truncate file
    ASSERT_TRUE(file.truncate(INITIAL_BUF_SIZE / 2));
}

TEST(FileTest, TruncateInWrongState)
{
    NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_truncate(_))
            .Times(0);

    // Truncate file
    auto result = file.truncate(INITIAL_BUF_SIZE + 1);
    ASSERT_FALSE(result);
    ASSERT_EQ(file.state(), FileState::New);
    ASSERT_EQ(result.error(), FileError::InvalidState);
}

TEST(FileTest, TruncateReturnFailure)
{
    NiceMock<MockTestFile> file;

    // Set truncate callback
    EXPECT_CALL(file, on_truncate(_))
            .Times(1)
            .WillOnce(Return(std::error_code{}));

    // Open file
    ASSERT_TRUE(file.open());

    // Truncate file
    ASSERT_FALSE(file.truncate(INITIAL_BUF_SIZE + 1));
    ASSERT_EQ(file.state(), FileState::Opened);
}
