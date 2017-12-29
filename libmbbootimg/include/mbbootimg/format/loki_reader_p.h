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

#include <optional>

#include "mbbootimg/format/android_p.h"
#include "mbbootimg/format/loki_p.h"
#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg::loki
{

class LokiFormatReader : public detail::FormatReader
{
public:
    LokiFormatReader(Reader &reader);
    virtual ~LokiFormatReader();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(LokiFormatReader)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(LokiFormatReader)

    int type() override;
    std::string name() override;

    oc::result<int> open(File &file, int best_bid) override;
    oc::result<void> close(File &file) override;
    oc::result<void> read_header(File &file, Header &header) override;
    oc::result<void> read_entry(File &file, Entry &entry) override;
    oc::result<void> go_to_entry(File &file, Entry &entry, int entry_type) override;
    oc::result<size_t> read_data(File &file, void *buf, size_t buf_size) override;

    static oc::result<void>
    find_loki_header(Reader &reader, File &file,
                     LokiHeader &header_out,
                     uint64_t &offset_out);
    static oc::result<void>
    find_ramdisk_address(Reader &reader, File &file,
                         const android::AndroidHeader &hdr,
                         const LokiHeader &loki_hdr,
                         uint32_t &ramdisk_addr_out);
    static oc::result<void>
    find_gzip_offset_old(Reader &reader, File &file,
                         uint32_t start_offset,
                         uint64_t &gzip_offset_out);
    static oc::result<void>
    find_ramdisk_size_old(Reader &reader, File &file,
                          const android::AndroidHeader &hdr,
                          uint32_t ramdisk_offset,
                          uint32_t &ramdisk_size_out);
    static oc::result<void>
    find_linux_kernel_size(Reader &reader, File &file,
                           uint32_t kernel_offset,
                           uint32_t &kernel_size_out);
    static oc::result<void>
    read_header_old(Reader &reader, File &file,
                    const android::AndroidHeader &hdr,
                    const LokiHeader &loki_hdr,
                    Header &header,
                    uint64_t &kernel_offset_out,
                    uint32_t &kernel_size_out,
                    uint64_t &ramdisk_offset_out,
                    uint32_t &ramdisk_size_out);
    static oc::result<void>
    read_header_new(Reader &reader, File &file,
                    const android::AndroidHeader &hdr,
                    const LokiHeader &loki_hdr,
                    Header &header,
                    uint64_t &kernel_offset_out,
                    uint32_t &kernel_size_out,
                    uint64_t &ramdisk_offset_out,
                    uint32_t &ramdisk_size_out,
                    uint64_t &dt_offset_out);

private:
    // Header values
    android::AndroidHeader m_hdr;
    LokiHeader m_loki_hdr;

    // Offsets
    std::optional<uint64_t> m_header_offset;
    std::optional<uint64_t> m_loki_offset;

    std::optional<SegmentReader> m_seg;
};

}
