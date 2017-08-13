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

#include "mbbootimg/entry.h"
#include "mbbootimg/format/android_p.h"
#include "mbbootimg/format/mtk_p.h"
#include "mbbootimg/format/segment_writer_p.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer.h"


namespace mb
{
namespace bootimg
{
namespace mtk
{

struct MtkWriterCtx
{
    // Header values
    android::AndroidHeader hdr;

    bool have_file_size;
    uint64_t file_size;

    SegmentWriter seg;
};

int mtk_writer_get_header(MbBiWriter *biw, void *userdata,
                          Header &header);
int mtk_writer_write_header(MbBiWriter *biw, void *userdata,
                            const Header &header);
int mtk_writer_get_entry(MbBiWriter *biw, void *userdata,
                         Entry &entry);
int mtk_writer_write_entry(MbBiWriter *biw, void *userdata,
                           const Entry &entry);
int mtk_writer_write_data(MbBiWriter *biw, void *userdata,
                          const void *buf, size_t buf_size,
                          size_t &bytes_written);
int mtk_writer_finish_entry(MbBiWriter *biw, void *userdata);
int mtk_writer_close(MbBiWriter *biw, void *userdata);
int mtk_writer_free(MbBiWriter *bir, void *userdata);

}
}
}
