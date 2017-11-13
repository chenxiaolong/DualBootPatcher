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
#include "mbbootimg/format/segment_reader_p.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb
{
namespace bootimg
{
namespace android
{

class AndroidFormatReader : public detail::FormatReader
{
public:
    AndroidFormatReader(Reader &reader, bool is_bump);
    virtual ~AndroidFormatReader();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(AndroidFormatReader)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(AndroidFormatReader)

    virtual int type() override;
    virtual std::string name() override;

    virtual int set_option(const char *key, const char *value) override;
    virtual int bid(File &file, int best_bid) override;
    virtual int read_header(File &file, Header &header) override;
    virtual int read_entry(File &file, Entry &entry) override;
    virtual int go_to_entry(File &file, Entry &entry, int entry_type) override;
    virtual int read_data(File &file, void *buf, size_t buf_size,
                          size_t &bytes_read) override;

    static int find_header(Reader &reader, File &file,
                           uint64_t max_header_offset,
                           AndroidHeader &header_out, uint64_t &offset_out);
    static int find_samsung_seandroid_magic(Reader &reader, File &file,
                                            const AndroidHeader &hdr,
                                            uint64_t &offset_out);
    static int find_bump_magic(Reader &reader, File &file,
                               const AndroidHeader &hdr, uint64_t &offset_out);
    static int convert_header(const AndroidHeader &hdr, Header &header);

private:
    int bid_android(File &file, int best_bid);
    int bid_bump(File &file, int best_bid);

    // Header values
    AndroidHeader _hdr;

    // Offsets
    optional<uint64_t> _header_offset;
    optional<uint64_t> _samsung_offset;
    optional<uint64_t> _bump_offset;

    bool _allow_truncated_dt;

    bool _is_bump;

    SegmentReader _seg;
};

}
}
}
