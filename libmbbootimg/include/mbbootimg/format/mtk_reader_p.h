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
#include "mbbootimg/format/mtk_p.h"
#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/reader.h"


MB_BEGIN_C_DECLS

struct MtkReaderCtx
{
    // Header values
    struct AndroidHeader hdr;
    struct MtkHeader mtk_kernel_hdr;
    struct MtkHeader mtk_ramdisk_hdr;

    // Offsets
    bool have_header_offset;
    uint64_t header_offset;
    bool have_mtkhdr_offsets;
    uint64_t mtk_kernel_offset;
    uint64_t mtk_ramdisk_offset;

    struct SegmentReaderCtx segctx;
};

int mtk_reader_bid(struct MbBiReader *bir, void *userdata, int best_bid);
int mtk_reader_set_option(struct MbBiReader *bir, void *userdata,
                          const char *key, const char *value);
int mtk_reader_read_header(struct MbBiReader *bir, void *userdata,
                           struct MbBiHeader *header);
int mtk_reader_read_entry(struct MbBiReader *bir, void *userdata,
                          struct MbBiEntry *entry);
int mtk_reader_go_to_entry(struct MbBiReader *bir, void *userdata,
                           struct MbBiEntry *entry, int entry_type);
int mtk_reader_read_data(struct MbBiReader *bir, void *userdata,
                         void *buf, size_t buf_size,
                         size_t *bytes_read);
int mtk_reader_free(struct MbBiReader *bir, void *userdata);

MB_END_C_DECLS
