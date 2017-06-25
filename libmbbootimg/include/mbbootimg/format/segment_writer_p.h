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

#include "mbbootimg/writer.h"

#define SEGMENT_WRITER_MAX_ENTRIES      10

enum
#ifdef __cplusplus
class
#endif
SegmentWriterState
{
    BEGIN,
    ENTRIES,
    END,
};

struct SegmentWriterEntry
{
    int type;
    uint64_t offset;
    uint32_t size;
    bool size_set;
    uint64_t align;
};

struct SegmentWriterCtx
{
    enum SegmentWriterState state;

    struct SegmentWriterEntry entries[SEGMENT_WRITER_MAX_ENTRIES];
    size_t entries_len;
    struct SegmentWriterEntry *entry;

    uint32_t entry_size;

    bool have_pos;
    uint64_t pos;
};

int _segment_writer_init(struct SegmentWriterCtx *ctx);
int _segment_writer_deinit(struct SegmentWriterCtx *ctx);

size_t _segment_writer_entries_size(struct SegmentWriterCtx *ctx);
void _segment_writer_entries_clear(struct SegmentWriterCtx *ctx);
int _segment_writer_entries_add(struct SegmentWriterCtx *ctx,
                                int type, uint32_t size, bool size_set,
                                uint64_t align, struct MbBiWriter *biw);
struct SegmentWriterEntry * _segment_writer_entries_get(struct SegmentWriterCtx *ctx,
                                                        size_t index);

struct SegmentWriterEntry * _segment_writer_entry(struct SegmentWriterCtx *ctx);
struct SegmentWriterEntry * _segment_writer_next_entry(struct SegmentWriterCtx *ctx);

void _segment_writer_update_size_if_unset(struct SegmentWriterCtx *ctx,
                                          uint32_t size);

int _segment_writer_get_entry(struct SegmentWriterCtx *ctx, struct MbFile *file,
                              struct MbBiEntry *entry, struct MbBiWriter *biw);
int _segment_writer_write_entry(struct SegmentWriterCtx *ctx, struct MbFile *file,
                                struct MbBiEntry *entry, struct MbBiWriter *biw);
int _segment_writer_write_data(struct SegmentWriterCtx *ctx, struct MbFile *file,
                               const void *buf, size_t buf_size,
                               size_t *bytes_written, struct MbBiWriter *biw);
int _segment_writer_finish_entry(struct SegmentWriterCtx *ctx, struct MbFile *file,
                                 struct MbBiWriter *biw);
