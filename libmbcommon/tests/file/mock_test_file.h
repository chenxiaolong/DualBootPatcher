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
    mb::Expected<void> on_open() override;
    mb::Expected<void> on_close() override;
    mb::Expected<size_t> on_read(void *buf, size_t size) override;
    mb::Expected<size_t> on_write(const void *buf, size_t size) override;
    mb::Expected<uint64_t> on_seek(int64_t offset, int whence) override;
    mb::Expected<void> on_truncate(uint64_t size) override;

public:
    std::vector<unsigned char> _buf;
    size_t _position = 0;
    TestFileCounters *_counters;
};

struct MockTestFile : public TestFile
{
    MOCK_METHOD0(on_open, mb::Expected<void>());
    MOCK_METHOD0(on_close, mb::Expected<void>());
    MOCK_METHOD2(on_read, mb::Expected<size_t>(void *buf, size_t size));
    MOCK_METHOD2(on_write, mb::Expected<size_t>(const void *buf, size_t size));
    MOCK_METHOD2(on_seek, mb::Expected<uint64_t>(int64_t offset, int whence));
    MOCK_METHOD1(on_truncate, mb::Expected<void>(uint64_t size));

    MockTestFile();
    MockTestFile(TestFileCounters *counters);
    virtual ~MockTestFile();

    mb::Expected<void> orig_on_open();
    mb::Expected<void> orig_on_close();
    mb::Expected<size_t> orig_on_read(void *buf, size_t size);
    mb::Expected<size_t> orig_on_write(const void *buf, size_t size);
    mb::Expected<uint64_t> orig_on_seek(int64_t offset, int whence);
    mb::Expected<void> orig_on_truncate(uint64_t size);
};
