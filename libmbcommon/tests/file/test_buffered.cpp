/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <fcntl.h>

#include "mbcommon/file.h"
#include "mbcommon/file/buffered.h"
#include "mbcommon/file/memory.h"
#include "mbcommon/file_error.h"
#include "mbcommon/file_util.h"

#include "mock_test_file.h"

using namespace mb;
using namespace testing;

struct FileBufferedTest : Test
{
    NiceMock<MockFile> _mock_file;
    MemoryFile _mem_file;
    BufferedFile _buf_file;
    void *_dyn_data = nullptr;
    size_t _dyn_size = 0;

    ~FileBufferedTest()
    {
        free(_dyn_data);
    }

    void open_mock()
    {
        EXPECT_CALL(_mock_file, is_open())
                .WillRepeatedly(Return(true));
        EXPECT_CALL(_mock_file, seek(_, _))
                .Times(1)
                .WillOnce(Return(0u));

        ASSERT_TRUE(_buf_file.open(_mock_file));
    }

    void open_mem(void *buf, size_t size)
    {
        ASSERT_TRUE(_mem_file.open(buf, size));
        ASSERT_TRUE(_buf_file.open(_mem_file));
    }

    void open_dynamic_mem()
    {
        ASSERT_TRUE(_mem_file.open(&_dyn_data, &_dyn_size));
        ASSERT_TRUE(_buf_file.open(_mem_file));
    }
};

TEST_F(FileBufferedTest, CheckInvalidStates)
{
    auto error = oc::failure(FileError::InvalidState);

    ASSERT_EQ(_buf_file.close(), error);
    ASSERT_EQ(_buf_file.read(nullptr, 0), error);
    ASSERT_EQ(_buf_file.write(nullptr, 0), error);
    ASSERT_EQ(_buf_file.seek(0, SEEK_SET), error);
    ASSERT_EQ(_buf_file.truncate(1024), error);

    ASSERT_EQ(_buf_file.sync_pos(), error);
    ASSERT_EQ(_buf_file.flush(), error);
    ASSERT_EQ(_buf_file.fill_rbuf(), error);
    ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    ASSERT_THAT(_buf_file.wbuf(), IsEmpty());

    ASSERT_FALSE(_buf_file.set_buf_size(0));
    ASSERT_TRUE(_buf_file.set_buf_size(1));
    open_mock();
    ASSERT_FALSE(_buf_file.set_buf_size(1));
}

TEST_F(FileBufferedTest, OpenSuccess)
{
    open_mock();
}

TEST_F(FileBufferedTest, OpenSeekFailure)
{
    EXPECT_CALL(_mock_file, seek(_, _))
            .Times(1)
            .WillOnce(Return(std::make_error_code(std::errc::io_error)));

    ASSERT_EQ(_buf_file.open(_mock_file), oc::failure(std::errc::io_error));
}

TEST_F(FileBufferedTest, CloseSuccess)
{
    open_mock();

    ASSERT_TRUE(_buf_file.close());
    ASSERT_FALSE(_buf_file.is_open());
}

TEST_F(FileBufferedTest, CloseWithFlushSuccess)
{
    open_mock();

    ASSERT_TRUE(_buf_file.write("x", 1));

    EXPECT_CALL(_mock_file, write(_, _))
            .Times(1)
            .WillOnce(Return(1u));

    ASSERT_TRUE(_buf_file.close());
    ASSERT_FALSE(_buf_file.is_open());
}

TEST_F(FileBufferedTest, CloseWithFlushFailure)
{
    open_mock();

    ASSERT_TRUE(_buf_file.write("x", 1));

    EXPECT_CALL(_mock_file, write(_, _))
            .Times(1)
            .WillOnce(Return(std::make_error_code(std::errc::io_error)));

    ASSERT_EQ(_buf_file.close(), oc::failure(std::errc::io_error));
    ASSERT_FALSE(_buf_file.is_open());
}

TEST_F(FileBufferedTest, ReadSuccess)
{
    char data[] = {0, 1, 2, 3, 4, 5, 6, 7};
    _buf_file.set_buf_size(2);
    open_mem(data, sizeof(data));

    // Read larger than buffer
    {
        char buf[3] = {};
        ASSERT_EQ(_buf_file.read(buf, sizeof(buf)), oc::success(3u));
        ASSERT_THAT(buf, ElementsAre(0, 1, 2));
        ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    }

    // Read equal to buffer
    {
        char buf[2] = {};
        ASSERT_EQ(_buf_file.read(buf, sizeof(buf)), oc::success(2u));
        ASSERT_THAT(buf, ElementsAre(3, 4));
        ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    }

    // Read smaller than buffer
    {
        char buf[1] = {};
        ASSERT_EQ(_buf_file.read(buf, sizeof(buf)), oc::success(1u));
        ASSERT_THAT(buf, ElementsAre(5));
        ASSERT_THAT(_buf_file.rbuf(), ElementsAre(6));
    }

    // Read larger than buffer should now return remaining portion of buffer
    {
        char buf[3] = {};
        ASSERT_EQ(_buf_file.read(buf, sizeof(buf)), oc::success(1u));
        ASSERT_THAT(buf, ElementsAre(6, 0, 0));
        ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    }

    // EOF
    {
        char buf[3] = {};
        ASSERT_EQ(_buf_file.read(buf, sizeof(buf)), oc::success(1u));
        ASSERT_THAT(buf, ElementsAre(7, 0, 0));
        ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    }
}

TEST_F(FileBufferedTest, ReadFailure)
{
    open_mock();

    EXPECT_CALL(_mock_file, read(_, _))
            .Times(2)
            .WillOnce(Return(std::make_error_code(std::errc::io_error)))
            .WillOnce(ReturnArg<1>());

    char buf[2];

    // Fail when filling buffer
    ASSERT_EQ(_buf_file.read(buf, sizeof(buf)),
              oc::failure(std::errc::io_error));
    ASSERT_THAT(_buf_file.rbuf(), IsEmpty());

    // Retry after failure
    ASSERT_EQ(_buf_file.read(buf, sizeof(buf)), oc::success(2u));
}

TEST_F(FileBufferedTest, ReadEINTR)
{
    open_mock();

    EXPECT_CALL(_mock_file, read(_, _))
            .Times(3)
            .WillOnce(Return(std::make_error_code(std::errc::interrupted)))
            .WillOnce(Return(std::make_error_code(std::errc::interrupted)))
            .WillOnce(ReturnArg<1>());

    char buf[2];

    ASSERT_EQ(_buf_file.read(buf, sizeof(buf)), oc::success(2u));
}

TEST_F(FileBufferedTest, ReadAfterWriteAndNoFlush)
{
    open_mock();

    char buf[1];

    ASSERT_EQ(_buf_file.write("x", 1), oc::success(1u));
    ASSERT_EQ(_buf_file.read(buf, sizeof(buf)),
              oc::failure(FileError::InvalidState));

    // Flush will occur in destructor
    EXPECT_CALL(_mock_file, write(_, _))
            .Times(1)
            .WillOnce(ReturnArg<1>());
}

TEST_F(FileBufferedTest, WriteSuccess)
{
    char buf[12] = {};
    _buf_file.set_buf_size(2);
    open_mem(buf, sizeof(buf));

    // Write equal to buffer size
    ASSERT_EQ(_buf_file.write("\x00\x01", 2), oc::success(2u));
    ASSERT_THAT(_buf_file.wbuf(), IsEmpty());
    ASSERT_THAT(span(buf, 2), ElementsAre(0, 1));

    // Write less than buffer size
    ASSERT_EQ(_buf_file.write("\x02", 1), oc::success(1u));
    ASSERT_THAT(_buf_file.wbuf(), ElementsAre(2));
    ASSERT_THAT(span(buf, 3), ElementsAre(0, 1, 0));

    // Write less than buffer size to completely fill up buffer
    ASSERT_EQ(_buf_file.write("\x03", 1), oc::success(1u));
    ASSERT_THAT(_buf_file.wbuf(), ElementsAre(2, 3));
    ASSERT_THAT(span(buf, 3), ElementsAre(0, 1, 0));

    // Writes that force a flush
    ASSERT_EQ(_buf_file.write("\x04", 1), oc::success(1u));
    ASSERT_EQ(_buf_file.write("\x05", 1), oc::success(1u));
    ASSERT_THAT(_buf_file.wbuf(), ElementsAre(4, 5));
    ASSERT_THAT(span(buf, 4), ElementsAre(0, 1, 2, 3));

    // Partially fill up buffer for next test
    ASSERT_EQ(_buf_file.write("\x06", 1), oc::success(1u));
    ASSERT_THAT(_buf_file.wbuf(), ElementsAre(6));
    ASSERT_THAT(span(buf, 6), ElementsAre(0, 1, 2, 3, 4, 5));

    // Write data that crosses the buffer boundary
    ASSERT_EQ(_buf_file.write("\x07\x08", 2), oc::success(2u));
    ASSERT_THAT(_buf_file.wbuf(), IsEmpty());
    ASSERT_THAT(span(buf, 8), ElementsAre(0, 1, 2, 3, 4, 5, 6, 7));

    // Write larger than buffer size
    ASSERT_EQ(_buf_file.write("\x09\x0a\x0b", 3), oc::success(3u));
    ASSERT_THAT(_buf_file.wbuf(), IsEmpty());
    ASSERT_THAT(buf, ElementsAre(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11));
}

TEST_F(FileBufferedTest, WriteEofNoFlush)
{
    _buf_file.set_buf_size(2);
    open_mock();

    EXPECT_CALL(_mock_file, write(_, _))
            .Times(1)
            .WillOnce(Return(0u));

    // Write larger than the buffer size, so write() on the underlying file is
    // called directly
    ASSERT_EQ(_buf_file.write("xyz", 3), oc::success(0u));
}

TEST_F(FileBufferedTest, WriteFailure)
{
    _buf_file.set_buf_size(2);
    open_mock();

    EXPECT_CALL(_mock_file, write(_, _))
            .Times(1)
            .WillOnce(Return(std::make_error_code(std::errc::io_error)));

    // Write larger than the buffer size, so write() on the underlying file is
    // called directly
    ASSERT_EQ(_buf_file.write("xyz", 3), oc::failure(std::errc::io_error));
}

TEST_F(FileBufferedTest, WriteEINTR)
{
    open_mock();

    EXPECT_CALL(_mock_file, write(_, _))
            .Times(3)
            .WillOnce(Return(std::make_error_code(std::errc::interrupted)))
            .WillOnce(Return(std::make_error_code(std::errc::interrupted)))
            .WillOnce(ReturnArg<1>());

    // Write larger than the buffer size, so write() on the underlying file is
    // called directly
    ASSERT_EQ(_buf_file.write("xyz", 3), oc::success(3u));
}

TEST_F(FileBufferedTest, WriteAfterRead)
{
    char buf[] = {1, 2, 3};
    _buf_file.set_buf_size(2);
    open_mem(buf, sizeof(buf));

    char temp[1];
    ASSERT_EQ(_buf_file.read(temp, sizeof(temp)), oc::success(1u));
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(2));

    ASSERT_EQ(_buf_file.write("\x04\x05", 2), oc::success(2u));
    ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    ASSERT_THAT(_buf_file.wbuf(), IsEmpty());
    ASSERT_THAT(buf, ElementsAre(1, 4, 5));
}

TEST_F(FileBufferedTest, SeekSuccess)
{
    char buf[] = {0, 1, 2, 3, 4, 5, 6, 7};
    _buf_file.set_buf_size(2);
    open_mem(buf, sizeof(buf));

    // Absolute seek from the end
    ASSERT_EQ(_buf_file.seek(-1, SEEK_END), oc::success(7u));
    ASSERT_TRUE(_buf_file.fill_rbuf());
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(7));

    // Absolute seek from the beginning
    ASSERT_EQ(_buf_file.seek(3, SEEK_SET), oc::success(3u));
    ASSERT_TRUE(_buf_file.fill_rbuf());
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(3, 4));

    // Relative seek to get current position
    ASSERT_EQ(_buf_file.seek(0, SEEK_CUR), oc::success(3u));
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(3, 4));

    // Relative forwards seek that does not clear the buffer
    ASSERT_EQ(_buf_file.seek(1, SEEK_CUR), oc::success(4u));
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(4));

    // Relative backwards seek that does not clear the buffer
    ASSERT_EQ(_buf_file.seek(-1, SEEK_CUR), oc::success(3u));
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(3, 4));

    // Relative forwards seek that does clear the buffer
    ASSERT_EQ(_buf_file.seek(2, SEEK_CUR), oc::success(5u));
    ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    ASSERT_TRUE(_buf_file.fill_rbuf());
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(5, 6));

    // Relative backwards seek that does clear the buffer
    ASSERT_EQ(_buf_file.seek(-1, SEEK_CUR), oc::success(4u));
    ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    ASSERT_TRUE(_buf_file.fill_rbuf());
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(4, 5));

    // Seek after write should flush
    ASSERT_EQ(_buf_file.write("\x08", 1), oc::success(1u));
    ASSERT_THAT(_buf_file.wbuf(), ElementsAre(8));
    ASSERT_EQ(_buf_file.seek(0, SEEK_CUR), oc::success(5u));
    ASSERT_THAT(_buf_file.wbuf(), IsEmpty());
    ASSERT_THAT(buf, ElementsAre(0, 1, 2, 3, 8, 5, 6, 7));
}

TEST_F(FileBufferedTest, SeekSuccessUnderflow)
{
    _buf_file.set_buf_size(5);
    open_mock();

    EXPECT_CALL(_mock_file, read(_, _))
            .Times(1)
            .WillOnce(Return(5u));

    ASSERT_TRUE(_buf_file.fill_rbuf());

    int64_t seek_args[3];

    EXPECT_CALL(_mock_file, seek(_, _))
            .Times(2)
            .WillOnce(DoAll(SaveArg<0>(seek_args), Return(0u)))
            .WillOnce(DoAll(SaveArg<0>(seek_args + 1), Return(0u)));

    // Requires 2 seeks due to underflow
    ASSERT_TRUE(_buf_file.seek(std::numeric_limits<int64_t>::min(), SEEK_CUR));
    ASSERT_EQ(seek_args[0], -5);
    ASSERT_EQ(seek_args[1], std::numeric_limits<int64_t>::min());

    EXPECT_CALL(_mock_file, seek(_, _))
            .Times(1)
            .WillOnce(DoAll(SaveArg<0>(seek_args + 2), Return(0u)));

    // Requires only 1 seek
    ASSERT_TRUE(_buf_file.seek(std::numeric_limits<int64_t>::min(), SEEK_CUR));
    ASSERT_EQ(seek_args[2], std::numeric_limits<int64_t>::min());
}

TEST_F(FileBufferedTest, SeekSuccessWithinBuffer)
{
    _buf_file.set_buf_size(5);
    open_mock();

    EXPECT_CALL(_mock_file, read(_, _))
            .Times(1)
            .WillOnce(Return(5u));

    ASSERT_TRUE(_buf_file.fill_rbuf());

    // seek() should not be called on the underlying file for this
    ASSERT_EQ(_buf_file.seek(3, SEEK_CUR), oc::success(3u));
}

TEST_F(FileBufferedTest, SeekFailed)
{
    open_mock();

    EXPECT_CALL(_mock_file, seek(_, _))
            .Times(1)
            .WillOnce(Return(std::make_error_code(std::errc::io_error)));

    ASSERT_EQ(_buf_file.seek(0, SEEK_SET), oc::failure(std::errc::io_error));
}

TEST_F(FileBufferedTest, SeekFlushFailed)
{
    open_mock();

    ASSERT_EQ(_buf_file.write("x", 1), oc::success(1u));

    // Once with flush() call in seek(), once in flush() call in destructor
    EXPECT_CALL(_mock_file, write(_, _))
            .Times(2)
            .WillRepeatedly(Return(std::make_error_code(std::errc::io_error)));

    ASSERT_EQ(_buf_file.seek(0, SEEK_SET), oc::failure(std::errc::io_error));
}

TEST_F(FileBufferedTest, TruncateSuccess)
{
    _buf_file.set_buf_size(5);
    open_dynamic_mem();

    // Truncate will flush any writes
    ASSERT_EQ(_buf_file.write("xyz", 3), oc::success(3u));
    ASSERT_THAT(_buf_file.wbuf(), ElementsAre('x', 'y', 'z'));
    ASSERT_TRUE(_buf_file.truncate(4));
    ASSERT_THAT(_buf_file.wbuf(), IsEmpty());
    ASSERT_THAT(span(static_cast<char *>(_dyn_data), _dyn_size),
                ElementsAre('x', 'y', 'z', 0));

    // Truncate will clear read buffer
    ASSERT_EQ(_buf_file.seek(0, SEEK_SET), oc::success(0u));
    ASSERT_TRUE(_buf_file.fill_rbuf());
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre('x', 'y', 'z', 0));
    ASSERT_TRUE(_buf_file.truncate(5));
    ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    ASSERT_THAT(span(static_cast<char *>(_dyn_data), _dyn_size),
                ElementsAre('x', 'y', 'z', 0, 0));
}

TEST_F(FileBufferedTest, TruncateFailed)
{
    open_mock();

    EXPECT_CALL(_mock_file, truncate(_))
            .Times(1)
            .WillOnce(Return(std::make_error_code(std::errc::io_error)));

    ASSERT_EQ(_buf_file.truncate(5), oc::failure(std::errc::io_error));
}

TEST_F(FileBufferedTest, SyncPosSuccess)
{
    char data[] = {1, 2, 3, 4, 5};
    _buf_file.set_buf_size(5);
    open_mem(data, sizeof(data));

    ASSERT_EQ(_buf_file.seek(0, SEEK_CUR), oc::success(0u));
    ASSERT_TRUE(_buf_file.fill_rbuf());
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(1, 2, 3, 4, 5));
    ASSERT_EQ(_mem_file.seek(3, SEEK_SET), oc::success(3u));
    ASSERT_TRUE(_buf_file.sync_pos());
    ASSERT_THAT(_buf_file.rbuf(), IsEmpty());
    ASSERT_EQ(_buf_file.seek(0, SEEK_CUR), oc::success(3u));
}

TEST_F(FileBufferedTest, SyncPosFailed)
{
    open_mock();

    ASSERT_EQ(_buf_file.seek(0, SEEK_CUR), oc::success(0u));

    EXPECT_CALL(_mock_file, seek(_, _))
            .Times(1)
            .WillOnce(Return(std::make_error_code(std::errc::io_error)));

    ASSERT_EQ(_buf_file.sync_pos(), oc::failure(std::errc::io_error));
    ASSERT_EQ(_buf_file.seek(0, SEEK_CUR), oc::success(0u));
}

TEST_F(FileBufferedTest, FlushSuccess)
{
    _buf_file.set_buf_size(2);
    open_dynamic_mem();

    // Flush with empty buffer
    ASSERT_TRUE(_buf_file.flush());
    ASSERT_THAT(_buf_file.wbuf(), IsEmpty());
    ASSERT_THAT(span(static_cast<char *>(_dyn_data), _dyn_size), IsEmpty());

    // Flush with non-empty buffer
    ASSERT_EQ(_buf_file.write("x", 1), oc::success(1u));
    ASSERT_THAT(_buf_file.wbuf(), ElementsAre('x'));
    ASSERT_TRUE(_buf_file.flush());
    ASSERT_THAT(_buf_file.wbuf(), IsEmpty());
    ASSERT_THAT(span(static_cast<char *>(_dyn_data), _dyn_size),
                ElementsAre('x'));
}

TEST_F(FileBufferedTest, FlushEof)
{
    _buf_file.set_buf_size(2);
    open_mock();

    // Once with explicit call, once in destructor
    EXPECT_CALL(_mock_file, write(_, _))
            .Times(2)
            .WillRepeatedly(Return(0u));

    ASSERT_EQ(_buf_file.write("x", 1), oc::success(1u));
    ASSERT_EQ(_buf_file.flush(), oc::failure(FileError::UnexpectedEof));
}

TEST_F(FileBufferedTest, FlushFailure)
{
    _buf_file.set_buf_size(2);
    open_mock();

    // Once with explicit call, once in destructor
    EXPECT_CALL(_mock_file, write(_, _))
            .Times(2)
            .WillRepeatedly(Return(std::make_error_code(std::errc::io_error)));

    ASSERT_EQ(_buf_file.write("x", 1), oc::success(1u));
    ASSERT_EQ(_buf_file.flush(), oc::failure(std::errc::io_error));
}

TEST_F(FileBufferedTest, FillRbufSuccess)
{
    char data[] = {1, 2, 3, 4, 5};
    _buf_file.set_buf_size(2);
    open_mem(data, sizeof(data));

    ASSERT_TRUE(_buf_file.fill_rbuf());
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(1, 2));
    ASSERT_TRUE(_buf_file.fill_rbuf());
    ASSERT_THAT(_buf_file.rbuf(), ElementsAre(1, 2));
}

TEST_F(FileBufferedTest, FillRbufFailure)
{
    open_mock();

    EXPECT_CALL(_mock_file, read(_, _))
            .Times(1)
            .WillOnce(Return(std::make_error_code(std::errc::io_error)));

    ASSERT_EQ(_buf_file.fill_rbuf(), oc::failure(std::errc::io_error));
}
