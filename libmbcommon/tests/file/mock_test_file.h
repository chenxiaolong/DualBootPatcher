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

#pragma once

#include <vector>

#include <gmock/gmock.h>

#include "mbcommon/file_p.h"

static constexpr size_t INITIAL_BUF_SIZE = 1024;

// Used for destructor tests, which gmock can't handle properly in our setup
struct TestFileCounters
{
    unsigned int n_open = 0;
    unsigned int n_close = 0;
    unsigned int n_read = 0;
    unsigned int n_write = 0;
    unsigned int n_seek = 0;
    unsigned int n_truncate = 0;
};

class TestFilePrivate : public mb::FilePrivate
{
};

class TestFile : public mb::File
{
public:
    MB_DECLARE_PRIVATE(TestFile)

    TestFile();
    TestFile(TestFileCounters *counters);
    virtual ~TestFile();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(TestFile)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(TestFile)

    // Make open function public since we don't need/have a parameterized open
    // function
    using mb::File::open;

protected:
    virtual mb::FileStatus on_open() override;
    virtual mb::FileStatus on_close() override;
    virtual mb::FileStatus on_read(void *buf, size_t size,
                                   size_t &bytes_read) override;
    virtual mb::FileStatus on_write(const void *buf, size_t size,
                                    size_t &bytes_written) override;
    virtual mb::FileStatus on_seek(int64_t offset, int whence,
                                   uint64_t &new_offset) override;
    virtual mb::FileStatus on_truncate(uint64_t size) override;

public:
    std::vector<unsigned char> _buf;
    size_t _position = 0;
    TestFileCounters *_counters;
};

struct MockTestFile : public TestFile
{
    MOCK_METHOD0(on_open, mb::FileStatus());
    MOCK_METHOD0(on_close, mb::FileStatus());
    MOCK_METHOD3(on_read, mb::FileStatus(void *buf, size_t size,
                                         size_t &bytes_read));
    MOCK_METHOD3(on_write, mb::FileStatus(const void *buf, size_t size,
                                          size_t &bytes_written));
    MOCK_METHOD3(on_seek, mb::FileStatus(int64_t offset, int whence,
                                         uint64_t &new_offset));
    MOCK_METHOD1(on_truncate, mb::FileStatus(uint64_t size));

    MockTestFile();
    MockTestFile(TestFileCounters *counters);
    virtual ~MockTestFile();

    mb::FileStatus orig_on_open();
    mb::FileStatus orig_on_close();
    mb::FileStatus orig_on_read(void *buf, size_t size, size_t &bytes_read);
    mb::FileStatus orig_on_write(const void *buf, size_t size,
                                 size_t &bytes_written);
    mb::FileStatus orig_on_seek(int64_t offset, int whence,
                                uint64_t &new_offset);
    mb::FileStatus orig_on_truncate(uint64_t size);
};
