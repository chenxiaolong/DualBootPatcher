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

#include <optional>
#include <vector>

#include <cstdint>

#include "mbcommon/file.h"

#include "mbbootimg/entry.h"

namespace mb::bootimg
{

enum class SegmentWriterState
{
    Begin,
    Entries,
    End,
};

struct SegmentWriterEntry
{
    EntryType type;
    uint64_t offset;
    std::optional<uint32_t> size;
    uint64_t align;
};

struct SegmentWriter
{
public:
    SegmentWriter() noexcept;

    const std::vector<SegmentWriterEntry> & entries() const;
    oc::result<void> set_entries(std::vector<SegmentWriterEntry> entries);

    std::vector<SegmentWriterEntry>::const_iterator entry() const;

    void update_size_if_unset(uint32_t size);

    oc::result<Entry> get_entry(File &file);
    oc::result<void> write_entry(File &file, const Entry &entry);
    oc::result<size_t> write_data(File &file, const void *buf, size_t buf_size);
    oc::result<void> finish_entry(File &file);

private:
    SegmentWriterState m_state;

    std::vector<SegmentWriterEntry> m_entries;
    decltype(m_entries)::iterator m_entry;

    uint32_t m_entry_size;

    std::optional<uint64_t> m_pos;
};

}
