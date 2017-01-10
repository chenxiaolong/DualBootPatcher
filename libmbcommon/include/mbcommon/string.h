/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/common.h"

#ifdef __cplusplus
#  include <cstdarg>
#  include <cstdbool>
#  include <cstddef>
#  include <cwchar>
#else
#  include <stdarg.h>
#  include <stdbool.h>
#  include <stddef.h>
#  include <wchar.h>
#endif

// zu, zd, etc. are not supported until VS2015
#ifdef _WIN32
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

MB_BEGIN_C_DECLS

MB_PRINTF(1, 2)
MB_EXPORT char * mb_format(const char *fmt, ...);
MB_EXPORT char * mb_format_v(const char *fmt, va_list ap);

MB_EXPORT bool mb_starts_with_n(const char *string, size_t len_string,
                                const char *prefix, size_t len_prefix,
                                bool case_insensitive);
MB_EXPORT bool mb_starts_with(const char *string, const char *prefix,
                              bool case_insensitive);
MB_EXPORT bool mb_starts_with_w_n(const wchar_t *string, size_t len_string,
                                  const wchar_t *prefix, size_t len_prefix,
                                  bool case_insensitive);
MB_EXPORT bool mb_starts_with_w(const wchar_t *string, const wchar_t *prefix,
                                bool case_insensitive);

MB_EXPORT bool mb_ends_with_n(const char *string, size_t len_string,
                              const char *suffix, size_t len_suffix,
                              bool case_insensitive);
MB_EXPORT bool mb_ends_with(const char *string, const char *suffix,
                            bool case_insensitive);
MB_EXPORT bool mb_ends_with_w_n(const wchar_t *string, size_t len_string,
                                const wchar_t *suffix, size_t len_suffix,
                                bool case_insensitive);
MB_EXPORT bool mb_ends_with_w(const wchar_t *string, const wchar_t *suffix,
                              bool case_insensitive);

MB_END_C_DECLS
