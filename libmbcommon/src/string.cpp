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

#include "mbcommon/string.h"

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#  include <windows.h>
#endif

#include "mbcommon/locale.h"

#ifdef _WIN32
#  define strncasecmp _strnicmp
#  define wcsncasecmp _wcsnicmp
#endif

/*!
 * \file mbcommon/string.h
 * \brief String utility functions
 */

/*!
 * \def MB_PRIzd
 *
 * \brief printf format argument to convert `ptrdiff_t` to base-10
 */

/*!
 * \def MB_PRIzi
 *
 * \brief printf format argument to convert `ptrdiff_t` to base-10
 */

/*!
 * \def MB_PRIzo
 *
 * \brief printf format argument to convert `size_t` to octal
 */

/*!
 * \def MB_PRIzu
 *
 * \brief printf format argument to convert `size_t` to base-10
 */

/*!
 * \def MB_PRIzx
 *
 * \brief printf format argument to convert `size_t` to lowercase hex
 */

/*!
 * \def MB_PRIzX
 *
 * \brief printf format argument to convert `size_t` to uppercase hex
 */

MB_BEGIN_C_DECLS

// No wide-character version of the format functions is provided because it's
// impossible to portably create an appropriately sized buffer with swprintf().
// Passing 0 as the length does not work and not every
// See: https://stackoverflow.com/questions/3693479/why-does-c-not-have-an-snwprintf-function

/*!
 * \brief Format an MBS string
 *
 * \note This uses the `*printf()` family of functions in the system's C
 *       library. The format string may not be understood the same way by every
 *       platform.
 *
 * \note The value of `errno` and `GetLastError()` (on Win32) are preserved if
 *       this function does not fail.
 *
 * \param fmt Format string
 * \param ... Format arguments
 *
 * \return Resulting string or NULL if out of memory or an error occurs. The
 *         result should be deallocated with `free()` when it is no longer
 *         needed.
 */
char * mb_format(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    char *result = mb_format_v(fmt, ap);
    va_end(ap);

    return result;
}

/*!
 * \brief Format an MBS string using a `va_list`
 *
 * \note This uses the `*printf()` family of functions in the system's C
 *       library. The format string may not be understood the same way by every
 *       platform.
 *
 * \note The value of `errno` and `GetLastError()` (on Win32) are preserved if
 *       this function does not fail.
 *
 * \param fmt Format string
 * \param ap Format arguments as `va_list`
 *
 * \return Resulting string or NULL if out of memory or an error occurs. The
 *         result should be deallocated with `free()` when it is no longer
 *         needed.
 */
char * mb_format_v(const char *fmt, va_list ap)
{
#ifdef _WIN32
    static_assert(INT_MAX <= SIZE_MAX, "INT_MAX > SIZE_MAX");

    int saved_errno;
    int saved_error;
    char *buf;
    size_t buf_size;
    int ret;
    va_list copy;

    saved_errno = errno;
    saved_error = GetLastError();

    va_copy(copy, ap);
    ret = vsnprintf(nullptr, 0, fmt, ap);
    va_end(copy);

    if (ret < 0 || ret == INT_MAX) {
        return nullptr;
    }

    buf_size = ret + 1;
    buf = static_cast<char *>(malloc(buf_size));
    if (!buf) {
        return nullptr;
    }

    va_copy(copy, ap);
    ret = vsnprintf(buf, buf_size, fmt, ap);
    va_end(copy);

    if (ret < 0) {
        free(buf);
        return nullptr;
    }

    // Restore errno and Win32 error on success
    errno = saved_errno;
    SetLastError(saved_error);

    return buf;
#else
    // Yay, GNU!
    int saved_errno;
    char *result;

    saved_errno = errno;

    if (vasprintf(&result, fmt, ap) < 0) {
        return nullptr;
    }

    // Restore errno on success
    errno = saved_errno;

    return result;
#endif
}

/*!
 * \brief Check if MBS string has prefix (allows non-NULL-terminated strings)
 *
 * \warning Use \p case_insensitive with care! The case-insensitive matching
 *          behavior is platform-dependent and may even fail to properly handle
 *          ASCII characters depending on the locale.
 *
 * \param string String
 * \param len_string String length
 * \param prefix Prefix
 * \param len_prefix Prefix length
 * \param case_insensitive Whether case-insensitive matching should be enabled
 *
 * \return Whether the string starts with the prefix
 */
bool mb_starts_with_n(const char *string, size_t len_string,
                      const char *prefix, size_t len_prefix,
                      bool case_insensitive)
{
    return len_string < len_prefix ? false :
            (case_insensitive ? strncasecmp : strncmp)
                    (string, prefix, len_prefix) == 0;
}

/*!
 * \brief Check if MBS string has prefix
 *
 * \warning Use \p case_insensitive with care! The case-insensitive matching
 *          behavior is platform-dependent and may even fail to properly handle
 *          ASCII characters depending on the locale.
 *
 * \param string String
 * \param prefix Prefix
 * \param case_insensitive Whether case-insensitive matching should be enabled
 *
 * \return Whether the string starts with the prefix
 */
bool mb_starts_with(const char *string, const char *prefix,
                    bool case_insensitive)
{
    return mb_starts_with_n(string, strlen(string), prefix, strlen(prefix),
                            case_insensitive);
}

/*!
 * \brief Check if WCS string has prefix (allows non-NULL-terminated strings)
 *
 * \warning Use \p case_insensitive with care! The case-insensitive matching
 *          behavior is platform-dependent and may even fail to properly handle
 *          ASCII characters depending on the locale.
 *
 * \param string String
 * \param len_string String length
 * \param prefix Prefix
 * \param len_prefix Prefix length
 * \param case_insensitive Whether case-insensitive matching should be enabled
 *
 * \return Whether the string starts with the prefix
 */
bool mb_starts_with_w_n(const wchar_t *string, size_t len_string,
                        const wchar_t *prefix, size_t len_prefix,
                        bool case_insensitive)
{
    return len_string < len_prefix ? false :
            (case_insensitive ? wcsncasecmp : wcsncmp)
                    (string, prefix, len_prefix) == 0;
}

/*!
 * \brief Check if WCS string has prefix
 *
 * \warning Use \p case_insensitive with care! The case-insensitive matching
 *          behavior is platform-dependent and may even fail to properly handle
 *          ASCII characters depending on the locale.
 *
 * \param string String
 * \param prefix Prefix
 * \param case_insensitive Whether case-insensitive matching should be enabled
 *
 * \return Whether the string starts with the prefix
 */
bool mb_starts_with_w(const wchar_t *string, const wchar_t *prefix,
                      bool case_insensitive)
{
    return mb_starts_with_w_n(string, wcslen(string), prefix, wcslen(prefix),
                              case_insensitive);
}

/*!
 * \brief Check if MBS string has suffix (allows non-NULL-terminated strings)
 *
 * \warning Use \p case_insensitive with care! The case-insensitive matching
 *          behavior is platform-dependent and may even fail to properly handle
 *          ASCII characters depending on the locale.
 *
 * \param string String
 * \param len_string String length
 * \param suffix Suffix
 * \param len_suffix Suffix length
 * \param case_insensitive Whether case-insensitive matching should be enabled
 *
 * \return Whether the string ends with the suffix
 */
bool mb_ends_with_n(const char *string, size_t len_string,
                    const char *suffix, size_t len_suffix,
                    bool case_insensitive)
{
    return len_string < len_suffix ? false :
            (case_insensitive ? strncasecmp : strncmp)
                    (string + len_string - len_suffix, suffix, len_suffix) == 0;
}

/*!
 * \brief Check if MBS string has suffix
 *
 * \warning Use \p case_insensitive with care! The case-insensitive matching
 *          behavior is platform-dependent and may even fail to properly handle
 *          ASCII characters depending on the locale.
 *
 * \param string String
 * \param suffix Suffix
 * \param case_insensitive Whether case-insensitive matching should be enabled
 *
 * \return Whether the string ends with the suffix
 */
bool mb_ends_with(const char *string, const char *suffix,
                  bool case_insensitive)
{
    return mb_ends_with_n(string, strlen(string), suffix, strlen(suffix),
                          case_insensitive);
}

/*!
 * \brief Check if WCS string has suffix (allows non-NULL-terminated strings)
 *
 * \warning Use \p case_insensitive with care! The case-insensitive matching
 *          behavior is platform-dependent and may even fail to properly handle
 *          ASCII characters depending on the locale.
 *
 * \param string String
 * \param len_string String length
 * \param suffix Suffix
 * \param len_suffix Suffix length
 * \param case_insensitive Whether case-insensitive matching should be enabled
 *
 * \return Whether the string ends with the suffix
 */
bool mb_ends_with_w_n(const wchar_t *string, size_t len_string,
                      const wchar_t *suffix, size_t len_suffix,
                      bool case_insensitive)
{
    return len_string < len_suffix ? false :
            (case_insensitive ? wcsncasecmp : wcsncmp)
                    (string + len_string - len_suffix, suffix, len_suffix) == 0;
}

/*!
 * \brief Check if WCS string has suffix
 *
 * \warning Use \p case_insensitive with care! The case-insensitive matching
 *          behavior is platform-dependent and may even fail to properly handle
 *          ASCII characters depending on the locale.
 *
 * \param string String
 * \param suffix Suffix
 * \param case_insensitive Whether case-insensitive matching should be enabled
 *
 * \return Whether the string ends with the suffix
 */
bool mb_ends_with_w(const wchar_t *string, const wchar_t *suffix,
                    bool case_insensitive)
{
    return mb_ends_with_w_n(string, wcslen(string), suffix, wcslen(suffix),
                            case_insensitive);
}

MB_END_C_DECLS
