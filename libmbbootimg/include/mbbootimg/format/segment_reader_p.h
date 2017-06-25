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

#ifdef __cplusplus
#  include <cstdint>
#else
#  include <stdint.h>
#endif

#include "mbbootimg/reader.h"

#define SEGMENT_READER_MAX_ENTRIES      10

enum
#ifdef __cplusplus
class
#endif
SegmentReaderState
{
    BEGIN,
    ENTRIES,
    END,
};

struct SegmentReaderEntry
{
    int type;
    uint64_t offset;
    uint32_t size;
    bool can_truncate;
};

struct SegmentReaderCtx
{
    enum SegmentReaderState state;

    struct SegmentReaderEntry entries[SEGMENT_READER_MAX_ENTRIES];
    size_t entries_len;
    struct SegmentReaderEntry *entry;

    uint64_t read_start_offset;
    uint64_t read_end_offset;
    uint64_t read_cur_offset;
};

int _segment_reader_init(struct SegmentReaderCtx *ctx);
int _segment_reader_deinit(struct SegmentReaderCtx *ctx);

size_t _segment_reader_entries_size(struct SegmentReaderCtx *ctx);
void _segment_reader_entries_clear(struct SegmentReaderCtx *ctx);
int _segment_reader_entries_add(struct SegmentReaderCtx *ctx,
                                int type, uint64_t offset, uint32_t size,
                                bool can_truncate, struct MbBiReader *bir);

struct SegmentReaderEntry * _segment_reader_entry(struct SegmentReaderCtx *ctx);
struct SegmentReaderEntry * _segment_reader_next_entry(struct SegmentReaderCtx *ctx);
struct SegmentReaderEntry * _segment_reader_find_entry(struct SegmentReaderCtx *ctx,
                                                       int entry_type);

int _segment_reader_move_to_entry(struct SegmentReaderCtx *ctx, struct MbFile *file,
                                  struct MbBiEntry *entry,
                                  struct SegmentReaderEntry *srentry,
                                  struct MbBiReader *bir);

int _segment_reader_read_entry(struct SegmentReaderCtx *ctx, struct MbFile *file,
                               struct MbBiEntry *entry, struct MbBiReader *bir);
int _segment_reader_go_to_entry(struct SegmentReaderCtx *ctx, struct MbFile *file,
                                struct MbBiEntry *entry, int entry_type,
                                struct MbBiReader *bir);
int _segment_reader_read_data(struct SegmentReaderCtx *ctx, struct MbFile *file,
                              void *buf, size_t buf_size, size_t *bytes_read,
                              struct MbBiReader *bir);
