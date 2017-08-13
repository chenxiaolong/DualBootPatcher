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

#include <string>

#include <cstdarg>
#include <cstddef>

#include "mbcommon/common.h"

#include "mbbootimg/defs.h"

namespace mb
{
class File;

namespace bootimg
{

struct MbBiReader;
class Entry;
class Header;

// Construction/destruction
MB_EXPORT MbBiReader * reader_new(void);
MB_EXPORT int reader_free(MbBiReader *bir);

// Open/close
MB_EXPORT int reader_open_filename(MbBiReader *bir,
                                   const std::string &filename);
MB_EXPORT int reader_open_filename_w(MbBiReader *bir,
                                     const std::wstring &filename);
MB_EXPORT int reader_open(MbBiReader *bir, File *file, bool owned);
MB_EXPORT int reader_close(MbBiReader *bir);

// Operations
MB_EXPORT int reader_read_header(MbBiReader *bir, Header *&header);
MB_EXPORT int reader_read_header2(MbBiReader *bir, Header &header);
MB_EXPORT int reader_read_entry(MbBiReader *bir, Entry *&entry);
MB_EXPORT int reader_read_entry2(MbBiReader *bir, Entry &entry);
MB_EXPORT int reader_go_to_entry(MbBiReader *bir, Entry *&entry,
                                 int entry_type);
MB_EXPORT int reader_go_to_entry2(MbBiReader *bir, Entry &entry,
                                  int entry_type);
MB_EXPORT int reader_read_data(MbBiReader *bir, void *buf,
                               size_t size, size_t &bytes_read);

// Format operations
MB_EXPORT int reader_format_code(MbBiReader *bir);
MB_EXPORT std::string reader_format_name(MbBiReader *bir);
MB_EXPORT int reader_set_format_by_code(MbBiReader *bir, int code);
MB_EXPORT int reader_set_format_by_name(MbBiReader *bir,
                                        const std::string &name);
MB_EXPORT int reader_enable_format_all(MbBiReader *bir);
MB_EXPORT int reader_enable_format_by_code(MbBiReader *bir, int code);
MB_EXPORT int reader_enable_format_by_name(MbBiReader *bir,
                                           const std::string &name);

// Specific formats
MB_EXPORT int reader_enable_format_android(MbBiReader *bir);
MB_EXPORT int reader_enable_format_bump(MbBiReader *bir);
MB_EXPORT int reader_enable_format_loki(MbBiReader *bir);
MB_EXPORT int reader_enable_format_mtk(MbBiReader *bir);
MB_EXPORT int reader_enable_format_sony_elf(MbBiReader *bir);

// Error handling functions
MB_EXPORT int reader_error(MbBiReader *bir);
MB_EXPORT const char * reader_error_string(MbBiReader *bir);
MB_PRINTF(3, 4)
MB_EXPORT int reader_set_error(MbBiReader *bir, int error_code,
                               const char *fmt, ...);
MB_EXPORT int reader_set_error_v(MbBiReader *bir, int error_code,
                                 const char *fmt, va_list ap);

// TODO TODO TODO
// SET OPTIONS FUNCTION
// TODO TODO TODO

}
}
