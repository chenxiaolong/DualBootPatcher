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

#include "mbcommon/string_p.h"
#include "mbcommon/locale.h"

#include "mbcommon/libc/string.h"

#ifdef _WIN32
#  define strncasecmp _strnicmp
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

void * _mb_mempcpy(void *dest, const void *src, size_t n)
{
#if defined(_WIN32) || defined(__ANDROID__)
    return static_cast<char *>(memcpy(dest, src, n)) + n;
#else
    return mempcpy(dest, src, n);
#endif
}

// No wide-character version of the format functions is provided because it's
// impossible to portably create an appropriately sized buffer with swprintf().
// Passing 0 as the length does not work and not every
// See: https://stackoverflow.com/questions/3693479/why-does-c-not-have-an-snwprintf-function

/*!
 * \brief Format a string
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
 * \brief Format a string using a `va_list`
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
 * \brief Check if string has prefix (allows non-NULL-terminated strings)
 *        (case sensitive)
 *
 * \param string String
 * \param len_string String length
 * \param prefix Prefix
 * \param len_prefix Prefix length
 *
 * \return Whether the string starts with the prefix
 */
bool mb_starts_with_n(const char *string, size_t len_string,
                      const char *prefix, size_t len_prefix)
{
    return len_string < len_prefix
            ? false
            : strncmp(string, prefix, len_prefix) == 0;
}

/*!
 * \brief Check if string has prefix (allows non-NULL-terminated strings)
 *        (case insensitive)
 *
 * \warning Use with care! The case-insensitive matching behavior is
 *          platform-dependent and may even fail to properly handle ASCII
 *          characters depending on the locale.
 *
 * \param string String
 * \param len_string String length
 * \param prefix Prefix
 * \param len_prefix Prefix length
 *
 * \return Whether the string starts with the prefix
 */
bool mb_starts_with_icase_n(const char *string, size_t len_string,
                            const char *prefix, size_t len_prefix)
{
    return len_string < len_prefix
            ? false
            : strncasecmp(string, prefix, len_prefix) == 0;
}

/*!
 * \brief Check if string has prefix
 *        (case sensitive)
 *
 * \param string String
 * \param prefix Prefix
 *
 * \return Whether the string starts with the prefix
 */
bool mb_starts_with(const char *string, const char *prefix)
{
    return mb_starts_with_n(string, strlen(string), prefix, strlen(prefix));
}

/*!
 * \brief Check if string has prefix
 *        (case insensitive)
 *
 * \warning Use with care! The case-insensitive matching behavior is
 *          platform-dependent and may even fail to properly handle ASCII
 *          characters depending on the locale.
 *
 * \param string String
 * \param prefix Prefix
 *
 * \return Whether the string starts with the prefix
 */
bool mb_starts_with_icase(const char *string, const char *prefix)
{
    return mb_starts_with_icase_n(string, strlen(string),
                                  prefix, strlen(prefix));
}

/*!
 * \brief Check if string has suffix (allows non-NULL-terminated strings)
 *        (case sensitive)
 *
 * \param string String
 * \param len_string String length
 * \param suffix Suffix
 * \param len_suffix Suffix length
 *
 * \return Whether the string ends with the suffix
 */
bool mb_ends_with_n(const char *string, size_t len_string,
                    const char *suffix, size_t len_suffix)
{
    return len_string < len_suffix
            ? false
            : strncmp(string + len_string - len_suffix,
                      suffix, len_suffix) == 0;
}

/*!
 * \brief Check if string has suffix (allows non-NULL-terminated strings)
 *        (case insensitive)
 *
 * \warning Use with care! The case-insensitive matching behavior is
 *          platform-dependent and may even fail to properly handle ASCII
 *          characters depending on the locale.
 *
 * \param string String
 * \param len_string String length
 * \param suffix Suffix
 * \param len_suffix Suffix length
 *
 * \return Whether the string ends with the suffix
 */
bool mb_ends_with_icase_n(const char *string, size_t len_string,
                          const char *suffix, size_t len_suffix)
{
    return len_string < len_suffix
            ? false
            : strncasecmp(string + len_string - len_suffix,
                          suffix, len_suffix) == 0;
}

/*!
 * \brief Check if string has suffix
 *        (case sensitive)
 *
 * \param string String
 * \param suffix Suffix
 *
 * \return Whether the string ends with the suffix
 */
bool mb_ends_with(const char *string, const char *suffix)
{
    return mb_ends_with_n(string, strlen(string), suffix, strlen(suffix));
}

/*!
 * \brief Check if string has suffix
 *        (case insensitive)
 *
 * \warning Use with care! The case-insensitive matching behavior is
 *          platform-dependent and may even fail to properly handle ASCII
 *          characters depending on the locale.
 *
 * \param string String
 * \param suffix Suffix
 *
 * \return Whether the string ends with the suffix
 */
bool mb_ends_with_icase(const char *string, const char *suffix)
{
    return mb_ends_with_icase_n(string, strlen(string), suffix, strlen(suffix));
}

/*!
 * \brief Insert byte sequence into byte sequence
 *
 * Insert (\p data, \p data_pos) into (\p *mem, \p *mem_size). It the function
 * succeeds, \p *mem will be passed to `free()` and \p *mem and \p *mem_size
 * will be updated to point to the newly allocated block of memory and its size.
 * If the function fails, \p *mem will be left unchanged.
 *
 * \param[in,out] mem Pointer to byte sequence to modify
 * \param[in,out] mem_size Pointer to size of bytes sequence to modify
 * \param[in] pos Position in which to insert new byte sequence
 *                (0 \<= \p pos \<= \p *mem_size)
 * \param[in] data New byte sequence to insert
 * \param[in] data_size Size of new byte sequence to insert
 *
 * \return
 *   * 0 if successful
 *   * -1 if an error occured, with `errno` set accordingly
 */
int mb_mem_insert(void **mem, size_t *mem_size, size_t pos,
                  const void *data, size_t data_size)
{
    void *buf;
    size_t buf_size;

    if (pos > *mem_size) {
        errno = EINVAL;
        return -1;
    } else if (*mem_size >= SIZE_MAX - data_size) {
        errno = EOVERFLOW;
        return -1;
    }

    buf_size = *mem_size + data_size;

    buf = static_cast<char *>(malloc(buf_size));
    if (!buf) {
        return -1;
    }

    void *target_ptr = buf;

    // Copy data left of the insertion point
    target_ptr = _mb_mempcpy(target_ptr, *mem, pos);

    // Copy new data
    target_ptr = _mb_mempcpy(target_ptr, data, data_size);

    // Copy data right of the insertion point
    target_ptr = _mb_mempcpy(target_ptr, static_cast<char*>(*mem) + pos,
                             *mem_size - pos);

    free(*mem);
    *mem = buf;
    *mem_size = buf_size;
    return 0;
}

/*!
 * \brief Insert string into string
 *
 * Insert \p s into \p *str. It the function succeeds. \p *str will be passed to
 * `free()` and \p *str will be updated to point to the newly allocated string.
 * If the function fails, \p *str will be left unchanged.
 *
 * \param[in,out] str Pointer to string to modify
 * \param[in] pos Position in which to insert new string
 *                (0 \<= \p pos \<= \p *mem_size)
 * \param[in] s New string to insert
 *
 * \return
 *   * 0 if successful
 *   * -1 if an error occured, with `errno` set accordingly
 */
int mb_str_insert(char **str, size_t pos, const char *s)
{
    size_t str_size;

    str_size = strlen(*str);

    if (pos > str_size) {
        errno = EINVAL;
        return -1;
    } else if (str_size == SIZE_MAX) {
        errno = EOVERFLOW;
        return -1;
    }

    ++str_size;

    return mb_mem_insert(reinterpret_cast<void **>(str), &str_size, pos,
                         s, strlen(s));
}

/*!
 * \brief Replace byte sequence in byte sequence
 *
 * Replace (\p from, \p from_size) with (\p to, \p to_size) in (\p *mem,
 * \p *mem_size), up to \p n times if \p n \> 0. If the function succeeds,
 * \p *mem will be passed to `free()` and \p *mem and \p *mem_size will be
 * updated to point to the newly allocated block of memory and its size. If
 * \p n_replaced is not NULL, the number of replacements done will be stored at
 * the value pointed by \p n_replaced. If the function fails, \p *mem will be
 * left unchanged.
 *
 * \param[in,out] mem Pointer to byte sequence to modify
 * \param[in,out] mem_size Pointer to size of bytes sequence to modify
 * \param[in] from Byte sequence to replace
 * \param[in] from_size Size of byte sequence to replace
 * \param[in] to Replacement byte sequence
 * \param[in] to_size Size of replacement byte sequence
 * \param[in] n Number of replacements to attempt (0 to disable limit)
 * \param[out] n_replaced Pointer to store number of replacements made
 *
 * \return
 *   * 0 if successful
 *   * -1 if an error occured, with `errno` set accordingly
 */
int mb_mem_replace(void **mem, size_t *mem_size,
                   const void *from, size_t from_size,
                   const void *to, size_t to_size,
                   size_t n, size_t *n_replaced)
{
    char *buf = nullptr;
    size_t buf_size = 0;
    void *target_ptr;
    char *base_ptr = static_cast<char *>(*mem);
    char *ptr = static_cast<char *>(*mem);
    size_t ptr_remain = *mem_size;
    size_t matches = 0;

    // Special case for replacing nothing
    if (from_size == 0) {
        if (n_replaced) {
            *n_replaced = 0;
        }
        return 0;
    }

    while ((n == 0 || matches < n) && (ptr = static_cast<char *>(
            mb_memmem(ptr, ptr_remain, from, from_size)))) {
        // Resize buffer to accomodate data
        if (buf_size >= SIZE_MAX - (ptr - base_ptr)
                || buf_size + (ptr - base_ptr) >= SIZE_MAX - to_size) {
            free(buf);
            errno = EOVERFLOW;
            return -1;
        }

        size_t new_buf_size = buf_size + (ptr - base_ptr) + to_size;
        char *new_buf = static_cast<char *>(realloc(buf, new_buf_size));
        if (!new_buf) {
            free(buf);
            return -1;
        }

        target_ptr = new_buf + buf_size;

        // Copy data left of the match
        target_ptr = _mb_mempcpy(target_ptr, base_ptr, ptr - base_ptr);

        // Copy replacement
        target_ptr = _mb_mempcpy(target_ptr, to, to_size);

        buf = new_buf;
        buf_size = new_buf_size;

        ptr += from_size;
        ptr_remain -= ptr - base_ptr;
        base_ptr = ptr;

        ++matches;
    }

    // Copy remainder of string
    if (ptr_remain > 0) {
        if (buf_size >= SIZE_MAX - ptr_remain) {
            free(buf);
            errno = EOVERFLOW;
            return -1;
        }

        size_t new_buf_size = buf_size + ptr_remain;
        char *new_buf = static_cast<char *>(realloc(buf, new_buf_size));
        if (!new_buf) {
            free(buf);
            return -1;
        }

        target_ptr = new_buf + buf_size;
        target_ptr = _mb_mempcpy(target_ptr, base_ptr, ptr_remain);

        buf = new_buf;
        buf_size = new_buf_size;
    }

    free(*mem);
    *mem = buf;
    *mem_size = buf_size;

    if (n_replaced) {
        *n_replaced = matches;
    }

    return 0;
}

/*!
 * \brief Replace string in string
 *
 * Replace \p from with \p to in \p *str, up to \p n times if \p n \> 0. If the
 * function succeeds, \p *str will be passed to `free()` and \p *str will be
 * updated to point to the newly allocated string. If \p n_replaced is not NULL,
 * the number of replacements done will be stored at the value pointed by
 * \p n_replaced. If the function fails, \p *str will be left unchanged.
 *
 * \param[in,out] str Pointer to string to modify
 * \param[in] from String to replace
 * \param[in] to Replacement string
 * \param[in] n Number of replacements to attempt (0 to disable limit)
 * \param[out] n_replaced Pointer to store number of replacements made
 *
 * \return
 *   * 0 if successful
 *   * -1 if an error occured, with `errno` set accordingly
 */
int mb_str_replace(char **str, const char *from, const char *to,
                   size_t n, size_t *n_replaced)
{
    size_t str_size = strlen(*str) + 1;

    return mb_mem_replace(reinterpret_cast<void **>(str), &str_size,
                          from, strlen(from), to, strlen(to), n, n_replaced);
}

MB_END_C_DECLS
