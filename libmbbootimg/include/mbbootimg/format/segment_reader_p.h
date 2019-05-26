/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <optional>
#include <vector>

#include <cstdint>

#include "mbcommon/file.h"

#include "mbbootimg/entry.h"

namespace mb::bootimg
{

enum class SegmentReaderState
{
    Begin,
    Entries,
    End,
};

struct SegmentReaderEntry
{
    EntryType type;
    uint64_t offset;
    uint32_t size;
    bool can_truncate;
};

class SegmentReader
{
public:
    SegmentReader() noexcept;

    const std::vector<SegmentReaderEntry> & entries() const;
    oc::result<void> set_entries(std::vector<SegmentReaderEntry> entries);

    oc::result<Entry> move_to_entry(File &file,
                                    std::vector<SegmentReaderEntry>::iterator srentry);

    oc::result<Entry> read_entry(File &file);
    oc::result<Entry> go_to_entry(File &file, std::optional<EntryType> entry_type);
    oc::result<size_t> read_data(File &file, void *buf, size_t buf_size);

private:
    SegmentReaderState m_state;

    std::vector<SegmentReaderEntry> m_entries;
    decltype(m_entries)::iterator m_entry;

    uint64_t m_read_start_offset;
    uint64_t m_read_end_offset;
    uint64_t m_read_cur_offset;
};

}
