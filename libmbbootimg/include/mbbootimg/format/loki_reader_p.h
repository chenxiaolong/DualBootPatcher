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
#include "mbbootimg/format/loki_p.h"
#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/reader.h"


namespace mb
{
namespace bootimg
{
namespace loki
{

struct LokiReaderCtx
{
    // Header values
    android::AndroidHeader hdr;
    LokiHeader loki_hdr;

    // Offsets
    bool have_header_offset;
    uint64_t header_offset;
    bool have_loki_offset;
    uint64_t loki_offset;

    SegmentReader seg;
};

int find_loki_header(MbBiReader *bir, mb::File &file,
                     LokiHeader &header_out, uint64_t &offset_out);
int loki_find_ramdisk_address(MbBiReader *bir, mb::File &file,
                              const android::AndroidHeader &hdr,
                              const LokiHeader &loki_hdr,
                              uint32_t &ramdisk_addr_out);
int loki_old_find_gzip_offset(MbBiReader *bir, mb::File &file,
                              uint32_t start_offset, uint64_t &gzip_offset_out);
int loki_old_find_ramdisk_size(MbBiReader *bir, mb::File &file,
                               const android::AndroidHeader &hdr,
                               uint32_t ramdisk_offset,
                               uint32_t &ramdisk_size_out);
int find_linux_kernel_size(MbBiReader *bir, mb::File &file,
                           uint32_t kernel_offset, uint32_t &kernel_size_out);
int loki_read_old_header(MbBiReader *bir, mb::File &file,
                         const android::AndroidHeader &hdr,
                         const LokiHeader &loki_hdr,
                         MbBiHeader *header,
                         uint64_t &kernel_offset_out,
                         uint32_t &kernel_size_out,
                         uint64_t &ramdisk_offset_out,
                         uint32_t &ramdisk_size_out);
int loki_read_new_header(MbBiReader *bir, mb::File &file,
                         const android::AndroidHeader &hdr,
                         const LokiHeader &loki_hdr,
                         MbBiHeader *header,
                         uint64_t &kernel_offset_out,
                         uint32_t &kernel_size_out,
                         uint64_t &ramdisk_offset_out,
                         uint32_t &ramdisk_size_out,
                         uint64_t &dt_offset_out);

int loki_reader_bid(MbBiReader *bir, void *userdata, int best_bid);
int loki_reader_read_header(MbBiReader *bir, void *userdata,
                            MbBiHeader *header);
int loki_reader_read_entry(MbBiReader *bir, void *userdata,
                           MbBiEntry *entry);
int loki_reader_go_to_entry(MbBiReader *bir, void *userdata,
                            MbBiEntry *entry, int entry_type);
int loki_reader_read_data(MbBiReader *bir, void *userdata,
                          void *buf, size_t buf_size,
                          size_t &bytes_read);
int loki_reader_free(MbBiReader *bir, void *userdata);

}
}
}
