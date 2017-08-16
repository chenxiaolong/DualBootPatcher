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

class MtkFormatReader : public FormatReader
{
public:
    MtkFormatReader(MbBiReader *bir);
    virtual ~MtkFormatReader();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(MtkFormatReader)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(MtkFormatReader)

    virtual int type();
    virtual std::string name();

    virtual int bid(int best_bid);
    virtual int read_header(Header &header);
    virtual int read_entry(Entry &entry);
    virtual int go_to_entry(Entry &entry, int entry_type);
    virtual int read_data(void *buf, size_t buf_size, size_t &bytes_read);

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
