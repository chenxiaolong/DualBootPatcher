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

namespace mb::bootimg
{

SegmentWriter::SegmentWriter()
    : m_state(SegmentWriterState::Begin)
    , m_entries()
    , m_entry()
    , m_entry_size()
    , m_pos()
{
}

const std::vector<SegmentWriterEntry> & SegmentWriter::entries() const
{
    return m_entries;
}

oc::result<void> SegmentWriter::set_entries(std::vector<SegmentWriterEntry> entries)
{
    if (m_state != SegmentWriterState::Begin) {
        return SegmentError::AddEntryInIncorrectState;
    }

    m_entries = std::move(entries);
    m_entry = m_entries.end();

    return oc::success();
}

std::vector<SegmentWriterEntry>::const_iterator SegmentWriter::entry() const
{
    return m_entry;
}

void SegmentWriter::update_size_if_unset(uint32_t size)
{
    if (!m_entry->size) {
        m_entry->size = size;
    }
}

oc::result<void> SegmentWriter::get_entry(File &file, Entry &entry,
                                          Writer &writer)
{
    if (!m_pos) {
        OUTCOME_TRY(pos, file.seek(0, SEEK_CUR));

        m_pos = pos;
    }

    // Get next entry
    auto swentry = m_entries.end();

    if (m_state == SegmentWriterState::Begin) {
        swentry = m_entries.begin();
    } else if (m_state == SegmentWriterState::Entries
            && m_entry != m_entries.end()) {
        swentry = m_entry + 1;
    }

    if (swentry == m_entries.end()) {
        m_state = SegmentWriterState::End;
        m_entry = swentry;
        return WriterError::EndOfEntries;
    }

    // Update starting offset
    swentry->offset = *m_pos;

    entry.clear();
    entry.set_type(swentry->type);

    m_entry_size = 0;
    m_state = SegmentWriterState::Entries;
    m_entry = swentry;

    return oc::success();
}

oc::result<void> SegmentWriter::write_entry(File &file, const Entry &entry,
                                            Writer &writer)
{
    (void) file;
    (void) writer;

    // Use entry size if specified
    auto size = entry.size();
    if (size) {
        if (*size > UINT32_MAX) {
            //DEBUG("Invalid entry size: %" PRIu64, *size);
            return SegmentError::InvalidEntrySize;
        }

        update_size_if_unset(static_cast<uint32_t>(*size));
    }

    return oc::success();
}

oc::result<size_t> SegmentWriter::write_data(File &file, const void *buf,
                                             size_t buf_size, Writer &writer)
{
    // Check for overflow
    if (buf_size > UINT32_MAX || m_entry_size > UINT32_MAX - buf_size
            || *m_pos > UINT64_MAX - buf_size) {
        return SegmentError::WriteWouldOverflowInteger;
    }

    auto ret = file_write_exact(file, buf, buf_size);
    if (!ret) {
        return ret.as_failure();
    }

    m_entry_size += static_cast<uint32_t>(buf_size);
    *m_pos += buf_size;

    return buf_size;
}

oc::result<void> SegmentWriter::finish_entry(File &file, Writer &writer)
{
    // Update size with number of bytes written
    update_size_if_unset(m_entry_size);

    // Finish previous entry by aligning to page
    if (m_entry->align > 0) {
        auto skip = align_page_size<uint64_t>(*m_pos, m_entry->align);

        OUTCOME_TRY(new_pos, file.seek(static_cast<int64_t>(skip), SEEK_CUR));

        m_pos = new_pos;
    }

    return oc::success();
}

}
