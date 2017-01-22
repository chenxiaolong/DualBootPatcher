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

struct MbBiReader;
struct MbBiEntry;
struct MbBiHeader;
struct MbFile;

// Construction/destruction
MB_EXPORT struct MbBiReader * mb_bi_reader_new(void);
MB_EXPORT int mb_bi_reader_free(struct MbBiReader *bir);

// Open/close
MB_EXPORT int mb_bi_reader_open_filename(struct MbBiReader *bir,
                                         const char *filename);
MB_EXPORT int mb_bi_reader_open_filename_w(struct MbBiReader *bir,
                                           const wchar_t *filename);
MB_EXPORT int mb_bi_reader_open(struct MbBiReader *bir,
                                struct MbFile *file, bool owned);
MB_EXPORT int mb_bi_reader_close(struct MbBiReader *bir);

// Operations
MB_EXPORT int mb_bi_reader_read_header(struct MbBiReader *bir,
                                       struct MbBiHeader **header);
MB_EXPORT int mb_bi_reader_read_header2(struct MbBiReader *bir,
                                        struct MbBiHeader *header);
MB_EXPORT int mb_bi_reader_read_entry(struct MbBiReader *bir,
                                      struct MbBiEntry **entry);
MB_EXPORT int mb_bi_reader_read_entry2(struct MbBiReader *bir,
                                       struct MbBiEntry *entry);
MB_EXPORT int mb_bi_reader_read_data(struct MbBiReader *bir, void *buf,
                                     size_t size, size_t *bytes_read);

// Format operations
MB_EXPORT int mb_bi_reader_format_code(struct MbBiReader *bir);
MB_EXPORT const char * mb_bi_reader_format_name(struct MbBiReader *bir);
MB_EXPORT int mb_bi_reader_set_format_by_code(struct MbBiReader *bir,
                                              int code);
MB_EXPORT int mb_bi_reader_set_format_by_name(struct MbBiReader *bir,
                                              const char *name);
MB_EXPORT int mb_bi_reader_enable_format_all(struct MbBiReader *bir);
MB_EXPORT int mb_bi_reader_enable_format_by_code(struct MbBiReader *bir,
                                                 int code);
MB_EXPORT int mb_bi_reader_enable_format_by_name(struct MbBiReader *bir,
                                                 const char *name);

// Specific formats
MB_EXPORT int mb_bi_reader_enable_format_android(struct MbBiReader *bir);
MB_EXPORT int mb_bi_reader_enable_format_bump(struct MbBiReader *bir);
MB_EXPORT int mb_bi_reader_enable_format_loki(struct MbBiReader *bir);
MB_EXPORT int mb_bi_reader_enable_format_mtk(struct MbBiReader *bir);
MB_EXPORT int mb_bi_reader_enable_format_sony_elf(struct MbBiReader *bir);

// Error handling functions
MB_EXPORT int mb_bi_reader_error(struct MbBiReader *bir);
MB_EXPORT const char * mb_bi_reader_error_string(struct MbBiReader *bir);
MB_PRINTF(3, 4)
MB_EXPORT int mb_bi_reader_set_error(struct MbBiReader *bir, int error_code,
                                     const char *fmt, ...);
MB_EXPORT int mb_bi_reader_set_error_v(struct MbBiReader *bir, int error_code,
                                       const char *fmt, va_list ap);

// TODO TODO TODO
// SET OPTIONS FUNCTION
// TODO TODO TODO

MB_END_C_DECLS
