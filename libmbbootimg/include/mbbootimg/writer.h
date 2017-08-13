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

#include <cstdarg>
#include <cstddef>
#include <cwchar>

#include "mbcommon/common.h"

#include "mbbootimg/defs.h"

namespace mb
{
class File;

namespace bootimg
{

struct MbBiWriter;
class Entry;
class Header;

// Construction/destruction
MB_EXPORT MbBiWriter * mb_bi_writer_new(void);
MB_EXPORT int mb_bi_writer_free(MbBiWriter *biw);

// Open/close
MB_EXPORT int mb_bi_writer_open_filename(MbBiWriter *biw,
                                         const char *filename);
MB_EXPORT int mb_bi_writer_open_filename_w(MbBiWriter *biw,
                                           const wchar_t *filename);
MB_EXPORT int mb_bi_writer_open(MbBiWriter *biw, File *file,
                                bool owned);
MB_EXPORT int mb_bi_writer_close(MbBiWriter *biw);

// Operations
MB_EXPORT int mb_bi_writer_get_header(MbBiWriter *biw,
                                      Header *&header);
MB_EXPORT int mb_bi_writer_get_header2(MbBiWriter *biw,
                                       Header &header);
MB_EXPORT int mb_bi_writer_write_header(MbBiWriter *biw,
                                        const Header &header);
MB_EXPORT int mb_bi_writer_get_entry(MbBiWriter *biw,
                                     Entry *&entry);
MB_EXPORT int mb_bi_writer_get_entry2(MbBiWriter *biw,
                                      Entry &entry);
MB_EXPORT int mb_bi_writer_write_entry(MbBiWriter *biw,
                                       const Entry &entry);
MB_EXPORT int mb_bi_writer_write_data(MbBiWriter *biw, const void *buf,
                                      size_t size, size_t *bytes_written);

// Format operations
MB_EXPORT int mb_bi_writer_format_code(MbBiWriter *biw);
MB_EXPORT const char * mb_bi_writer_format_name(MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_by_code(MbBiWriter *biw,
                                              int code);
MB_EXPORT int mb_bi_writer_set_format_by_name(MbBiWriter *biw,
                                              const char *name);

// Specific formats
MB_EXPORT int mb_bi_writer_set_format_android(MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_bump(MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_loki(MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_mtk(MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_sony_elf(MbBiWriter *biw);

// Error handling functions
MB_EXPORT int mb_bi_writer_error(MbBiWriter *biw);
MB_EXPORT const char * mb_bi_writer_error_string(MbBiWriter *biw);
MB_PRINTF(3, 4)
MB_EXPORT int mb_bi_writer_set_error(MbBiWriter *biw, int error_code,
                                     const char *fmt, ...);
MB_EXPORT int mb_bi_writer_set_error_v(MbBiWriter *biw, int error_code,
                                       const char *fmt, va_list ap);

// TODO TODO TODO
// SET OPTIONS FUNCTION
// TODO TODO TODO

}
}
