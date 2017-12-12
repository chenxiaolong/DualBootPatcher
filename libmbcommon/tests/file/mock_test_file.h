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

#include "mbcommon/file.h"

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

class TestFile : public mb::File
{
public:
    TestFile();
    TestFile(TestFileCounters *counters);
    virtual ~TestFile();

    TestFile(TestFile &&other) noexcept;
    TestFile & operator=(TestFile &&rhs) noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(TestFile)

    // Make open function public since we don't need/have a parameterized open
    // function
    using mb::File::open;

    using mb::File::state;
    using mb::File::set_state;

protected:
    mb::oc::result<void> on_open() override;
    mb::oc::result<void> on_close() override;
    mb::oc::result<size_t> on_read(void *buf, size_t size) override;
    mb::oc::result<size_t> on_write(const void *buf, size_t size) override;
    mb::oc::result<uint64_t> on_seek(int64_t offset, int whence) override;
    mb::oc::result<void> on_truncate(uint64_t size) override;

public:
    std::vector<unsigned char> _buf;
    size_t _position = 0;
    TestFileCounters *_counters;
};

struct MockTestFile : public TestFile
{
    MOCK_METHOD0(on_open, mb::oc::result<void>());
    MOCK_METHOD0(on_close, mb::oc::result<void>());
    MOCK_METHOD2(on_read, mb::oc::result<size_t>(void *buf, size_t size));
    MOCK_METHOD2(on_write, mb::oc::result<size_t>(const void *buf, size_t size));
    MOCK_METHOD2(on_seek, mb::oc::result<uint64_t>(int64_t offset, int whence));
    MOCK_METHOD1(on_truncate, mb::oc::result<void>(uint64_t size));

    MockTestFile();
    MockTestFile(TestFileCounters *counters);
    virtual ~MockTestFile();

    mb::oc::result<void> orig_on_open();
    mb::oc::result<void> orig_on_close();
    mb::oc::result<size_t> orig_on_read(void *buf, size_t size);
    mb::oc::result<size_t> orig_on_write(const void *buf, size_t size);
    mb::oc::result<uint64_t> orig_on_seek(int64_t offset, int whence);
    mb::oc::result<void> orig_on_truncate(uint64_t size);
};
