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

#include <gtest/gtest.h>

#include <memory>

#include <cinttypes>

#include "mbcommon/file/memory.h"
#include "mbcommon/file_p.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"


typedef std::unique_ptr<MbFile, decltype(mb_file_free) *> ScopedFile;

struct FileUtilTest : testing::Test
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

    FileUtilTest() : _file(mb_file_new())
    {
    }

    virtual ~FileUtilTest()
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

        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
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

        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_close;

        test->_buf.clear();
        return MB_FILE_OK;
    }

    static int _read_cb(MbFile *file, void *userdata,
                        void *buf, size_t size,
                        size_t *bytes_read)
    {
        (void) file;

        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
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

        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
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
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
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

        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_truncate;

        test->_buf.resize(size);
        return MB_FILE_OK;
    }
};

TEST_F(FileUtilTest, ReadFullyNormal)
{
    set_all_callbacks();

    auto read_cb = [](MbFile *file, void *userdata,
                      void *buf, size_t size,
                      size_t *bytes_read) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_read;
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_read;
        *bytes_read = 2;
        return MB_FILE_OK;
    };
    ASSERT_EQ(mb_file_set_read_callback(_file, read_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);

    char buf[10];
    size_t n;
    ASSERT_EQ(mb_file_read_fully(_file, buf, sizeof(buf), &n), MB_FILE_OK);
    ASSERT_EQ(n, 10);
    ASSERT_EQ(_n_read, 5);
}

TEST_F(FileUtilTest, ReadFullyEOF)
{
    set_all_callbacks();

    auto read_cb = [](MbFile *file, void *userdata,
                      void *buf, size_t size,
                      size_t *bytes_read) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_read;
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_read;
        switch (test->_n_read) {
        case 1:
        case 2:
        case 3:
        case 4:
            *bytes_read = 2;
            break;
        default:
            *bytes_read = 0;
            break;
        }
        return MB_FILE_OK;
    };
    ASSERT_EQ(mb_file_set_read_callback(_file, read_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);

    char buf[10];
    size_t n;
    ASSERT_EQ(mb_file_read_fully(_file, buf, sizeof(buf), &n), MB_FILE_OK);
    ASSERT_EQ(n, 8);
    ASSERT_EQ(_n_read, 5);
}

TEST_F(FileUtilTest, ReadFullyPartialFail)
{
    set_all_callbacks();

    auto read_cb = [](MbFile *file, void *userdata,
                      void *buf, size_t size,
                      size_t *bytes_read) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_read;
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_read;
        switch (test->_n_read) {
        case 1:
        case 2:
        case 3:
        case 4:
            *bytes_read = 2;
            return MB_FILE_OK;
        default:
            *bytes_read = 0;
            return MB_FILE_FAILED;
        }
    };
    ASSERT_EQ(mb_file_set_read_callback(_file, read_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);

    char buf[10];
    size_t n;
    ASSERT_EQ(mb_file_read_fully(_file, buf, sizeof(buf), &n), MB_FILE_FAILED);
    ASSERT_EQ(n, 8);
    ASSERT_EQ(_n_read, 5);
}

TEST_F(FileUtilTest, WriteFullyNormal)
{
    set_all_callbacks();

    auto write_cb = [](MbFile *file, void *userdata,
                       const void *buf, size_t size,
                       size_t *bytes_written) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_written;
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_write;
        *bytes_written = 2;
        return MB_FILE_OK;
    };
    ASSERT_EQ(mb_file_set_write_callback(_file, write_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);

    size_t n;
    ASSERT_EQ(mb_file_write_fully(_file, "xxxxxxxxxx", 10, &n), MB_FILE_OK);
    ASSERT_EQ(n, 10);
    ASSERT_EQ(_n_write, 5);
}

TEST_F(FileUtilTest, WriteFullyEOF)
{
    set_all_callbacks();

    auto write_cb = [](MbFile *file, void *userdata,
                       const void *buf, size_t size,
                       size_t *bytes_written) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_written;
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_write;
        switch (test->_n_write) {
        case 1:
        case 2:
        case 3:
        case 4:
            *bytes_written = 2;
            break;
        default:
            *bytes_written = 0;
            break;
        }
        return MB_FILE_OK;
    };
    ASSERT_EQ(mb_file_set_write_callback(_file, write_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);

    size_t n;
    ASSERT_EQ(mb_file_write_fully(_file, "xxxxxxxxxx", 10, &n), MB_FILE_OK);
    ASSERT_EQ(n, 8);
    ASSERT_EQ(_n_write, 5);
}

TEST_F(FileUtilTest, WriteFullyPartialFail)
{
    set_all_callbacks();

    auto write_cb = [](MbFile *file, void *userdata,
                       const void *buf, size_t size,
                       size_t *bytes_written) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_written;
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_write;
        switch (test->_n_write) {
        case 1:
        case 2:
        case 3:
        case 4:
            *bytes_written = 2;
            return MB_FILE_OK;
        default:
            *bytes_written = 0;
            return MB_FILE_FAILED;
        }
    };
    ASSERT_EQ(mb_file_set_write_callback(_file, write_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);

    size_t n;
    ASSERT_EQ(mb_file_write_fully(_file, "xxxxxxxxxx", 10, &n), MB_FILE_FAILED);
    ASSERT_EQ(n, 8);
    ASSERT_EQ(_n_write, 5);
}

TEST_F(FileUtilTest, ReadDiscardNormal)
{
    set_all_callbacks();

    auto read_cb = [](MbFile *file, void *userdata,
                      void *buf, size_t size,
                      size_t *bytes_read) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_read;
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_read;
        *bytes_read = 2;
        return MB_FILE_OK;
    };
    ASSERT_EQ(mb_file_set_read_callback(_file, read_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);

    uint64_t n;
    ASSERT_EQ(mb_file_read_discard(_file, 10, &n), MB_FILE_OK);
    ASSERT_EQ(n, 10);
    ASSERT_EQ(_n_read, 5);
}

TEST_F(FileUtilTest, ReadDiscardEOF)
{
    set_all_callbacks();

    auto read_cb = [](MbFile *file, void *userdata,
                      void *buf, size_t size,
                      size_t *bytes_read) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_read;
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_read;
        switch (test->_n_read) {
        case 1:
        case 2:
        case 3:
        case 4:
            *bytes_read = 2;
            break;
        default:
            *bytes_read = 0;
            break;
        }
        return MB_FILE_OK;
    };
    ASSERT_EQ(mb_file_set_read_callback(_file, read_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);

    uint64_t n;
    ASSERT_EQ(mb_file_read_discard(_file, 10, &n), MB_FILE_OK);
    ASSERT_EQ(n, 8);
    ASSERT_EQ(_n_read, 5);
}

TEST_F(FileUtilTest, ReadDiscardPartialFail)
{
    set_all_callbacks();

    auto read_cb = [](MbFile *file, void *userdata,
                      void *buf, size_t size,
                      size_t *bytes_read) -> int {
        (void) file;
        (void) buf;
        (void) size;
        (void) bytes_read;
        FileUtilTest *test = static_cast<FileUtilTest *>(userdata);
        ++test->_n_read;
        switch (test->_n_read) {
        case 1:
        case 2:
        case 3:
        case 4:
            *bytes_read = 2;
            return MB_FILE_OK;
        default:
            *bytes_read = 0;
            return MB_FILE_FAILED;
        }
    };
    ASSERT_EQ(mb_file_set_read_callback(_file, read_cb), MB_FILE_OK);

    // Open file
    ASSERT_EQ(mb_file_open(_file), MB_FILE_OK);

    uint64_t n;
    ASSERT_EQ(mb_file_read_discard(_file, 10, &n), MB_FILE_FAILED);
    ASSERT_EQ(n, 8);
    ASSERT_EQ(_n_read, 5);
}

struct FileSearchTest : testing::Test
{
    MbFile *_file;

    // Callback counters
    int _n_result = 0;

    FileSearchTest() : _file(mb_file_new())
    {
    }

    virtual ~FileSearchTest()
    {
        mb_file_free(_file);
    }

    static int _result_cb(MbFile *file, void *userdata, uint64_t offset)
    {
        (void) file;
        (void) offset;

        FileSearchTest *test = static_cast<FileSearchTest *>(userdata);
        ++test->_n_result;

        return MB_FILE_OK;
    }
};

TEST_F(FileSearchTest, CheckInvalidBoundariesFail)
{
    ASSERT_EQ(mb_file_open_memory_static(_file, "", 0), MB_FILE_OK);

    ASSERT_EQ(mb_file_search(_file, 20, 10, 0, "x", 1, -1,
                             &_result_cb, this), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(_file), "offset"));
}

TEST_F(FileSearchTest, CheckZeroMaxMatches)
{
    ASSERT_EQ(mb_file_open_memory_static(_file, "", 0), MB_FILE_OK);

    ASSERT_EQ(mb_file_search(_file, -1, -1, 0, "x", 1, 0,
                             &_result_cb, this), MB_FILE_OK);
}

TEST_F(FileSearchTest, CheckZeroPatternSize)
{
    ASSERT_EQ(mb_file_open_memory_static(_file, "", 0), MB_FILE_OK);

    ASSERT_EQ(mb_file_search(_file, -1, -1, 0, nullptr, 0, -1,
                             &_result_cb, this), MB_FILE_OK);
}

TEST_F(FileSearchTest, CheckBufferSize)
{
    ASSERT_EQ(mb_file_open_memory_static(_file, "", 0), MB_FILE_OK);

    // Auto buffer size
    ASSERT_EQ(mb_file_search(_file, -1, -1, 0, "x", 1, -1,
                             &_result_cb, this), MB_FILE_OK);

    // Too small
    ASSERT_EQ(mb_file_search(_file, -1, -1, 1, "xxx", 3, -1,
                             &_result_cb, this), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(_file), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(_file), "Buffer size"));

    // Equal to pattern size
    ASSERT_EQ(mb_file_search(_file, -1, -1, 1, "x", 1, -1,
                             &_result_cb, this), MB_FILE_OK);
}

TEST_F(FileSearchTest, FindNormal)
{
    ASSERT_EQ(mb_file_open_memory_static(_file, "abc", 3), MB_FILE_OK);

    ASSERT_EQ(mb_file_search(_file, -1, -1, 0, "a", 1, -1,
                             &_result_cb, this), MB_FILE_OK);
}

TEST(FileMoveTest, DegenerateCasesShouldSucceed)
{
    char buf[] = "abcdef";
    uint64_t n;

    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), buf, sizeof(buf) - 1),
              MB_FILE_OK);

    // src == dest
    ASSERT_EQ(mb_file_move(file.get(), 0, 0, 3, &n), MB_FILE_OK);

    // size == 0
    ASSERT_EQ(mb_file_move(file.get(), 3, 0, 0, &n), MB_FILE_OK);
}

TEST(FileMoveTest, NormalForwardsCopyShouldSucceed)
{
    char buf[] = "abcdef";
    uint64_t n;

    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), buf, sizeof(buf) - 1),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_move(file.get(), 2, 0, 3, &n), MB_FILE_OK);
    ASSERT_EQ(n, 3);
    ASSERT_STREQ(buf, "cdedef");
}

TEST(FileMoveTest, NormalBackwardsCopyShouldSucceed)
{
    char buf[] = "abcdef";
    uint64_t n;

    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), buf, sizeof(buf) - 1),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_move(file.get(), 0, 2, 3, &n), MB_FILE_OK);
    ASSERT_EQ(n, 3);
    ASSERT_STREQ(buf, "ababcf");
}

TEST(FileMoveTest, OutOfBoundsForwardsCopyShouldCopyPartially)
{
    char buf[] = "abcdef";
    uint64_t n;

    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), buf, sizeof(buf) - 1),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_move(file.get(), 2, 0, 5, &n), MB_FILE_OK);
    ASSERT_EQ(n, 4);
    ASSERT_STREQ(buf, "cdefef");
}

TEST(FileMoveTest, OutOfBoundsBackwardsCopyShouldCopyPartially)
{
    char buf[] = "abcdef";
    uint64_t n;

    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), buf, sizeof(buf) - 1),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_move(file.get(), 0, 2, 5, &n), MB_FILE_OK);
    ASSERT_EQ(n, 4);
    ASSERT_STREQ(buf, "ababcd");
}

TEST(FileMoveTest, LargeForwardsCopyShouldSucceed)
{
    char *buf;
    size_t buf_size = 100000;
    uint64_t n;

    buf = static_cast<char *>(malloc(buf_size));
    ASSERT_TRUE(!!buf);

    memset(buf, 'a', buf_size / 2);
    memset(buf + buf_size / 2, 'b', buf_size / 2);

    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), buf, buf_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_move(file.get(), buf_size / 2, 0, buf_size / 2, &n),
              MB_FILE_OK);
    ASSERT_EQ(n, buf_size / 2);

    for (size_t i = 0; i < buf_size; ++i) {
        ASSERT_EQ(buf[i], 'b');
    }

    free(buf);
}

TEST(FileMoveTest, LargeBackwardsCopyShouldSucceed)
{
    char *buf;
    size_t buf_size = 100000;
    uint64_t n;

    buf = static_cast<char *>(malloc(buf_size));
    ASSERT_TRUE(!!buf);

    memset(buf, 'a', buf_size / 2);
    memset(buf + buf_size / 2, 'b', buf_size / 2);

    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), buf, buf_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_move(file.get(), 0, buf_size / 2, buf_size / 2, &n),
              MB_FILE_OK);
    ASSERT_EQ(n, buf_size / 2);

    for (size_t i = 0; i < buf_size; ++i) {
        ASSERT_EQ(buf[i], 'a');
    }

    free(buf);
}

// TODO: Add more tests after integrating gmock
