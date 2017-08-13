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


namespace mb
{
namespace bootimg
{
namespace sonyelf
{

struct SonyElfReaderCtx
{
    // Header values
    Sony_Elf32_Ehdr hdr;

    bool have_header;

    SegmentReader seg;
};

int find_sony_elf_header(MbBiReader *bir, File &file,
                         Sony_Elf32_Ehdr &header_out);

int sony_elf_reader_bid(MbBiReader *bir, void *userdata, int best_bid);
int sony_elf_reader_read_header(MbBiReader *bir, void *userdata,
                                Header &header);
int sony_elf_reader_read_entry(MbBiReader *bir, void *userdata,
                               Entry &entry);
int sony_elf_reader_go_to_entry(MbBiReader *bir, void *userdata,
                                Entry &entry, int entry_type);
int sony_elf_reader_read_data(MbBiReader *bir, void *userdata,
                              void *buf, size_t buf_size,
                              size_t &bytes_read);
int sony_elf_reader_free(MbBiReader *bir, void *userdata);

}
}
}
