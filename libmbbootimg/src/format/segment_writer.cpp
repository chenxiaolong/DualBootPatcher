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

#include "mbbootimg/format/segment_writer_p.h"

#include <algorithm>

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/align_p.h"
#include "mbbootimg/format/segment_error_p.h"

namespace mb
{
namespace bootimg
{

SegmentWriter::SegmentWriter()
    : _state(SegmentWriterState::Begin)
    , _entries()
    , _entries_len()
    , _entry()
    , _entry_size()
    , _have_pos()
    , _pos()
{
}

size_t SegmentWriter::entries_size() const
{
    return _entries_len;
}

void SegmentWriter::entries_clear()
{
    _entries_len = 0;
    _entry = nullptr;
}

int SegmentWriter::entries_add(int type, uint32_t size, bool size_set,
                               uint64_t align, Writer &writer)
{
    if (_state != SegmentWriterState::Begin) {
        writer.set_error(make_error_code(
                SegmentError::AddEntryInIncorrectState));
        return RET_FATAL;
    }

    if (_entries_len == sizeof(_entries) / sizeof(_entries[0])) {
        writer.set_error(make_error_code(SegmentError::TooManyEntries));
        return RET_FATAL;
    }

    SegmentWriterEntry *entry = &_entries[_entries_len];
    entry->type = type;
    entry->offset = 0;
    entry->size = size;
    entry->size_set = size_set;
    entry->align = align;

    ++_entries_len;

    return RET_OK;
}

const SegmentWriterEntry * SegmentWriter::entries_get(size_t index)
{
    if (index < _entries_len) {
        return &_entries[index];
    } else {
        return nullptr;
    }
}

const SegmentWriterEntry * SegmentWriter::entry() const
{
    switch (_state) {
    case SegmentWriterState::Entries:
        return _entry;
    default:
        return nullptr;
    }
}

SegmentWriterEntry * SegmentWriter::next_entry()
{
    switch (_state) {
    case SegmentWriterState::Begin:
        if (_entries_len > 0) {
            return _entries;
        } else {
            return nullptr;
        }
    case SegmentWriterState::Entries:
        if (static_cast<size_t>(_entry - _entries + 1) == _entries_len) {
            return nullptr;
        } else {
            return _entry + 1;
        }
    default:
        return nullptr;
    }
}

const SegmentWriterEntry * SegmentWriter::next_entry() const
{
    switch (_state) {
    case SegmentWriterState::Begin:
        if (_entries_len > 0) {
            return _entries;
        } else {
            return nullptr;
        }
    case SegmentWriterState::Entries:
        if (static_cast<size_t>(_entry - _entries + 1) == _entries_len) {
            return nullptr;
        } else {
            return _entry + 1;
        }
    default:
        return nullptr;
    }
}

void SegmentWriter::update_size_if_unset(uint32_t size)
{
    if (!_entry->size_set) {
        _entry->size = size;
        _entry->size_set = true;
    }
}

int SegmentWriter::get_entry(File &file, Entry &entry, Writer &writer)
{
    if (!_have_pos) {
        if (!file.seek(0, SEEK_CUR, &_pos)) {
            writer.set_error(file.error(),
                             "Failed to get current offset: %s",
                             file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        _have_pos = true;
    }

    // Get next entry
    auto *swentry = next_entry();
    if (!swentry) {
        _state = SegmentWriterState::End;
        _entry = nullptr;
        return RET_EOF;
    }

    // Update starting offset
    swentry->offset = _pos;

    entry.clear();
    entry.set_type(swentry->type);

    _entry_size = 0;
    _state = SegmentWriterState::Entries;
    _entry = swentry;

    return RET_OK;
}

int SegmentWriter::write_entry(File &file, const Entry &entry, Writer &writer)
{
    (void) file;

    // Use entry size if specified
    auto size = entry.size();
    if (size) {
        if (*size > UINT32_MAX) {
            writer.set_error(make_error_code(SegmentError::InvalidEntrySize),
                             "Invalid entry size: %" PRIu64, *size);
            return RET_FAILED;
        }

        update_size_if_unset(static_cast<uint32_t>(*size));
    }

    return RET_OK;
}

int SegmentWriter::write_data(File &file, const void *buf, size_t buf_size,
                              size_t &bytes_written, Writer &writer)
{
    // Check for overflow
    if (buf_size > UINT32_MAX || _entry_size > UINT32_MAX - buf_size
            || _pos > UINT64_MAX - buf_size) {
        writer.set_error(make_error_code(
                SegmentError::WriteWouldOverflowInteger));
        return RET_FAILED;
    }

    if (!file_write_fully(file, buf, buf_size, bytes_written)) {
        writer.set_error(file.error(),
                         "Failed to write data: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    } else if (bytes_written != buf_size) {
        writer.set_error(file.error(),
                         "Write was truncated: %s",
                         file.error_string().c_str());
        // This is a fatal error. We must guarantee that buf_size bytes will be
        // written.
        return RET_FATAL;
    }

    _entry_size += static_cast<uint32_t>(buf_size);
    _pos += buf_size;

    return RET_OK;
}

int SegmentWriter::finish_entry(File &file, Writer &writer)
{
    // Update size with number of bytes written
    update_size_if_unset(_entry_size);

    // Finish previous entry by aligning to page
    if (_entry->align > 0) {
        auto skip = align_page_size<uint64_t>(_pos, _entry->align);
        uint64_t new_pos;

        if (!file.seek(static_cast<int64_t>(skip), SEEK_CUR, &new_pos)) {
            writer.set_error(file.error(),
                             "Failed to seek to page boundary: %s",
                             file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        _pos = new_pos;
    }

    return RET_OK;
}

}
}
