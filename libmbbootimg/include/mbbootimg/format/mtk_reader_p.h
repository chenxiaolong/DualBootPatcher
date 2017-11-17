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

#include "mbcommon/optional.h"

#include "mbbootimg/format/android_p.h"
#include "mbbootimg/format/mtk_p.h"
#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb
{
namespace bootimg
{
namespace mtk
{

class MtkFormatReader : public detail::FormatReader
{
public:
    MtkFormatReader(Reader &reader);
    virtual ~MtkFormatReader();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(MtkFormatReader)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(MtkFormatReader)

    int type() override;
    std::string name() override;

    int bid(File &file, int best_bid) override;
    bool read_header(File &file, Header &header) override;
    bool read_entry(File &file, Entry &entry) override;
    bool go_to_entry(File &file, Entry &entry, int entry_type) override;
    bool read_data(File &file, void *buf, size_t buf_size,
                   size_t &bytes_read) override;

private:
    // Header values
    android::AndroidHeader _hdr;
    MtkHeader _mtk_kernel_hdr;
    MtkHeader _mtk_ramdisk_hdr;

    // Offsets
    optional<uint64_t> _header_offset;
    optional<uint64_t> _mtk_kernel_offset;
    optional<uint64_t> _mtk_ramdisk_offset;

    SegmentReader _seg;
};

}
}
}
