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

int _segment_writer_init(SegmentWriterCtx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));

    ctx->state = SegmentWriterState::BEGIN;

    return MB_BI_OK;
}

int _segment_writer_deinit(SegmentWriterCtx *ctx)
{
    (void) ctx;
    return MB_BI_OK;
}

size_t _segment_writer_entries_size(SegmentWriterCtx *ctx)
{
    return ctx->entries_len;
}

void _segment_writer_entries_clear(SegmentWriterCtx *ctx)
{
    ctx->entries_len = 0;
    ctx->entry = nullptr;
}

int _segment_writer_entries_add(SegmentWriterCtx *ctx,
                                int type, uint32_t size, bool size_set,
                                uint64_t align, MbBiWriter *biw)
{
    if (ctx->state != SegmentWriterState::BEGIN) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Adding entry in incorrect state");
        return MB_BI_FATAL;
    }

    if (ctx->entries_len == sizeof(ctx->entries) / sizeof(ctx->entries[0])) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Too many entries");
        return MB_BI_FATAL;
    }

    SegmentWriterEntry *entry = &ctx->entries[ctx->entries_len];
    entry->type = type;
    entry->offset = 0;
    entry->size = size;
    entry->size_set = size_set;
    entry->align = align;

    ++ctx->entries_len;

    return MB_BI_OK;
}

SegmentWriterEntry * _segment_writer_entries_get(SegmentWriterCtx *ctx,
                                                 size_t index)
{
    if (index < ctx->entries_len) {
        return &ctx->entries[index];
    } else {
        return nullptr;
    }
}

SegmentWriterEntry * _segment_writer_entry(SegmentWriterCtx *ctx)
{
    switch (ctx->state) {
    case SegmentWriterState::ENTRIES:
        return ctx->entry;
    default:
        return nullptr;
    }
}

SegmentWriterEntry * _segment_writer_next_entry(SegmentWriterCtx *ctx)
{
    switch (ctx->state) {
    case SegmentWriterState::BEGIN:
        if (ctx->entries_len > 0) {
            return ctx->entries;
        } else {
            return nullptr;
        }
    case SegmentWriterState::ENTRIES:
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

void _segment_writer_update_size_if_unset(SegmentWriterCtx *ctx,
                                          uint32_t size)
{
    if (!ctx->entry->size_set) {
        ctx->entry->size = size;
        ctx->entry->size_set = true;
    }
}

int _segment_writer_get_entry(SegmentWriterCtx *ctx, MbFile *file,
                              MbBiEntry *entry, MbBiWriter *biw)
{
    SegmentWriterEntry *swentry;
    int ret;

    if (!ctx->have_pos) {
        ret = mb_file_seek(file, 0, SEEK_CUR, &ctx->pos);
        if (ret != MB_FILE_OK) {
            mb_bi_writer_set_error(biw, mb_file_error(file),
                                   "Failed to get current offset: %s",
                                   mb_file_error_string(file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        ctx->have_pos = true;
    }

    // Get next entry
    swentry = _segment_writer_next_entry(ctx);
    if (!swentry) {
        ctx->state = SegmentWriterState::END;
        ctx->entry = nullptr;
        return MB_BI_EOF;
    }

    // Update starting offset
    swentry->offset = ctx->pos;

    mb_bi_entry_clear(entry);

    ret = mb_bi_entry_set_type(entry, swentry->type);
    if (ret != MB_BI_OK) return ret;

    ctx->entry_size = 0;
    ctx->state = SegmentWriterState::ENTRIES;
    ctx->entry = swentry;

    return MB_BI_OK;
}

int _segment_writer_write_entry(SegmentWriterCtx *ctx, MbFile *file,
                                MbBiEntry *entry, MbBiWriter *biw)
{
    (void) file;

    // Use entry size if specified
    if (mb_bi_entry_size_is_set(entry)) {
        uint64_t size = mb_bi_entry_size(entry);

        if (size > UINT32_MAX) {
            mb_bi_writer_set_error(biw, MB_BI_ERROR_INVALID_ARGUMENT,
                                   "Invalid entry size: %" PRIu64, size);
            return MB_BI_FAILED;
        }

        _segment_writer_update_size_if_unset(ctx, size);
    }

    return MB_BI_OK;
}

int _segment_writer_write_data(SegmentWriterCtx *ctx, MbFile *file,
                               const void *buf, size_t buf_size,
                               size_t *bytes_written, MbBiWriter *biw)
{
    int ret;

    // Check for overflow
    if (buf_size > UINT32_MAX || ctx->entry_size > UINT32_MAX - buf_size
            || ctx->pos > UINT64_MAX - buf_size) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INVALID_ARGUMENT,
                               "Overflow in entry size");
        return MB_BI_FAILED;
    }

    ret = mb_file_write_fully(file, buf, buf_size, bytes_written);
    if (ret != MB_FILE_OK) {
        mb_bi_writer_set_error(biw, mb_file_error(file),
                               "Failed to write data: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    } else if (*bytes_written != buf_size) {
        mb_bi_writer_set_error(biw, mb_file_error(file),
                               "Write was truncated: %s",
                               mb_file_error_string(file));
        // This is a fatal error. We must guarantee that buf_size bytes will be
        // written.
        return MB_BI_FATAL;
    }

    ctx->entry_size += buf_size;
    ctx->pos += buf_size;

    return MB_BI_OK;
}

int _segment_writer_finish_entry(SegmentWriterCtx *ctx, MbFile *file,
                                 MbBiWriter *biw)
{
    int ret;

    // Update size with number of bytes written
    _segment_writer_update_size_if_unset(ctx, ctx->entry_size);

    // Finish previous entry by aligning to page
    if (ctx->entry->align > 0) {
        uint64_t skip = align_page_size<uint64_t>(ctx->pos, ctx->entry->align);
        uint64_t new_pos;

        ret = mb_file_seek(file, skip, SEEK_CUR, &new_pos);
        if (ret != MB_FILE_OK) {
            mb_bi_writer_set_error(biw, mb_file_error(file),
                                   "Failed to seek to page boundary: %s",
                                   mb_file_error_string(file));
            return ret == MB_BI_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        ctx->pos = new_pos;
    }

    return MB_BI_OK;
}
