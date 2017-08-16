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

#include <openssl/sha.h>

#include "mbcommon/optional.h"

#include "mbbootimg/format/android_p.h"
#include "mbbootimg/format/segment_writer_p.h"
#include "mbbootimg/writer.h"
#include "mbbootimg/writer_p.h"


namespace mb
{
namespace bootimg
{
namespace android
{

class AndroidFormatWriter : public FormatWriter
{
public:
    AndroidFormatWriter(MbBiWriter *biw, bool is_bump);
    virtual ~AndroidFormatWriter();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(AndroidFormatWriter)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(AndroidFormatWriter)

    virtual int type();
    virtual std::string name();

    virtual int init();
    virtual int get_header(Header &header);
    virtual int write_header(const Header &header);
    virtual int get_entry(Entry &entry);
    virtual int write_entry(const Entry &entry);
    virtual int write_data(const void *buf, size_t buf_size,
                           size_t &bytes_written);
    virtual int finish_entry();
    virtual int close();

private:
    // Header values
    AndroidHeader _hdr;

    optional<uint64_t> _file_size;

    bool _is_bump;

    SHA_CTX _sha_ctx;

    SegmentWriter _seg;
};

}
}
}
