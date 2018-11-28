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

#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/format/sony_elf_p.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg::sonyelf
{

class SonyElfFormatReader : public detail::FormatReader
{
public:
    SonyElfFormatReader() noexcept;
    virtual ~SonyElfFormatReader() noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(SonyElfFormatReader)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(SonyElfFormatReader)

    Format type() override;

    oc::result<int> open(File &file, int best_bid) override;
    oc::result<void> close(File &file) override;
    oc::result<Header> read_header(File &file) override;
    oc::result<Entry> read_entry(File &file) override;
    oc::result<Entry> go_to_entry(File &file,
                                  std::optional<EntryType> entry_type) override;
    oc::result<size_t> read_data(File &file, void *buf, size_t buf_size) override;

    static oc::result<Sony_Elf32_Ehdr>
    find_sony_elf_header(File &file);

private:
    // Header values
    Sony_Elf32_Ehdr m_hdr;

    std::optional<SegmentReader> m_seg;
};

}
