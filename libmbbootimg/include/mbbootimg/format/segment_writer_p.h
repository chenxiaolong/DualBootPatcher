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

#include "mbbootimg/guard_p.h"

#include <vector>

#include <cstdint>

#include "mbcommon/optional.h"

#include "mbbootimg/writer.h"

namespace mb
{
namespace bootimg
{

enum class SegmentWriterState
{
    Begin,
    Entries,
    End,
};

struct SegmentWriterEntry
{
    int type;
    uint64_t offset;
    optional<uint32_t> size;
    uint64_t align;
};

struct SegmentWriter
{
public:
    SegmentWriter();

    const std::vector<SegmentWriterEntry> & entries() const;
    bool set_entries(Writer &writer, std::vector<SegmentWriterEntry> entries);

    std::vector<SegmentWriterEntry>::const_iterator entry() const;

    void update_size_if_unset(uint32_t size);

    bool get_entry(File &file, Entry &entry, Writer &writer);
    bool write_entry(File &file, const Entry &entry, Writer &writer);
    bool write_data(File &file, const void *buf, size_t buf_size,
                    size_t &bytes_written, Writer &writer);
    bool finish_entry(File &file, Writer &writer);

private:
    SegmentWriterState m_state;

    std::vector<SegmentWriterEntry> m_entries;
    decltype(m_entries)::iterator m_entry;

    uint32_t m_entry_size;

    optional<uint64_t> m_pos;
};

}
}
