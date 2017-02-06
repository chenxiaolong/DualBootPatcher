/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifdef __cplusplus
#  include <cstdarg>
#  include <cstddef>
#  include <cwchar>
#else
#  include <stdarg.h>
#  include <stddef.h>
#  include <wchar.h>
#endif

#include "mbcommon/common.h"

#include "mbbootimg/defs.h"

MB_BEGIN_C_DECLS

struct MbBiWriter;
struct MbBiEntry;
struct MbBiHeader;
struct MbFile;

// Construction/destruction
MB_EXPORT struct MbBiWriter * mb_bi_writer_new(void);
MB_EXPORT int mb_bi_writer_free(struct MbBiWriter *biw);

// Open/close
MB_EXPORT int mb_bi_writer_open_filename(struct MbBiWriter *biw,
                                         const char *filename);
MB_EXPORT int mb_bi_writer_open_filename_w(struct MbBiWriter *biw,
                                           const wchar_t *filename);
MB_EXPORT int mb_bi_writer_open(struct MbBiWriter *biw, MbFile *file,
                                bool owned);
MB_EXPORT int mb_bi_writer_close(struct MbBiWriter *biw);

// Operations
MB_EXPORT int mb_bi_writer_get_header(struct MbBiWriter *biw,
                                      struct MbBiHeader **header);
MB_EXPORT int mb_bi_writer_write_header(struct MbBiWriter *biw,
                                        struct MbBiHeader *header);
MB_EXPORT int mb_bi_writer_get_entry(struct MbBiWriter *biw,
                                     struct MbBiEntry **entry);
MB_EXPORT int mb_bi_writer_write_entry(struct MbBiWriter *biw,
                                       struct MbBiEntry *entry);
MB_EXPORT int mb_bi_writer_write_data(struct MbBiWriter *biw, const void *buf,
                                      size_t size, size_t *bytes_written);

// Format operations
MB_EXPORT int mb_bi_writer_format_code(struct MbBiWriter *biw);
MB_EXPORT const char * mb_bi_writer_format_name(struct MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_by_code(struct MbBiWriter *biw,
                                              int code);
MB_EXPORT int mb_bi_writer_set_format_by_name(struct MbBiWriter *biw,
                                              const char *name);

// Specific formats
MB_EXPORT int mb_bi_writer_set_format_android(struct MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_bump(struct MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_loki(struct MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_mtk(struct MbBiWriter *biw);
MB_EXPORT int mb_bi_writer_set_format_sony_elf(struct MbBiWriter *biw);

// Error handling functions
MB_EXPORT int mb_bi_writer_error(struct MbBiWriter *biw);
MB_EXPORT const char * mb_bi_writer_error_string(struct MbBiWriter *biw);
MB_PRINTF(3, 4)
MB_EXPORT int mb_bi_writer_set_error(struct MbBiWriter *biw, int error_code,
                                     const char *fmt, ...);
MB_EXPORT int mb_bi_writer_set_error_v(struct MbBiWriter *biw, int error_code,
                                       const char *fmt, va_list ap);

// TODO TODO TODO
// SET OPTIONS FUNCTION
// TODO TODO TODO

MB_END_C_DECLS
