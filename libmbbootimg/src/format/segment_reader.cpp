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

#include "mbbootimg/format/segment_reader_p.h"

#include <algorithm>

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/segment_error_p.h"

namespace mb
{
namespace bootimg
{

SegmentReader::SegmentReader()
    : _state(SegmentReaderState::Begin)
    , _entries()
    , _entries_len()
    , _entry()
    , _read_start_offset()
    , _read_end_offset()
    , _read_cur_offset()
{
}

size_t SegmentReader::entries_size() const
{
    return _entries_len;
}

void SegmentReader::entries_clear()
{
    _entries_len = 0;
    _entry = nullptr;
}

int SegmentReader::entries_add(int type, uint64_t offset, uint32_t size,
                               bool can_truncate, Reader &reader)
{
    if (_state != SegmentReaderState::Begin) {
        reader.set_error(make_error_code(
                SegmentError::AddEntryInIncorrectState));
        return RET_FATAL;
    }

    if (_entries_len == sizeof(_entries) / sizeof(_entries[0])) {
        reader.set_error(make_error_code(SegmentError::TooManyEntries));
        return RET_FATAL;
    }

    SegmentReaderEntry *entry = &_entries[_entries_len];
    entry->type = type;
    entry->offset = offset;
    entry->size = size;
    entry->can_truncate = can_truncate;

    ++_entries_len;

    return RET_OK;
}

const SegmentReaderEntry * SegmentReader::entry() const
{
    switch (_state) {
    case SegmentReaderState::Entries:
        return _entry;
    default:
        return nullptr;
    }
}

const SegmentReaderEntry * SegmentReader::next_entry() const
{
    switch (_state) {
    case SegmentReaderState::Begin:
        if (_entries_len > 0) {
            return _entries;
        } else {
            return nullptr;
        }
    case SegmentReaderState::Entries:
        if (static_cast<size_t>(_entry - _entries + 1) == _entries_len) {
            return nullptr;
        } else {
            return _entry + 1;
        }
    default:
        return nullptr;
    }
}

const SegmentReaderEntry * SegmentReader::find_entry(int entry_type)
{
    if (entry_type == 0) {
        if (_entries_len > 0) {
            return _entries;
        }
    } else {
        for (size_t i = 0; i < _entries_len; ++i) {
            if (_entries[i].type == entry_type) {
                return &_entries[i];
            }
        }
    }

    return nullptr;
}

int SegmentReader::move_to_entry(File &file, Entry &entry,
                                 const SegmentReaderEntry &srentry,
                                 Reader &reader)
{
    if (srentry.offset > UINT64_MAX - srentry.size) {
        reader.set_error(make_error_code(
                SegmentError::EntryWouldOverflowOffset));
        return RET_FAILED;
    }

    uint64_t read_start_offset = srentry.offset;
    uint64_t read_end_offset = read_start_offset + srentry.size;
    uint64_t read_cur_offset = read_start_offset;

    if (_read_cur_offset != srentry.offset) {
        if (!file.seek(static_cast<int64_t>(read_start_offset), SEEK_SET,
                       nullptr)) {
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }
    }

    entry.set_type(srentry.type);
    entry.set_size(srentry.size);

    _state = SegmentReaderState::Entries;
    _entry = &srentry;
    _read_start_offset = read_start_offset;
    _read_end_offset = read_end_offset;
    _read_cur_offset = read_cur_offset;

    return RET_OK;
}

int SegmentReader::read_entry(File &file, Entry &entry, Reader &reader)
{
    auto const *srentry = next_entry();
    if (!srentry) {
        _state = SegmentReaderState::End;
        _entry = nullptr;
        return RET_EOF;
    }

    return move_to_entry(file, entry, *srentry, reader);
}

int SegmentReader::go_to_entry(File &file, Entry &entry, int entry_type,
                               Reader &reader)
{
    auto const *srentry = find_entry(entry_type);
    if (!srentry) {
        _state = SegmentReaderState::End;
        _entry = nullptr;
        return RET_EOF;
    }

    return move_to_entry(file, entry, *srentry, reader);
}

int SegmentReader::read_data(File &file, void *buf, size_t buf_size,
                             size_t &bytes_read, Reader &reader)
{
    auto to_copy = static_cast<size_t>(std::min<uint64_t>(
            buf_size, _read_end_offset - _read_cur_offset));

    if (_read_cur_offset > SIZE_MAX - to_copy) {
        reader.set_error(make_error_code(SegmentError::ReadWouldOverflowInteger),
                         "Current offset %" PRIu64 " with read size %"
                         MB_PRIzu " would overflow integer",
                         _read_cur_offset, to_copy);
        return RET_FAILED;
    }

    if (!file_read_fully(file, buf, to_copy, bytes_read)) {
        reader.set_error(file.error(),
                         "Failed to read data: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    _read_cur_offset += bytes_read;

    // Fail if we reach EOF early
    if (bytes_read == 0 && _read_cur_offset != _read_end_offset
            && !_entry->can_truncate) {
        reader.set_error(make_error_code(SegmentError::EntryIsTruncated),
                         "Entry is truncated (expected %" PRIu64 " more bytes)",
                         _read_end_offset - _read_cur_offset);
        return RET_FATAL;
    }

    return bytes_read == 0 ? RET_EOF : RET_OK;
}

}
}
