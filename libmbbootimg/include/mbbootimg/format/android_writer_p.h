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

    int type() override;
    std::string name() override;

    oc::result<void> open(File &file) override;
    oc::result<void> close(File &file) override;
    oc::result<void> get_header(File &file, Header &header) override;
    oc::result<void> write_header(File &file, const Header &header) override;
    oc::result<void> get_entry(File &file, Entry &entry) override;
    oc::result<void> write_entry(File &file, const Entry &entry) override;
    oc::result<size_t> write_data(File &file, const void *buf, size_t buf_size) override;
    oc::result<void> finish_entry(File &file) override;

private:
    const bool m_is_bump;

    // Header values
    AndroidHeader m_hdr;

    SHA_CTX m_sha_ctx;

    optional<SegmentWriter> m_seg;
};

}
}
}
