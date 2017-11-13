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

class AndroidFormatWriter : public detail::FormatWriter
{
public:
    AndroidFormatWriter(Writer &writer, bool is_bump);
    virtual ~AndroidFormatWriter();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(AndroidFormatWriter)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(AndroidFormatWriter)

    virtual int type() override;
    virtual std::string name() override;

    virtual int init() override;
    virtual int get_header(File &file, Header &header) override;
    virtual int write_header(File &file, const Header &header) override;
    virtual int get_entry(File &file, Entry &entry) override;
    virtual int write_entry(File &file, const Entry &entry) override;
    virtual int write_data(File &file, const void *buf, size_t buf_size,
                           size_t &bytes_written) override;
    virtual int finish_entry(File &file) override;
    virtual int close(File &file) override;

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
