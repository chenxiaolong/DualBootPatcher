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

#include <cstdint>

#include "mbbootimg/reader.h"

namespace mb
{
namespace bootimg
{

constexpr size_t SEGMENT_READER_MAX_ENTRIES = 10;

enum class SegmentReaderState
{
    Begin,
    Entries,
    End,
};

struct SegmentReaderEntry
{
    int type;
    uint64_t offset;
    uint32_t size;
    bool can_truncate;
};

class SegmentReader
{
public:
    SegmentReader();

    size_t entries_size() const;
    void entries_clear();
    int entries_add(int type, uint64_t offset, uint32_t size,
                    bool can_truncate, MbBiReader *bir);

    const SegmentReaderEntry * entry() const;
    const SegmentReaderEntry * next_entry() const;
    const SegmentReaderEntry * find_entry(int entry_type);

    int move_to_entry(mb::File &file, MbBiEntry *entry,
                      const SegmentReaderEntry &srentry,
                      MbBiReader *bir);

    int read_entry(mb::File &file, MbBiEntry *entry, MbBiReader *bir);
    int go_to_entry(mb::File &file, MbBiEntry *entry, int entry_type,
                    MbBiReader *bir);
    int read_data(mb::File &file, void *buf, size_t buf_size,
                  size_t &bytes_read, MbBiReader *bir);

private:
    SegmentReaderState _state;

    SegmentReaderEntry _entries[SEGMENT_READER_MAX_ENTRIES];
    size_t _entries_len;
    const SegmentReaderEntry *_entry;

    uint64_t _read_start_offset;
    uint64_t _read_end_offset;
    uint64_t _read_cur_offset;
};

}
}
