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

#include "mbbootimg/writer.h"

namespace mb
{
namespace bootimg
{

constexpr size_t SEGMENT_WRITER_MAX_ENTRIES = 10;

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
    uint32_t size;
    bool size_set;
    uint64_t align;
};

struct SegmentWriter
{
public:
    SegmentWriter();

    size_t entries_size() const;
    void entries_clear();
    int entries_add(int type, uint32_t size, bool size_set,
                    uint64_t align, MbBiWriter *biw);
    const SegmentWriterEntry * entries_get(size_t index);

    const SegmentWriterEntry * entry() const;
    SegmentWriterEntry * next_entry();
    const SegmentWriterEntry * next_entry() const;

    void update_size_if_unset(uint32_t size);

    int get_entry(mb::File &file, MbBiEntry *entry, MbBiWriter *biw);
    int write_entry(mb::File &file, MbBiEntry *entry, MbBiWriter *biw);
    int write_data(mb::File &file, const void *buf, size_t buf_size,
                   size_t &bytes_written, MbBiWriter *biw);
    int finish_entry(mb::File &file, MbBiWriter *biw);

private:
    SegmentWriterState _state;

    SegmentWriterEntry _entries[SEGMENT_WRITER_MAX_ENTRIES];
    size_t _entries_len;
    SegmentWriterEntry *_entry;

    uint32_t _entry_size;

    bool _have_pos;
    uint64_t _pos;
};

}
}
