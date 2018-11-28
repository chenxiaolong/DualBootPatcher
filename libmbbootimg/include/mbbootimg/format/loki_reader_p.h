/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include "mbbootimg/reader_p.h"


namespace mb::bootimg::loki
{

struct ReadHeaderResult
{
    Header header;
    uint64_t kernel_offset;
    uint64_t ramdisk_offset;
    uint64_t dt_offset;
    uint32_t kernel_size;
    uint32_t ramdisk_size;
};

class LokiFormatReader : public detail::FormatReader
{
public:
    LokiFormatReader() noexcept;
    virtual ~LokiFormatReader() noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(LokiFormatReader)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(LokiFormatReader)

    Format type() override;

    oc::result<int> open(File &file, int best_bid) override;
    oc::result<void> close(File &file) override;
    oc::result<Header> read_header(File &file) override;
    oc::result<Entry> read_entry(File &file) override;
    oc::result<Entry> go_to_entry(File &file,
                                  std::optional<EntryType> entry_type) override;
    oc::result<size_t> read_data(File &file, void *buf, size_t buf_size) override;

    static oc::result<std::pair<LokiHeader, uint64_t>>
    find_loki_header(File &file);
    static oc::result<uint32_t>
    find_ramdisk_address(File &file,
                         const android::AndroidHeader &ahdr,
                         const LokiHeader &lhdr);
    static oc::result<uint64_t>
    find_gzip_offset_old(File &file,
                         uint32_t start_offset);
    static oc::result<uint32_t>
    find_ramdisk_size_old(File &file,
                          const android::AndroidHeader &ahdr,
                          uint32_t ramdisk_offset);
    static oc::result<uint32_t>
    find_linux_kernel_size(File &file,
                           uint32_t kernel_offset);
    static oc::result<ReadHeaderResult>
    read_header_old(File &file,
                    const android::AndroidHeader &ahdr,
                    const LokiHeader &lhdr);
    static oc::result<ReadHeaderResult>
    read_header_new(File &file,
                    const android::AndroidHeader &ahdr,
                    const LokiHeader &lhdr);

private:
    // Header values
    android::AndroidHeader m_ahdr;
    LokiHeader m_lhdr;

    // Offsets
    std::optional<uint64_t> m_ahdr_offset;
    std::optional<uint64_t> m_lhdr_offset;

    std::optional<SegmentReader> m_seg;
};

}
