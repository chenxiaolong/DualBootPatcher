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

int _segment_reader_init(SegmentReaderCtx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->state = SegmentReaderState::BEGIN;

    return MB_BI_OK;
}

int _segment_reader_deinit(SegmentReaderCtx *ctx)
{
    (void) ctx;
    return MB_BI_OK;
}

size_t _segment_reader_entries_size(SegmentReaderCtx *ctx)
{
    return ctx->entries_len;
}

void _segment_reader_entries_clear(SegmentReaderCtx *ctx)
{
    ctx->entries_len = 0;
    ctx->entry = nullptr;
}

int _segment_reader_entries_add(SegmentReaderCtx *ctx,
                                int type, uint64_t offset, uint32_t size,
                                bool can_truncate, MbBiReader *bir)
{
    if (ctx->state != SegmentReaderState::BEGIN) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "Adding entry in incorrect state");
        return MB_BI_FATAL;
    }

    if (ctx->entries_len == sizeof(ctx->entries) / sizeof(ctx->entries[0])) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "Too many entries");
        return MB_BI_FATAL;
    }

    SegmentReaderEntry *entry = &ctx->entries[ctx->entries_len];
    entry->type = type;
    entry->offset = offset;
    entry->size = size;
    entry->can_truncate = can_truncate;

    ++ctx->entries_len;

    return MB_BI_OK;
}

SegmentReaderEntry * _segment_reader_entry(SegmentReaderCtx *ctx)
{
    switch (ctx->state) {
    case SegmentReaderState::ENTRIES:
        return ctx->entry;
    default:
        return nullptr;
    }
}

SegmentReaderEntry * _segment_reader_next_entry(SegmentReaderCtx *ctx)
{
    switch (ctx->state) {
    case SegmentReaderState::BEGIN:
        if (ctx->entries_len > 0) {
            return ctx->entries;
        } else {
            return nullptr;
        }
    case SegmentReaderState::ENTRIES:
        if (static_cast<size_t>(ctx->entry - ctx->entries + 1)
                == ctx->entries_len) {
            return nullptr;
        } else {
            return ctx->entry + 1;
        }
    default:
        return nullptr;
    }
}

SegmentReaderEntry * _segment_reader_find_entry(SegmentReaderCtx *ctx,
                                                int entry_type)
{
    if (entry_type == 0) {
        if (ctx->entries_len > 0) {
            return ctx->entries;
        }
    } else {
        for (size_t i = 0; i < ctx->entries_len; ++i) {
            if (ctx->entries[i].type == entry_type) {
                return &ctx->entries[i];
            }
        }
    }

    return nullptr;
}

int _segment_reader_move_to_entry(SegmentReaderCtx *ctx, MbFile *file,
                                  MbBiEntry *entry, SegmentReaderEntry *srentry,
                                  MbBiReader *bir)
{
    int ret;

    if (srentry->offset > UINT64_MAX - srentry->size) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INVALID_ARGUMENT,
                               "Entry would overflow offset");
        return MB_BI_FAILED;
    }

    uint64_t read_start_offset = srentry->offset;
    uint64_t read_end_offset = read_start_offset + srentry->size;
    uint64_t read_cur_offset = read_start_offset;

    if (ctx->read_cur_offset != srentry->offset) {
        ret = mb_file_seek(file, read_start_offset, SEEK_SET, nullptr);
        if (ret < 0) {
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }
    }

    ret = mb_bi_entry_set_type(entry, srentry->type);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_entry_set_size(entry, srentry->size);
    if (ret != MB_BI_OK) return ret;

    ctx->state = SegmentReaderState::ENTRIES;
    ctx->entry = srentry;
    ctx->read_start_offset = read_start_offset;
    ctx->read_end_offset = read_end_offset;
    ctx->read_cur_offset = read_cur_offset;

    return MB_BI_OK;
}

int _segment_reader_read_entry(SegmentReaderCtx *ctx, MbFile *file,
                               MbBiEntry *entry, MbBiReader *bir)
{
    SegmentReaderEntry *srentry;

    srentry = _segment_reader_next_entry(ctx);
    if (!srentry) {
        ctx->state = SegmentReaderState::END;
        ctx->entry = nullptr;
        return MB_BI_EOF;
    }

    return _segment_reader_move_to_entry(ctx, file, entry, srentry, bir);
}

int _segment_reader_go_to_entry(struct SegmentReaderCtx *ctx, struct MbFile *file,
                                struct MbBiEntry *entry, int entry_type,
                                struct MbBiReader *bir)
{
    SegmentReaderEntry *srentry;

    srentry = _segment_reader_find_entry(ctx, entry_type);
    if (!srentry) {
        ctx->state = SegmentReaderState::END;
        ctx->entry = nullptr;
        return MB_BI_EOF;
    }

    return _segment_reader_move_to_entry(ctx, file, entry, srentry, bir);
}

int _segment_reader_read_data(SegmentReaderCtx *ctx, MbFile *file,
                              void *buf, size_t buf_size, size_t *bytes_read,
                              MbBiReader *bir)
{
    size_t to_copy = std::min<uint64_t>(
            buf_size, ctx->read_end_offset - ctx->read_cur_offset);

    if (ctx->read_cur_offset > SIZE_MAX - to_copy) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Current offset %" PRIu64 " with read size %"
                               MB_PRIzu " would overflow integer",
                               ctx->read_cur_offset, to_copy);
        return MB_BI_FAILED;
    }

    int ret = mb_file_read_fully(file, buf, to_copy, bytes_read);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to read data: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    ctx->read_cur_offset += *bytes_read;

    // Fail if we reach EOF early
    if (*bytes_read == 0 && ctx->read_cur_offset != ctx->read_end_offset
            && !ctx->entry->can_truncate) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Entry is truncated "
                               "(expected %" PRIu64 " more bytes)",
                               ctx->read_end_offset - ctx->read_cur_offset);
        return MB_BI_FATAL;
    }

    return *bytes_read == 0 ? MB_BI_EOF : MB_BI_OK;
}
