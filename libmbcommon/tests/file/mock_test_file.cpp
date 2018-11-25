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

#include "mock_test_file.h"

#include <cinttypes>

#include "mbcommon/string.h"

using namespace mb;
using namespace testing;

TestFile::TestFile() : TestFile(nullptr)
{
}

TestFile::TestFile(TestFileCounters *counters)
    : File(), _counters(counters)
{
}

TestFile::~TestFile()
{
    (void) close();
}

TestFile::TestFile(TestFile &&other) noexcept
    : File(std::move(other))
    , _buf(std::move(other._buf))
    , _position(other._position)
    , _counters(other._counters)
{
}

TestFile & TestFile::operator=(TestFile &&rhs) noexcept
{
    File::operator=(std::move(rhs));

    _buf.swap(rhs._buf);
    _position = rhs._position;
    _counters = rhs._counters;

    return *this;
}

oc::result<void> TestFile::on_open()
{
    if (_counters) {
        ++_counters->n_open;
    }

    // Generate some data
    for (size_t i = 0; i < INITIAL_BUF_SIZE; ++i) {
        _buf.push_back(static_cast<unsigned char>('a' + i % 26));
    }

    return oc::success();
}

oc::result<void> TestFile::on_close()
{
    if (_counters) {
        ++_counters->n_close;
    }

    // Reset to allow opening another file
    _buf.clear();
    _position = 0;

    return oc::success();
}

oc::result<size_t> TestFile::on_read(void *buf, size_t size)
{
    if (_counters) {
        ++_counters->n_read;
    }

    size_t empty = _buf.size() - _position;
    size_t n = std::min(empty, size);
    memcpy(buf, _buf.data() + _position, n);
    _position += n;

    return n;
}

oc::result<size_t> TestFile::on_write(const void *buf, size_t size)
{
    if (_counters) {
        ++_counters->n_write;
    }

    size_t required = _position + size;
    if (required > _buf.size()) {
        _buf.resize(required);
    }

    memcpy(_buf.data() + _position, buf, size);
    _position += size;

    return size;
}

oc::result<uint64_t> TestFile::on_seek(int64_t offset, int whence)
{
    if (_counters) {
        ++_counters->n_seek;
    }

    switch (whence) {
    case SEEK_SET:
        if (offset < 0 || static_cast<uint64_t>(offset) > SIZE_MAX) {
            return FileError::ArgumentOutOfRange;
        }
        return _position = static_cast<size_t>(offset);
    case SEEK_CUR:
        if (offset < 0) {
            if (static_cast<uint64_t>(-offset) > _position) {
                return FileError::ArgumentOutOfRange;
            }
            return _position -= static_cast<size_t>(-offset);
        } else {
            if (static_cast<uint64_t>(offset) > SIZE_MAX - _position) {
                return FileError::ArgumentOutOfRange;
            }
            return _position += static_cast<size_t>(offset);
        }
    case SEEK_END:
        if (offset < 0) {
            if (static_cast<uint64_t>(-offset) > _buf.size()) {
                return FileError::ArgumentOutOfRange;
            }
            return _position = _buf.size() - static_cast<size_t>(-offset);
        } else {
            if (static_cast<uint64_t>(offset) > SIZE_MAX - _buf.size()) {
                return FileError::ArgumentOutOfRange;
            }
            return _position = _buf.size() + static_cast<size_t>(offset);
        }
    default:
        MB_UNREACHABLE("Invalid whence argument: %d", whence);
    }
}

oc::result<void> TestFile::on_truncate(uint64_t size)
{
    if (_counters) {
        ++_counters->n_truncate;
    }

    if (size > SIZE_MAX) {
        return FileError::ArgumentOutOfRange;
    }

    _buf.resize(static_cast<size_t>(size));
    return oc::success();
}

MockTestFile::MockTestFile() : MockTestFile(nullptr)
{
}

MockTestFile::MockTestFile(TestFileCounters *counters) : TestFile(counters)
{
    // Call original methods by default
    ON_CALL(*this, on_open())
            .WillByDefault(Invoke(this, &MockTestFile::orig_on_open));
    ON_CALL(*this, on_close())
            .WillByDefault(Invoke(this, &MockTestFile::orig_on_close));
    ON_CALL(*this, on_read(_, _))
            .WillByDefault(Invoke(this, &MockTestFile::orig_on_read));
    ON_CALL(*this, on_write(_, _))
            .WillByDefault(Invoke(this, &MockTestFile::orig_on_write));
    ON_CALL(*this, on_seek(_, _))
            .WillByDefault(Invoke(this, &MockTestFile::orig_on_seek));
    ON_CALL(*this, on_truncate(_))
            .WillByDefault(Invoke(this, &MockTestFile::orig_on_truncate));
}

MockTestFile::~MockTestFile()
{
}

oc::result<void> MockTestFile::orig_on_open()
{
    return TestFile::on_open();
}

oc::result<void> MockTestFile::orig_on_close()
{
    return TestFile::on_close();
}

oc::result<size_t> MockTestFile::orig_on_read(void *buf, size_t size)
{
    return TestFile::on_read(buf, size);
}

oc::result<size_t> MockTestFile::orig_on_write(const void *buf, size_t size)
{
    return TestFile::on_write(buf, size);
}

oc::result<uint64_t> MockTestFile::orig_on_seek(int64_t offset, int whence)
{
    return TestFile::on_seek(offset, whence);
}

oc::result<void> MockTestFile::orig_on_truncate(uint64_t size)
{
    return TestFile::on_truncate(size);
}
