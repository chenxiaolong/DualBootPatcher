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
#include "mbcommon/file_p.h"
#include "mbcommon/string.h"

#include "file/mock_test_file.h"


TEST(FileTest, CheckInitialValues)
{
    testing::NiceMock<MockTestFile> file;

    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
    ASSERT_EQ(file._priv_func()->error_code, std::error_code());
    ASSERT_TRUE(file._priv_func()->error_string.empty());
}

TEST(FileTest, CheckStatesNormal)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_open())
            .Times(1);
    EXPECT_CALL(file, on_close())
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::Opened);

    // Close file
    ASSERT_TRUE(file.close());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
}

TEST(FileTest, CheckMoveConstructor)
{
    // Can't use gmock for this test since its objects are not movable
    TestFile file1;

    // Open the file
    ASSERT_TRUE(file1.open());

    // Write some data to the file
    size_t n;
    ASSERT_TRUE(file1.write("foobar", 6, n));

    // Construct another file from file1
    TestFile file2(std::move(file1));

    // file2 should become what file1 was
    ASSERT_EQ(file2._priv_func()->state, mb::FileState::Opened);
    ASSERT_EQ(file2._position, 6u);

    // file1 should be left in an unspecified (but valid) state where the pimpl
    // pointer is NULL
    ASSERT_EQ(file1._priv_func(), nullptr);

    // Everything should fail for file1
    ASSERT_FALSE(file1.open());
    ASSERT_FALSE(file1.read(nullptr, 0, n));
    ASSERT_FALSE(file1.write(nullptr, 0, n));
    ASSERT_FALSE(file1.seek(0, SEEK_SET, nullptr));
    ASSERT_FALSE(file1.truncate(0));
    ASSERT_FALSE(file1.close());
    ASSERT_FALSE(file1.set_error({}, ""));

    // Bring File1 back to life
    file1 = TestFile();
    ASSERT_NE(file1._priv_func(), nullptr);
    ASSERT_EQ(file1._priv_func()->state, mb::FileState::New);
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
    size_t n;
    ASSERT_TRUE(file1.write("foobar", 6, n));
    ASSERT_TRUE(file2.write("hello", 5, n));

    // Move file1 to file2
    file2 = std::move(file1);

    // file2 should have been closed
    ASSERT_EQ(counters2.n_close, 1u);

    // file2 should become what file1 was
    ASSERT_EQ(file2._priv_func()->state, mb::FileState::Opened);
    ASSERT_EQ(file2._position, 6u);

    // file1 should be left in an unspecified (but valid) state where the pimpl
    // pointer is NULL
    ASSERT_EQ(file1._priv_func(), nullptr);

    // Everything should fail for file1
    ASSERT_FALSE(file1.open());
    ASSERT_FALSE(file1.read(nullptr, 0, n));
    ASSERT_FALSE(file1.write(nullptr, 0, n));
    ASSERT_FALSE(file1.seek(0, SEEK_SET, nullptr));
    ASSERT_FALSE(file1.truncate(0));
    ASSERT_FALSE(file1.close());
    ASSERT_FALSE(file1.set_error({}, ""));

    // Bring File1 back to life
    file1 = TestFile();
    ASSERT_NE(file1._priv_func(), nullptr);
    ASSERT_EQ(file1._priv_func()->state, mb::FileState::New);
}

TEST(FileTest, FreeNewFile)
{
    TestFileCounters counters;

    {
        testing::NiceMock<MockTestFile> file(&counters);
    }

    // The close callback should not have been called because nothing was opened
    ASSERT_EQ(counters.n_close, 0u);
}

TEST(FileTest, FreeOpenedFile)
{
    TestFileCounters counters;

    {
        testing::NiceMock<MockTestFile> file(&counters);

        ASSERT_TRUE(file.open());
    }

    // Ensure that the close callback was called
    ASSERT_EQ(counters.n_close, 1u);
}

TEST(FileTest, FreeClosedFile)
{
    TestFileCounters counters;

    {
        testing::NiceMock<MockTestFile> file(&counters);

        EXPECT_CALL(file, on_close())
                .Times(1);

        // Open file
        ASSERT_TRUE(file.open());

        // Close file
        ASSERT_TRUE(file.close());
        ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
    }

    // Ensure that the close callback was not called again
    ASSERT_EQ(counters.n_close, 1u);
}

TEST(FileTest, FreeFatalFile)
{
    TestFileCounters counters;

    {
        testing::NiceMock<MockTestFile> file(&counters);

        // Open file
        ASSERT_TRUE(file.open());

        file._priv_func()->state = mb::FileState::Fatal;
    }

    // Ensure that the close callback was called
    ASSERT_EQ(counters.n_close, 1u);
}

TEST(FileTest, OpenReturnFailure)
{
    testing::NiceMock<MockTestFile> file;

    // Set open callback
    EXPECT_CALL(file, on_open())
            .Times(2)
            .WillOnce(testing::Return(false))
            .WillOnce(Invoke(&file, &MockTestFile::orig_on_open));
    EXPECT_CALL(file, on_close())
            .Times(1);

    // Open file
    ASSERT_FALSE(file.open());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);

    // Reopen file
    ASSERT_TRUE(file.open());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::Opened);

    // The close callback should have been called to clean up resources
}

TEST(FileTest, OpenFileTwice)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_open())
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::Opened);

    // Open again
    ASSERT_FALSE(file.open());
    ASSERT_EQ(file._priv_func()->error_code, mb::FileError::InvalidState);
    ASSERT_NE(file._priv_func()->error_string.find("open"), std::string::npos);
    ASSERT_NE(file._priv_func()->error_string.find("Invalid state"), std::string::npos);
}

TEST(FileTest, CloseNewFile)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_close())
            .Times(0);

    ASSERT_TRUE(file.close());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
}

TEST(FileTest, CloseFileTwice)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_close())
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());

    // Close file
    ASSERT_TRUE(file.close());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);

    // Close file again
    ASSERT_TRUE(file.close());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
}

TEST(FileTest, CloseReturnFailure)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_close())
            .Times(1)
            .WillOnce(testing::Return(false));

    // Open file
    ASSERT_TRUE(file.open());

    // Close file
    ASSERT_FALSE(file.close());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
}

TEST(FileTest, ReadCallbackCalled)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_read(testing::_, testing::_, testing::_))
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());

    // Read from file
    char buf[10];
    size_t n;
    ASSERT_TRUE(file.read(buf, sizeof(buf), n));
    ASSERT_EQ(n, sizeof(buf));
    ASSERT_EQ(memcmp(buf, file._buf.data(), sizeof(buf)), 0);
}

TEST(FileTest, ReadInWrongState)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_read(testing::_, testing::_, testing::_))
            .Times(0);

    // Read from file
    char c;
    size_t n;
    ASSERT_FALSE(file.read(&c, 1, n));
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
    ASSERT_EQ(file._priv_func()->error_code, mb::FileError::InvalidState);
    ASSERT_NE(file._priv_func()->error_string.find("read"), std::string::npos);
    ASSERT_NE(file._priv_func()->error_string.find("Invalid state"), std::string::npos);
}

TEST(FileTest, ReadReturnFailure)
{
    testing::NiceMock<MockTestFile> file;

    // Set read callback
    EXPECT_CALL(file, on_read(testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::Return(false));

    // Open file
    ASSERT_TRUE(file.open());

    // Read from file
    char c;
    size_t n;
    ASSERT_FALSE(file.read(&c, 1, n));
    ASSERT_EQ(file._priv_func()->state, mb::FileState::Opened);
}

TEST(FileTest, WriteCallbackCalled)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_write(testing::_, testing::_, testing::_))
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());

    // Write to file
    char buf[] = "Hello, world!";
    size_t size = strlen(buf);
    size_t n;
    ASSERT_TRUE(file.write(buf, size, n));
    ASSERT_EQ(n, size);
    ASSERT_EQ(memcmp(buf, file._buf.data(), size), 0);
}

TEST(FileTest, WriteInWrongState)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_write(testing::_, testing::_, testing::_))
            .Times(0);

    // Write to file
    char c;
    size_t n;
    ASSERT_FALSE(file.write(&c, 1, n));
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
    ASSERT_EQ(file._priv_func()->error_code, mb::FileError::InvalidState);
    ASSERT_NE(file._priv_func()->error_string.find("write"), std::string::npos);
    ASSERT_NE(file._priv_func()->error_string.find("Invalid state"), std::string::npos);
}

TEST(FileTest, WriteReturnFailure)
{
    testing::NiceMock<MockTestFile> file;

    // Set write callback
    EXPECT_CALL(file, on_write(testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::Return(false));

    // Open file
    ASSERT_TRUE(file.open());

    // Write to file
    size_t n;
    ASSERT_FALSE(file.write("x", 1, n));
    ASSERT_EQ(file._priv_func()->state, mb::FileState::Opened);
}

TEST(FileTest, SeekCallbackCalled)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_seek(testing::_, testing::_, testing::_))
            .Times(2);

    // Open file
    ASSERT_TRUE(file.open());

    // Seek file
    uint64_t pos;
    ASSERT_TRUE(file.seek(0, SEEK_END, &pos));
    ASSERT_EQ(pos, file._buf.size());
    ASSERT_EQ(pos, file._position);

    // Seek again with NULL offset output parameter
    ASSERT_TRUE(file.seek(-10, SEEK_END, nullptr));
    ASSERT_EQ(file._position, file._buf.size() - 10);
}

TEST(FileTest, SeekInWrongState)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_seek(testing::_, testing::_, testing::_))
            .Times(0);

    // Seek file
    ASSERT_FALSE(file.seek(0, SEEK_END, nullptr));
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
    ASSERT_EQ(file._priv_func()->error_code, mb::FileError::InvalidState);
    ASSERT_NE(file._priv_func()->error_string.find("seek"), std::string::npos);
    ASSERT_NE(file._priv_func()->error_string.find("Invalid state"), std::string::npos);
}

TEST(FileTest, SeekReturnFailure)
{
    testing::NiceMock<MockTestFile> file;

    // Set seek callback
    EXPECT_CALL(file, on_seek(testing::_, testing::_, testing::_))
            .Times(1)
            .WillOnce(testing::Return(false));

    // Open file
    ASSERT_TRUE(file.open());

    // Seek file
    ASSERT_FALSE(file.seek(0, SEEK_END, nullptr));
    ASSERT_EQ(file._priv_func()->state, mb::FileState::Opened);
}

TEST(FileTest, TruncateCallbackCalled)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_truncate(testing::_))
            .Times(1);

    // Open file
    ASSERT_TRUE(file.open());

    // Truncate file
    ASSERT_TRUE(file.truncate(INITIAL_BUF_SIZE / 2));
}

TEST(FileTest, TruncateInWrongState)
{
    testing::NiceMock<MockTestFile> file;

    EXPECT_CALL(file, on_truncate(testing::_))
            .Times(0);

    // Truncate file
    ASSERT_FALSE(file.truncate(INITIAL_BUF_SIZE + 1));
    ASSERT_EQ(file._priv_func()->state, mb::FileState::New);
    ASSERT_EQ(file._priv_func()->error_code, mb::FileError::InvalidState);
    ASSERT_NE(file._priv_func()->error_string.find("truncate"), std::string::npos);
    ASSERT_NE(file._priv_func()->error_string.find("Invalid state"), std::string::npos);
}

TEST(FileTest, TruncateReturnFailure)
{
    testing::NiceMock<MockTestFile> file;

    // Set truncate callback
    EXPECT_CALL(file, on_truncate(testing::_))
            .Times(1)
            .WillOnce(testing::Return(false));

    // Open file
    ASSERT_TRUE(file.open());

    // Truncate file
    ASSERT_FALSE(file.truncate(INITIAL_BUF_SIZE + 1));
    ASSERT_FALSE(file.is_fatal());
    ASSERT_EQ(file._priv_func()->state, mb::FileState::Opened);
}

TEST(FileTest, SetError)
{
    testing::NiceMock<MockTestFile> file;

    ASSERT_EQ(file._priv_func()->error_code, std::error_code());
    ASSERT_TRUE(file._priv_func()->error_string.empty());

    ASSERT_TRUE(file.set_error(mb::make_error_code(mb::FileError::ArgumentOutOfRange),
                               "%s, %s!", "Hello", "world"));

    ASSERT_EQ(file._priv_func()->error_code, mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file._priv_func()->error_string.find("Hello, world!"),
              std::string::npos);
    ASSERT_EQ(file.error(), file._priv_func()->error_code);
    ASSERT_EQ(file.error_string(), file._priv_func()->error_string);
}
