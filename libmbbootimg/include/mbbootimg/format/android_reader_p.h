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
#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg::android
{

class AndroidFormatReader : public detail::FormatReader
{
public:
    AndroidFormatReader(bool is_bump) noexcept;
    virtual ~AndroidFormatReader() noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(AndroidFormatReader)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(AndroidFormatReader)

    Format type() override;

    oc::result<void> set_option(const char *key, const char *value) override;
    oc::result<int> open(File &file, int best_bid) override;
    oc::result<void> close(File &file) override;
    oc::result<Header> read_header(File &file) override;
    oc::result<Entry> read_entry(File &file) override;
    oc::result<Entry> go_to_entry(File &file,
                                  std::optional<EntryType> entry_type) override;
    oc::result<size_t> read_data(File &file, void *buf, size_t buf_size) override;

    static oc::result<std::pair<AndroidHeader, uint64_t>>
    find_header(File &file, uint64_t max_header_offset);
    static oc::result<uint64_t>
    find_samsung_seandroid_magic(File &file, const AndroidHeader &ahdr);
    static oc::result<uint64_t>
    find_bump_magic(File &file, const AndroidHeader &ahdr);
    static Header convert_header(const AndroidHeader &hdr);

private:
    oc::result<int> open_android(File &file, int best_bid);
    oc::result<int> open_bump(File &file, int best_bid);

    const bool m_is_bump;

    // Header values
    AndroidHeader m_hdr;

    // Offsets
    std::optional<uint64_t> m_hdr_offset;

    bool m_allow_truncated_dt;

    std::optional<SegmentReader> m_seg;
};

}
