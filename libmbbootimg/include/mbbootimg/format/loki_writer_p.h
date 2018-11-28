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

#include <optional>
#include <vector>

#include <openssl/sha.h>

#include "mbbootimg/format/android_p.h"
#include "mbbootimg/format/segment_writer_p.h"
#include "mbbootimg/writer_p.h"


namespace mb::bootimg::loki
{

class LokiFormatWriter : public detail::FormatWriter
{
public:
    LokiFormatWriter() noexcept;
    virtual ~LokiFormatWriter() noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(LokiFormatWriter)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(LokiFormatWriter)

    Format type() override;

    oc::result<void> open(File &file) override;
    oc::result<void> close(File &file) override;
    oc::result<Header> get_header(File &file) override;
    oc::result<void> write_header(File &file, const Header &header) override;
    oc::result<Entry> get_entry(File &file) override;
    oc::result<void> write_entry(File &file, const Entry &entry) override;
    oc::result<size_t> write_data(File &file, const void *buf, size_t buf_size) override;
    oc::result<void> finish_entry(File &file) override;

private:
    // Header values
    android::AndroidHeader m_hdr;

    std::vector<unsigned char> m_aboot;

    SHA_CTX m_sha_ctx;

    std::optional<SegmentWriter> m_seg;
};

}
