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
#include "mbbootimg/format/mtk_p.h"
#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg::mtk
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

    oc::result<int> open(File &file, int best_bid) override;
    oc::result<void> close(File &file) override;
    oc::result<void> read_header(File &file, Header &header) override;
    oc::result<void> read_entry(File &file, Entry &entry) override;
    oc::result<void> go_to_entry(File &file, Entry &entry, int entry_type) override;
    oc::result<size_t> read_data(File &file, void *buf, size_t buf_size) override;

private:
    // Header values
    android::AndroidHeader m_hdr;
    MtkHeader m_mtk_kernel_hdr;
    MtkHeader m_mtk_ramdisk_hdr;

    // Offsets
    std::optional<uint64_t> m_header_offset;
    std::optional<uint64_t> m_mtk_kernel_offset;
    std::optional<uint64_t> m_mtk_ramdisk_offset;

    std::optional<SegmentReader> m_seg;
};

}
