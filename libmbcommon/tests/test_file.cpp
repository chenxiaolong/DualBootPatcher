/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <gtest/gtest.h>

#include <cinttypes>

#include "mbcommon/file.h"
#include "mbcommon/file_p.h"
#include "mbcommon/string.h"

struct FileTest : testing::Test
{
    enum {
        INITIAL_BUF_SIZE = 1024,
    };

    MbFile *_file;
    std::vector<unsigned char> _buf;
    size_t _position = 0;

    // Callback counters
    int _n_open = 0;
    int _n_close = 0;
    int _n_read = 0;
    int _n_write = 0;
    int _n_seek = 0;
    int _n_truncate = 0;

    FileTest() : _file(mb_file_new())
    {
    }

    virtual ~FileTest()
    {
        mb_file_free(_file);
    }

    void set_all_callbacks()
    {
        ASSERT_EQ(mb_file_set_open_callback(_file, &_open_cb), MB_FILE_OK);
        ASSERT_EQ(_file->open_cb, &_open_cb);
        ASSERT_EQ(mb_file_set_close_callback(_file, &_close_cb), MB_FILE_OK);
        ASSERT_EQ(_file->close_cb, &_close_cb);
        ASSERT_EQ(mb_file_set_read_callback(_file, &_read_cb), MB_FILE_OK);
        ASSERT_EQ(_file->read_cb, &_read_cb);
        ASSERT_EQ(mb_file_set_write_callback(_file, &_write_cb), MB_FILE_OK);
        ASSERT_EQ(_file->write_cb, &_write_cb);
        ASSERT_EQ(mb_file_set_seek_callback(_file, &_seek_cb), MB_FILE_OK);
        ASSERT_EQ(_file->seek_cb, &_seek_cb);
        ASSERT_EQ(mb_file_set_truncate_callback(_file, &_truncate_cb), MB_FILE_OK);
        ASSERT_EQ(_file->truncate_cb, &_truncate_cb);
        ASSERT_EQ(mb_file_set_callback_data(_file, this), MB_FILE_OK);
        ASSERT_EQ(_file->cb_userdata, this);
    }

    static int _open_cb(MbFile *file, void *userdata)
    {
        (void) file;

        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_open;

        // Generate some data
        for (int i = 0; i < INITIAL_BUF_SIZE; ++i) {
            test->_buf.push_back('a' + (i % 26));
        }
        return MB_FILE_OK;
    }

    static int _close_cb(MbFile *file, void *userdata)
    {
        (void) file;

        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_close;

        test->_buf.clear();
        return MB_FILE_OK;
    }

    static int _read_cb(MbFile *file, void *userdata,
                        void *buf, size_t size,
                        size_t *bytes_read)
    {
        (void) file;

        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_read;

        size_t empty = test->_buf.size() - test->_position;
        uint64_t n = std::min<uint64_t>(empty, size);
        memcpy(buf, test->_buf.data() + test->_position, n);
        test->_position += n;
        *bytes_read = n;

        return MB_FILE_OK;
    }

    static int _write_cb(MbFile *file, void *userdata,
                         const void *buf, size_t size,
                         size_t *bytes_written)
    {
        (void) file;

        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_write;

        size_t required = test->_position + size;
        if (required > test->_buf.size()) {
            test->_buf.resize(required);
        }

        memcpy(test->_buf.data() + test->_position, buf, size);
        test->_position += size;
        *bytes_written = size;

        return MB_FILE_OK;
    }

    static int _seek_cb(MbFile *file, void *userdata,
                        int64_t offset, int whence,
                        uint64_t *new_offset)
    {
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_seek;

        switch (whence) {
        case SEEK_SET:
            if (offset < 0) {
                mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                                  "Invalid SEET_SET offset %" PRId64,
                                  offset);
                return MB_FILE_FAILED;
            }
            *new_offset = test->_position = offset;
            break;
        case SEEK_CUR:
            if (offset < 0 && static_cast<size_t>(-offset) > test->_position) {
                mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                                  "Invalid SEEK_CUR offset %" PRId64
                                  " for position %" MB_PRIzu,
                                  offset, test->_position);
                return MB_FILE_FAILED;
            }
            *new_offset = test->_position += offset;
            break;
        case SEEK_END:
            if (offset < 0 && static_cast<size_t>(-offset) > test->_buf.size()) {
                mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                                  "Invalid SEEK_END offset %" PRId64
                                  " for file of size %" MB_PRIzu,
                                  offset, test->_buf.size());
                return MB_FILE_FAILED;
            }
            *new_offset = test->_position = test->_buf.size() + offset;
            break;
        default:
            mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                              "Invalid whence argument: %d", whence);
            return MB_FILE_FAILED;
        }

        return MB_FILE_OK;
    }

    static int _truncate_cb(MbFile *file, void *userdata,
                            uint64_t size)
    {
        (void) file;

        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_truncate;

        test->_buf.resize(size);
        return MB_FILE_OK;
    }
};

TEST_F(FileTest, CheckInitialValues)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);
    ASSERT_EQ(_file->open_cb, nullptr);
    ASSERT_EQ(_file->close_cb, nullptr);
    ASSERT_EQ(_file->read_cb, nullptr);
    ASSERT_EQ(_file->write_cb, nullptr);
    ASSERT_EQ(_file->seek_cb, nullptr);
    ASSERT_EQ(_file->truncate_cb, nullptr);
    ASSERT_EQ(_file->cb_userdata, nullptr);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_NONE);
    ASSERT_EQ(_file->error_string, nullptr);
}

TEST_F(FileTest, CheckStatesNormal)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Close file
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::CLOSED);
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileTest, FreeNewFile)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Free file
    ASSERT_EQ(mb_file_free(_file), MB_FILE_OK);
    _file = nullptr;
}

TEST_F(FileTest, FreeNewFileWithRegisteredCallbacks)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    set_all_callbacks();

    // Free file
    ASSERT_EQ(mb_file_free(_file), MB_FILE_OK);
    _file = nullptr;

    // The close callback should not have been called because nothing was opened
    ASSERT_EQ(_n_close, 0);
}

TEST_F(FileTest, FreeOpenedFile)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Free file
    ASSERT_EQ(mb_file_free(_file), MB_FILE_OK);
    _file = nullptr;

    // Ensure that the close callback was called
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileTest, FreeClosedFile)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Close file
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::CLOSED);
    ASSERT_EQ(_n_close, 1);

    // Free file
    ASSERT_EQ(mb_file_free(_file), MB_FILE_OK);
    _file = nullptr;

    // Ensure that the close callback was not called again
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileTest, FreeFatalFile)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set read callback to return fatal error
    auto fatal_open_cb = [](MbFile *file, void *userdata,
                            void *buf, size_t size,
                            size_t *bytes_read) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_read;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_read;
        return MB_FILE_FATAL;
    };
    ASSERT_EQ(mb_file_set_read_callback(_file, fatal_open_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Read file
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_n_read, 1);

    // Free file
    ASSERT_EQ(mb_file_free(_file), MB_FILE_OK);
    _file = nullptr;

    // Ensure that the close callback was called
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileTest, SetCallbacksInNonNewState)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(_buf.size(), 0);
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_buf.size(), INITIAL_BUF_SIZE);
    ASSERT_EQ(_n_open, 1);

    ASSERT_EQ(mb_file_set_open_callback(_file, nullptr), MB_FILE_FATAL);
    ASSERT_NE(_file->open_cb, nullptr);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_set_open_callback"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));

    ASSERT_EQ(mb_file_set_close_callback(_file, nullptr), MB_FILE_FATAL);
    ASSERT_NE(_file->close_cb, nullptr);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_set_close_callback"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));

    ASSERT_EQ(mb_file_set_read_callback(_file, nullptr), MB_FILE_FATAL);
    ASSERT_NE(_file->read_cb, nullptr);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_set_read_callback"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));

    ASSERT_EQ(mb_file_set_write_callback(_file, nullptr), MB_FILE_FATAL);
    ASSERT_NE(_file->write_cb, nullptr);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_set_write_callback"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));

    ASSERT_EQ(mb_file_set_seek_callback(_file, nullptr), MB_FILE_FATAL);
    ASSERT_NE(_file->seek_cb, nullptr);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_set_seek_callback"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));

    ASSERT_EQ(mb_file_set_truncate_callback(_file, nullptr), MB_FILE_FATAL);
    ASSERT_NE(_file->truncate_cb, nullptr);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_set_truncate_callback"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));

    ASSERT_EQ(mb_file_set_callback_data(_file, nullptr), MB_FILE_FATAL);
    ASSERT_NE(_file->cb_userdata, nullptr);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_set_callback_data"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));
}

TEST_F(FileTest, OpenReturnNonFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set open callback
    auto open_cb = [](MbFile *file, void *userdata) -> int {
        (void) file;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_open;
        return MB_FILE_FAILED;
    };
    ASSERT_EQ(mb_file_set_open_callback(_file, open_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_FAILED);
    ASSERT_EQ(_file->state, MbFileState::NEW);
    ASSERT_EQ(_n_open, 1);

    // Reopen file
    ASSERT_EQ(mb_file_set_open_callback(_file, &_open_cb), MB_FILE_OK);
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 2);

    // The close callback should have been called to clean up resources
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileTest, OpenReturnFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set open callback
    auto open_cb = [](MbFile *file, void *userdata) -> int {
        (void) file;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_open;
        return MB_FILE_FATAL;
    };
    ASSERT_EQ(mb_file_set_open_callback(_file, open_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_n_open, 1);

    // The close callback should have been called to clean up resources
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileTest, OpenFileTwice)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Open again
    ASSERT_EQ(mb_file_open(_file), MB_FILE_FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_open"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));
    ASSERT_EQ(_n_open, 1);
}

TEST_F(FileTest, OpenNoCallback)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Clear open callback
    ASSERT_EQ(mb_file_set_open_callback(_file, nullptr), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 0);
}

TEST_F(FileTest, CloseNewFile)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    set_all_callbacks();

    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::CLOSED);
    ASSERT_EQ(_n_close, 0);
}

TEST_F(FileTest, CloseFileTwice)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Close file
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::CLOSED);
    ASSERT_EQ(_n_close, 1);

    // Close file again
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::CLOSED);
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileTest, CloseNoCallback)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Clear close callback
    mb_file_set_close_callback(_file, nullptr);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Close file
    ASSERT_EQ(mb_file_close(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::CLOSED);
    ASSERT_EQ(_n_close, 0);
}

TEST_F(FileTest, CloseReturnNonFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set open callback
    auto close_cb = [](MbFile *file, void *userdata) -> int {
        (void) file;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_close;
        return MB_FILE_FAILED;
    };
    ASSERT_EQ(mb_file_set_close_callback(_file, close_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Close file
    ASSERT_EQ(mb_file_close(_file), MB_FILE_FAILED);
    ASSERT_EQ(_file->state, MbFileState::CLOSED);
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileTest, CloseReturnFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set open callback
    auto close_cb = [](MbFile *file, void *userdata) -> int {
        (void) file;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_close;
        return MB_FILE_FATAL;
    };
    ASSERT_EQ(mb_file_set_close_callback(_file, close_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Close file
    ASSERT_EQ(mb_file_close(_file), MB_FILE_FATAL);
    // mb_file_close() always results in the state changing to CLOSED
    ASSERT_EQ(_file->state, MbFileState::CLOSED);
    ASSERT_EQ(_n_close, 1);
}

TEST_F(FileTest, ReadCallbackCalled)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Read from file
    char buf[10];
    size_t n;
    ASSERT_EQ(mb_file_read(_file, buf, sizeof(buf), &n), MB_FILE_OK);
    ASSERT_EQ(n, sizeof(buf));
    ASSERT_EQ(memcmp(buf, _buf.data(), sizeof(buf)), 0);
    ASSERT_EQ(_n_read, 1);
}

TEST_F(FileTest, ReadInWrongState)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Read from file
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_read"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));
    ASSERT_EQ(_n_read, 0);
}

TEST_F(FileTest, ReadWithNullBytesReadParam)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Read from file
    char c;
    ASSERT_EQ(mb_file_read(_file, &c, 1, nullptr), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_read"));
    ASSERT_TRUE(strstr(_file->error_string, "is NULL"));
    ASSERT_EQ(_n_read, 0);
}

TEST_F(FileTest, ReadNoCallback)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Clear read callback
    mb_file_set_read_callback(_file, nullptr);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Read from file
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_UNSUPPORTED);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_UNSUPPORTED);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_read"));
    ASSERT_TRUE(strstr(_file->error_string, "read callback"));
    ASSERT_EQ(_n_read, 0);
}

TEST_F(FileTest, ReadReturnNonFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set read callback
    auto read_cb = [](MbFile *file, void *userdata,
                      void *buf, size_t size,
                      size_t *bytes_read) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_read;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_read;
        return MB_FILE_FAILED;
    };
    ASSERT_EQ(mb_file_set_read_callback(_file, read_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Read from file
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_FAILED);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_read, 1);
}

TEST_F(FileTest, ReadReturnFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set read callback
    auto read_cb = [](MbFile *file, void *userdata,
                      void *buf, size_t size,
                      size_t *bytes_read) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_read;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_read;
        return MB_FILE_FATAL;
    };
    ASSERT_EQ(mb_file_set_read_callback(_file, read_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Read from file
    char c;
    size_t n;
    ASSERT_EQ(mb_file_read(_file, &c, 1, &n), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_n_read, 1);
}

TEST_F(FileTest, WriteCallbackCalled)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Write to file
    char buf[] = "Hello, world!";
    size_t size = strlen(buf);
    size_t n;
    ASSERT_EQ(mb_file_write(_file, buf, size, &n), MB_FILE_OK);
    ASSERT_EQ(n, size);
    ASSERT_EQ(memcmp(buf, _buf.data(), size), 0);
    ASSERT_EQ(_n_write, 1);
}

TEST_F(FileTest, WriteInWrongState)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Write to file
    char c;
    size_t n;
    ASSERT_EQ(mb_file_write(_file, &c, 1, &n), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_write"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));
    ASSERT_EQ(_n_write, 0);
}

TEST_F(FileTest, WriteWithNullBytesReadParam)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Write to file
    ASSERT_EQ(mb_file_write(_file, "x", 1, nullptr), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_write"));
    ASSERT_TRUE(strstr(_file->error_string, "is NULL"));
    ASSERT_EQ(_n_write, 0);
}

TEST_F(FileTest, WriteNoCallback)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Clear write callback
    mb_file_set_write_callback(_file, nullptr);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Write to file
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_UNSUPPORTED);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_UNSUPPORTED);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_write"));
    ASSERT_TRUE(strstr(_file->error_string, "write callback"));
    ASSERT_EQ(_n_write, 0);
}

TEST_F(FileTest, WriteReturnNonFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set write callback
    auto write_cb = [](MbFile *file, void *userdata,
                       const void *buf, size_t size,
                       size_t *bytes_written) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_written;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_write;
        return MB_FILE_FAILED;
    };
    ASSERT_EQ(mb_file_set_write_callback(_file, write_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Write to file
    size_t n;
    ASSERT_EQ(mb_file_write(_file, "x", 1, &n), MB_FILE_FAILED);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_write, 1);
}

TEST_F(FileTest, WriteReturnFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set write callback
    auto write_cb = [](MbFile *file, void *userdata,
                      const void *buf, size_t size,
                      size_t *bytes_written) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_written;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_write;
        return MB_FILE_FATAL;
    };
    ASSERT_EQ(mb_file_set_write_callback(_file, write_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Write to file
    char c;
    size_t n;
    ASSERT_EQ(mb_file_write(_file, &c, 1, &n), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_n_write, 1);
}

TEST_F(FileTest, SeekCallbackCalled)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Seek file
    uint64_t pos;
    ASSERT_EQ(mb_file_seek(_file, 0, SEEK_END, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, _buf.size());
    ASSERT_EQ(pos, _position);
    ASSERT_EQ(_n_seek, 1);

    // Seek again with NULL offset output parameter
    ASSERT_EQ(mb_file_seek(_file, -10, SEEK_END, nullptr), MB_FILE_OK);
    ASSERT_EQ(_position, _buf.size() - 10);
    ASSERT_EQ(_n_seek, 2);
}

TEST_F(FileTest, SeekInWrongState)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Seek file
    ASSERT_EQ(mb_file_seek(_file, 0, SEEK_END, nullptr), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_seek"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));
    ASSERT_EQ(_n_seek, 0);
}

TEST_F(FileTest, SeekNoCallback)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Clear seek callback
    mb_file_set_seek_callback(_file, nullptr);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Seek file
    ASSERT_EQ(mb_file_seek(_file, 0, SEEK_END, nullptr), MB_FILE_UNSUPPORTED);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_UNSUPPORTED);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_seek"));
    ASSERT_TRUE(strstr(_file->error_string, "seek callback"));
    ASSERT_EQ(_n_seek, 0);
}

TEST_F(FileTest, SeekReturnNonFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set seek callback
    auto seek_cb = [](MbFile *file, void *userdata,
                      int64_t offset, int whence,
                      uint64_t *new_offset) -> int {
        (void) file;
        (void) offset;
        (void) whence;
        (void) new_offset;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_seek;
        return MB_FILE_FAILED;
    };
    ASSERT_EQ(mb_file_set_seek_callback(_file, seek_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Seek file
    ASSERT_EQ(mb_file_seek(_file, 0, SEEK_END, nullptr), MB_FILE_FAILED);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_seek, 1);
}

TEST_F(FileTest, SeekReturnFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set seek callback
    auto seek_cb = [](MbFile *file, void *userdata,
                      int64_t offset, int whence,
                      uint64_t *new_offset) -> int {
        (void) file;
        (void) offset;
        (void) whence;
        (void) new_offset;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_seek;
        return MB_FILE_FATAL;
    };
    ASSERT_EQ(mb_file_set_seek_callback(_file, seek_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Seek file
    ASSERT_EQ(mb_file_seek(_file, 0, SEEK_END, nullptr), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_n_seek, 1);
}

TEST_F(FileTest, TruncateCallbackCalled)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Truncate file
    ASSERT_EQ(mb_file_truncate(_file, INITIAL_BUF_SIZE / 2), MB_FILE_OK);
    ASSERT_EQ(_n_truncate, 1);
}

TEST_F(FileTest, TruncateInWrongState)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Truncate file
    ASSERT_EQ(mb_file_truncate(_file, INITIAL_BUF_SIZE + 1), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_PROGRAMMER_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_truncate"));
    ASSERT_TRUE(strstr(_file->error_string, "Invalid state"));
    ASSERT_EQ(_n_truncate, 0);
}

TEST_F(FileTest, TruncateNoCallback)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Clear truncate callback
    mb_file_set_truncate_callback(_file, nullptr);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Truncate file
    ASSERT_EQ(mb_file_truncate(_file, INITIAL_BUF_SIZE + 1),
              MB_FILE_UNSUPPORTED);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_UNSUPPORTED);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_TRUE(strstr(_file->error_string, "mb_file_truncate"));
    ASSERT_TRUE(strstr(_file->error_string, "truncate callback"));
    ASSERT_EQ(_n_truncate, 0);
}

TEST_F(FileTest, TruncateReturnNonFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set truncate callback
    auto truncate_cb = [](MbFile *file, void *userdata,
                          uint64_t size) -> int {
        (void) file;
        (void) size;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_truncate;
        return MB_FILE_FAILED;
    };
    ASSERT_EQ(mb_file_set_truncate_callback(_file, truncate_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Truncate file
    ASSERT_EQ(mb_file_truncate(_file, INITIAL_BUF_SIZE + 1), MB_FILE_FAILED);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_truncate, 1);
}

TEST_F(FileTest, TruncateReturnFatalFailure)
{
    ASSERT_EQ(_file->state, MbFileState::NEW);

    // Set callbacks
    set_all_callbacks();

    // Set truncate callback
    auto truncate_cb = [](MbFile *file, void *userdata,
                          uint64_t size) -> int {
        (void) file;
        (void) size;
        FileTest *test = static_cast<FileTest *>(userdata);
        ++test->_n_truncate;
        return MB_FILE_FATAL;
    };
    ASSERT_EQ(mb_file_set_truncate_callback(_file, truncate_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);
    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_n_open, 1);

    // Truncate file
    ASSERT_EQ(mb_file_truncate(_file, INITIAL_BUF_SIZE + 1), MB_FILE_FATAL);
    ASSERT_EQ(_file->state, MbFileState::FATAL);
    ASSERT_EQ(_n_truncate, 1);
}

TEST_F(FileTest, SetError)
{
    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_NONE);
    ASSERT_EQ(_file->error_string, nullptr);

    ASSERT_EQ(mb_file_set_error(_file, MB_FILE_ERROR_INTERNAL_ERROR,
                                "%s, %s!", "Hello", "world"), MB_FILE_OK);

    ASSERT_EQ(_file->error_code, MB_FILE_ERROR_INTERNAL_ERROR);
    ASSERT_NE(_file->error_string, nullptr);
    ASSERT_EQ(mb_file_error(_file), _file->error_code);
    ASSERT_STREQ(mb_file_error_string(_file), _file->error_string);
}
