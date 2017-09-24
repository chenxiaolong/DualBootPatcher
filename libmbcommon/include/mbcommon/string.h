/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/common.h"

#include <string>

#include <cstdarg>
#include <cstdbool>
#include <cstddef>

// zu, zd, etc. are not supported until VS2015
#if defined(_WIN32) && !__USE_MINGW_ANSI_STDIO
#  define MB_PRIzd "Id"
#  define MB_PRIzi "Ii"
#  define MB_PRIzo "Io"
#  define MB_PRIzu "Iu"
#  define MB_PRIzx "Ix"
#  define MB_PRIzX "IX"
#else
#  define MB_PRIzd "zd"
#  define MB_PRIzi "zi"
#  define MB_PRIzo "zo"
#  define MB_PRIzu "zu"
#  define MB_PRIzx "zx"
#  define MB_PRIzX "zX"
#endif

namespace mb
{

// String formatting
MB_PRINTF(2, 3)
MB_EXPORT bool format(std::string &out, const char *fmt, ...);
MB_PRINTF(1, 2)
MB_EXPORT std::string format(const char *fmt, ...);
MB_EXPORT bool format_v(std::string &out, const char *fmt, va_list ap);
MB_EXPORT std::string format_v(const char *fmt, va_list ap);

// String starts with
MB_EXPORT bool starts_with_n(const char *string, size_t len_string,
                             const char *prefix, size_t len_prefix);
MB_EXPORT bool starts_with_icase_n(const char *string, size_t len_string,
                                   const char *prefix, size_t len_prefix);
MB_EXPORT bool starts_with(const char *string, const char *prefix);
MB_EXPORT bool starts_with(const std::string &string, const char *prefix);
MB_EXPORT bool starts_with(const char *string, const std::string &prefix);
MB_EXPORT bool starts_with(const std::string &string, const std::string &prefix);
MB_EXPORT bool starts_with_icase(const char *string, const char *prefix);
MB_EXPORT bool starts_with_icase(const std::string &string, const char *prefix);
MB_EXPORT bool starts_with_icase(const char *string, const std::string &prefix);
MB_EXPORT bool starts_with_icase(const std::string &string, const std::string &prefix);

// String ends with
MB_EXPORT bool ends_with_n(const char *string, size_t len_string,
                           const char *suffix, size_t len_suffix);
MB_EXPORT bool ends_with_icase_n(const char *string, size_t len_string,
                                 const char *suffix, size_t len_suffix);
MB_EXPORT bool ends_with(const char *string, const char *suffix);
MB_EXPORT bool ends_with(const std::string &string, const char *suffix);
MB_EXPORT bool ends_with(const char *string, const std::string &suffix);
MB_EXPORT bool ends_with(const std::string &string, const std::string &suffix);
MB_EXPORT bool ends_with_icase(const char *string, const char *suffix);
MB_EXPORT bool ends_with_icase(const std::string &string, const char *suffix);
MB_EXPORT bool ends_with_icase(const char *string, const std::string &suffix);
MB_EXPORT bool ends_with_icase(const std::string &string, const std::string &suffix);

// String insert
MB_EXPORT int mem_insert(void **mem, size_t *mem_size, size_t pos,
                         const void *data, size_t data_size);
MB_EXPORT int str_insert(char **str, size_t pos, const char *s);

// String replace
MB_EXPORT int mem_replace(void **mem, size_t *mem_size,
                          const void *from, size_t from_size,
                          const void *to, size_t to_size,
                          size_t n, size_t *n_replaced);
MB_EXPORT int str_replace(char **str, const char *from, const char *to,
                          size_t n, size_t *n_replaced);

}
