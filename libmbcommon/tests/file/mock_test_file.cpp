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

TestFile::TestFile() : TestFile(nullptr)
{
}

TestFile::TestFile(TestFileCounters *counters)
    : mb::File(new TestFilePrivate()), _counters(counters)
{
}

TestFile::~TestFile()
{
    close();
}

bool TestFile::on_open()
{
    if (_counters) {
        ++_counters->n_open;
    }

    // Generate some data
    for (size_t i = 0; i < INITIAL_BUF_SIZE; ++i) {
        _buf.push_back('a' + (i % 26));
    }

    return true;
}

bool TestFile::on_close()
{
    if (_counters) {
        ++_counters->n_close;
    }

    // Reset to allow opening another file
    _buf.clear();
    _position = 0;

    return true;
}

bool TestFile::on_read(void *buf, size_t size, size_t &bytes_read)
{
    if (_counters) {
        ++_counters->n_read;
    }

    size_t empty = _buf.size() - _position;
    uint64_t n = std::min<uint64_t>(empty, size);
    memcpy(buf, _buf.data() + _position, n);
    _position += n;
    bytes_read = n;

    return true;
}

bool TestFile::on_write(const void *buf, size_t size, size_t &bytes_written)
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
    bytes_written = size;

    return true;
}

bool TestFile::on_seek(int64_t offset, int whence, uint64_t &new_offset)
{
    if (_counters) {
        ++_counters->n_seek;
    }

    switch (whence) {
    case SEEK_SET:
        if (offset < 0) {
            set_error(mb::make_error_code(mb::FileError::ArgumentOutOfRange),
                      "Invalid SEET_SET offset %" PRId64, offset);
            return false;
        }
        new_offset = _position = offset;
        break;
    case SEEK_CUR:
        if (offset < 0 && static_cast<size_t>(-offset) > _position) {
            set_error(mb::make_error_code(mb::FileError::ArgumentOutOfRange),
                      "Invalid SEEK_CUR offset %" PRId64
                      " for position %" MB_PRIzu, offset, _position);
            return false;
        }
        new_offset = _position += offset;
        break;
    case SEEK_END:
        if (offset < 0 && static_cast<size_t>(-offset) > _buf.size()) {
            set_error(mb::make_error_code(mb::FileError::ArgumentOutOfRange),
                      "Invalid SEEK_END offset %" PRId64
                      " for file of size %" MB_PRIzu, offset, _buf.size());
            return false;
        }
        new_offset = _position = _buf.size() + offset;
        break;
    default:
        MB_UNREACHABLE("Invalid whence argument: %d", whence);
    }

    return true;
}

bool TestFile::on_truncate(uint64_t size)
{
    if (_counters) {
        ++_counters->n_truncate;
    }

    _buf.resize(size);
    return true;
}

MockTestFile::MockTestFile() : MockTestFile(nullptr)
{
}

MockTestFile::MockTestFile(TestFileCounters *counters) : TestFile(counters)
{
    // Call original methods by default
    ON_CALL(*this, on_open())
            .WillByDefault(testing::Invoke(this, &MockTestFile::orig_on_open));
    ON_CALL(*this, on_close())
            .WillByDefault(testing::Invoke(this, &MockTestFile::orig_on_close));
    ON_CALL(*this, on_read(testing::_, testing::_, testing::_))
            .WillByDefault(testing::Invoke(this, &MockTestFile::orig_on_read));
    ON_CALL(*this, on_write(testing::_, testing::_, testing::_))
            .WillByDefault(testing::Invoke(this, &MockTestFile::orig_on_write));
    ON_CALL(*this, on_seek(testing::_, testing::_, testing::_))
            .WillByDefault(testing::Invoke(this, &MockTestFile::orig_on_seek));
    ON_CALL(*this, on_truncate(testing::_))
            .WillByDefault(testing::Invoke(this, &MockTestFile::orig_on_truncate));
}

MockTestFile::~MockTestFile()
{
}

bool MockTestFile::orig_on_open()
{
    return TestFile::on_open();
}

bool MockTestFile::orig_on_close()
{
    return TestFile::on_close();
}

bool MockTestFile::orig_on_read(void *buf, size_t size, size_t &bytes_read)
{
    return TestFile::on_read(buf, size, bytes_read);
}

bool MockTestFile::orig_on_write(const void *buf, size_t size,
                                 size_t &bytes_written)
{
    return TestFile::on_write(buf, size, bytes_written);
}

bool MockTestFile::orig_on_seek(int64_t offset, int whence,
                                uint64_t &new_offset)
{
    return TestFile::on_seek(offset, whence, new_offset);
}

bool MockTestFile::orig_on_truncate(uint64_t size)
{
    return TestFile::on_truncate(size);
}
