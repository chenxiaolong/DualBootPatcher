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
    : m_state(SegmentReaderState::Begin)
    , m_entries()
    , m_entry(m_entries.end())
    , m_read_start_offset()
    , m_read_end_offset()
    , m_read_cur_offset()
{
}

const std::vector<SegmentReaderEntry> & SegmentReader::entries() const
{
    return m_entries;
}

bool SegmentReader::set_entries(Reader &reader,
                                std::vector<SegmentReaderEntry> entries)
{
    if (m_state != SegmentReaderState::Begin) {
        reader.set_error(SegmentError::AddEntryInIncorrectState);
        return false;
    }

    m_entries = std::move(entries);
    m_entry = m_entries.end();

    return true;
}

bool SegmentReader::move_to_entry(File &file, Entry &entry,
                                  std::vector<SegmentReaderEntry>::iterator srentry,
                                  Reader &reader)
{
    if (srentry->offset > UINT64_MAX - srentry->size) {
        reader.set_error(SegmentError::EntryWouldOverflowOffset);
        return false;
    }

    uint64_t read_start_offset = srentry->offset;
    uint64_t read_end_offset = read_start_offset + srentry->size;
    uint64_t read_cur_offset = read_start_offset;

    if (m_read_cur_offset != srentry->offset) {
        if (!file.seek(static_cast<int64_t>(read_start_offset), SEEK_SET)) {
            if (file.is_fatal()) { reader.set_fatal(); }
            return false;
        }
    }

    entry.set_type(srentry->type);
    entry.set_size(srentry->size);

    m_state = SegmentReaderState::Entries;
    m_entry = srentry;
    m_read_start_offset = read_start_offset;
    m_read_end_offset = read_end_offset;
    m_read_cur_offset = read_cur_offset;

    return true;
}

bool SegmentReader::read_entry(File &file, Entry &entry, Reader &reader)
{
    auto srentry = m_entries.end();

    if (m_state == SegmentReaderState::Begin) {
        srentry = m_entries.begin();
    } else if (m_state == SegmentReaderState::Entries
            && m_entry != m_entries.end()) {
        srentry = m_entry + 1;
    }

    if (srentry == m_entries.end()) {
        m_state = SegmentReaderState::End;
        m_entry = srentry;
        reader.set_error(ReaderError::EndOfEntries);
        return false;
    }

    return move_to_entry(file, entry, srentry, reader);
}

bool SegmentReader::go_to_entry(File &file, Entry &entry, int entry_type,
                                Reader &reader)
{
    decltype(m_entries)::iterator srentry;

    if (entry_type == 0) {
        srentry = m_entries.begin();
    } else {
        srentry = std::find_if(
            m_entries.begin(),
            m_entries.end(),
            [&](const SegmentReaderEntry &sre) {
                return sre.type == entry_type;
            }
        );
    }

    if (srentry == m_entries.end()) {
        m_state = SegmentReaderState::End;
        m_entry = srentry;
        reader.set_error(ReaderError::EndOfEntries);
        return false;
    }

    return move_to_entry(file, entry, srentry, reader);
}

bool SegmentReader::read_data(File &file, void *buf, size_t buf_size,
                              size_t &bytes_read, Reader &reader)
{
    auto to_copy = static_cast<size_t>(std::min<uint64_t>(
            buf_size, m_read_end_offset - m_read_cur_offset));

    if (m_read_cur_offset > SIZE_MAX - to_copy) {
        reader.set_error(SegmentError::ReadWouldOverflowInteger,
                         "Current offset %" PRIu64 " with read size %"
                         MB_PRIzu " would overflow integer",
                         m_read_cur_offset, to_copy);
        return false;
    }

    auto n = file_read_fully(file, buf, to_copy);
    if (!n) {
        reader.set_error(n.error(),
                         "Failed to read data: %s",
                         n.error().message().c_str());
        if (file.is_fatal()) { reader.set_fatal(); }
        return false;
    }
    bytes_read = n.value();

    m_read_cur_offset += bytes_read;

    // Fail if we reach EOF early
    if (bytes_read == 0 && m_read_cur_offset != m_read_end_offset
            && !m_entry->can_truncate) {
        reader.set_error(SegmentError::EntryIsTruncated,
                         "Entry is truncated (expected %" PRIu64 " more bytes)",
                         m_read_end_offset - m_read_cur_offset);
        reader.set_fatal();
        return false;
    }

    return true;
}

}
}
