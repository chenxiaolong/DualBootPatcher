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

#include "mbbootimg/format/android_p.h"
#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/reader.h"


MB_BEGIN_C_DECLS

struct AndroidReaderCtx
{
    // Header values
    struct AndroidHeader hdr;

    // Offsets
    bool have_header_offset;
    uint64_t header_offset;
    bool have_samsung_offset;
    uint64_t samsung_offset;
    bool have_bump_offset;
    uint64_t bump_offset;

    bool allow_truncated_dt;

    bool is_bump;

    struct SegmentReaderCtx segctx;
};

int find_android_header(struct MbBiReader *bir, struct MbFile *file,
                        uint64_t max_header_offset,
                        struct AndroidHeader *header_out, uint64_t *offset_out);
int find_samsung_seandroid_magic(struct MbBiReader *bir, struct MbFile *file,
                                 struct AndroidHeader *hdr,
                                 uint64_t *offset_out);
int find_bump_magic(struct MbBiReader *bir, struct MbFile *file,
                    struct AndroidHeader *hdr, uint64_t *offset_out);
int android_set_header(struct AndroidHeader *hdr, struct MbBiHeader *header);

int android_reader_bid(struct MbBiReader *bir, void *userdata, int best_bid);
int bump_reader_bid(struct MbBiReader *bir, void *userdata, int best_bid);
int android_reader_set_option(struct MbBiReader *bir, void *userdata,
                              const char *key, const char *value);
int android_reader_read_header(struct MbBiReader *bir, void *userdata,
                               struct MbBiHeader *header);
int android_reader_read_entry(struct MbBiReader *bir, void *userdata,
                              struct MbBiEntry *entry);
int android_reader_go_to_entry(struct MbBiReader *bir, void *userdata,
                               struct MbBiEntry *entry, int entry_type);
int android_reader_read_data(struct MbBiReader *bir, void *userdata,
                             void *buf, size_t buf_size,
                             size_t *bytes_read);
int android_reader_free(struct MbBiReader *bir, void *userdata);

MB_END_C_DECLS
