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

#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/format/sony_elf_p.h"
#include "mbbootimg/reader.h"


MB_BEGIN_C_DECLS

struct SonyElfReaderCtx
{
    // Header values
    struct Sony_Elf32_Ehdr hdr;

    bool have_header;

    struct SegmentReaderCtx segctx;
};

int find_sony_elf_header(struct MbBiReader *bir, struct MbFile *file,
                         struct Sony_Elf32_Ehdr *header_out);

int sony_elf_reader_bid(struct MbBiReader *bir, void *userdata, int best_bid);
int sony_elf_reader_read_header(struct MbBiReader *bir, void *userdata,
                                struct MbBiHeader *header);
int sony_elf_reader_read_entry(struct MbBiReader *bir, void *userdata,
                               struct MbBiEntry *entry);
int sony_elf_reader_go_to_entry(struct MbBiReader *bir, void *userdata,
                                struct MbBiEntry *entry, int entry_type);
int sony_elf_reader_read_data(struct MbBiReader *bir, void *userdata,
                              void *buf, size_t buf_size,
                              size_t *bytes_read);
int sony_elf_reader_free(struct MbBiReader *bir, void *userdata);

MB_END_C_DECLS
