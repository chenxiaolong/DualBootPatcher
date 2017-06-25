/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#ifdef __cplusplus
#  include <cstdarg>
#  include <cstdbool>
#  include <cstddef>
#else
#  include <stdarg.h>
#  include <stdbool.h>
#  include <stddef.h>
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

// String formatting
MB_PRINTF(1, 2)
MB_EXPORT char * mb_format(const char *fmt, ...);
MB_EXPORT char * mb_format_v(const char *fmt, va_list ap);

// String starts with
MB_EXPORT bool mb_starts_with_n(const char *string, size_t len_string,
                                const char *prefix, size_t len_prefix);
MB_EXPORT bool mb_starts_with_icase_n(const char *string, size_t len_string,
                                      const char *prefix, size_t len_prefix);
MB_EXPORT bool mb_starts_with(const char *string, const char *prefix);
MB_EXPORT bool mb_starts_with_icase(const char *string, const char *prefix);

// String ends with
MB_EXPORT bool mb_ends_with_n(const char *string, size_t len_string,
                              const char *suffix, size_t len_suffix);
MB_EXPORT bool mb_ends_with_icase_n(const char *string, size_t len_string,
                                    const char *suffix, size_t len_suffix);
MB_EXPORT bool mb_ends_with(const char *string, const char *suffix);
MB_EXPORT bool mb_ends_with_icase(const char *string, const char *suffix);

// String insert
MB_EXPORT int mb_mem_insert(void **mem, size_t *mem_size, size_t pos,
                            const void *data, size_t data_size);
MB_EXPORT int mb_str_insert(char **str, size_t pos, const char *s);

// String replace
MB_EXPORT int mb_mem_replace(void **mem, size_t *mem_size,
                             const void *from, size_t from_size,
                             const void *to, size_t to_size,
                             size_t n, size_t *n_replaced);
MB_EXPORT int mb_str_replace(char **str, const char *from, const char *to,
                             size_t n, size_t *n_replaced);

MB_END_C_DECLS
