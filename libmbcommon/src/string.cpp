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

#include "mbcommon/error.h"
#include "mbcommon/locale.h"
#include "mbcommon/string_p.h"

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

MB_END_C_DECLS

namespace mb
{

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
 * \param[out] out Reference to store output string
 * \param[in] fmt Format string
 * \param[in] ... Format arguments
 *
 * \return Whether the format string was successfully processed.
 */
bool format(std::string &out, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    bool result = format_v(out, fmt, ap);
    va_end(ap);

    return result;
}

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
 * \return Resulting formatted string.
 * \throws std::exception If an error occurs while formatting the string.
 */
std::string format(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    // TODO: This should use exceptions to report errors, but we currently don't
    // support them due to the substantial size increase of the compiled
    // binaries.
    std::string result = format_v(fmt, ap);
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
 * \note The value of `errno` and `GetLastError()` (on Win32) are always
 *       preserved.
 *
 * \param[out] out Reference to store output string
 * \param[in] fmt Format string
 * \param[in] ap Format arguments as `va_list`
 *
 * \return Whether the format string was successfully processed.
 */
bool format_v(std::string &out, const char *fmt, va_list ap)
{
    static_assert(INT_MAX <= SIZE_MAX, "INT_MAX > SIZE_MAX");

    ErrorRestorer restorer;
    int ret;
    va_list copy;

    va_copy(copy, ap);
    ret = vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);

    if (ret < 0 || static_cast<size_t>(ret) == SIZE_MAX) {
        return false;
    }

    // C++11 guarantees that the memory is contiguous, but does not guarantee
    // that the internal buffer is NULL-terminated, so we'll make room for '\0'
    // and then get rid of it.
    out.resize(static_cast<size_t>(ret) + 1);

    va_copy(copy, ap);
    // NOTE: Change `&out[0]` to `out.data()` once we target C++17.
    ret = vsnprintf(&out[0], out.size(), fmt, copy);
    va_end(copy);

    if (ret < 0) {
        return false;
    }

    out.resize(static_cast<size_t>(ret));

    return true;
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
 * \return Resulting formatted string.
 * \throws std::exception If an error occurs while formatting the string.
 */
std::string format_v(const char *fmt, va_list ap)
{
    std::string result;
    // TODO: This should use exceptions to report errors, but we currently don't
    // support them due to the substantial size increase of the compiled
    // binaries.
    format_v(result, fmt, ap);
    return result;
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
bool starts_with_n(const char *string, size_t len_string,
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
bool starts_with_icase_n(const char *string, size_t len_string,
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
bool starts_with(const char *string, const char *prefix)
{
    return starts_with_n(string, strlen(string), prefix, strlen(prefix));
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
bool starts_with(const std::string &string, const char *prefix)
{
    return starts_with_n(string.c_str(), string.size(), prefix, strlen(prefix));
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
bool starts_with(const char *string, const std::string &prefix)
{
    return starts_with_n(string, strlen(string), prefix.c_str(), prefix.size());
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
bool starts_with(const std::string &string, const std::string &prefix)
{
    return starts_with_n(string.c_str(), string.size(),
                         prefix.c_str(), prefix.size());
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
bool starts_with_icase(const char *string, const char *prefix)
{
    return starts_with_icase_n(string, strlen(string),
                               prefix, strlen(prefix));
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
bool starts_with_icase(const std::string &string, const char *prefix)
{
    return starts_with_icase_n(string.c_str(), string.size(),
                               prefix, strlen(prefix));
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
bool starts_with_icase(const char *string, const std::string &prefix)
{
    return starts_with_icase_n(string, strlen(string),
                               prefix.c_str(), prefix.size());
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
bool starts_with_icase(const std::string &string, const std::string &prefix)
{
    return starts_with_icase_n(string.c_str(), string.size(),
                               prefix.c_str(), prefix.size());
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
bool ends_with_n(const char *string, size_t len_string,
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
bool ends_with_icase_n(const char *string, size_t len_string,
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
bool ends_with(const char *string, const char *suffix)
{
    return ends_with_n(string, strlen(string), suffix, strlen(suffix));
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
bool ends_with(const std::string &string, const char *suffix)
{
    return ends_with_n(string.c_str(), string.size(), suffix, strlen(suffix));
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
bool ends_with(const char *string, const std::string &suffix)
{
    return ends_with_n(string, strlen(string), suffix.c_str(), suffix.size());
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
bool ends_with(const std::string &string, const std::string &suffix)
{
    return ends_with_n(string.c_str(), string.size(),
                       suffix.c_str(), suffix.size());
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
bool ends_with_icase(const char *string, const char *suffix)
{
    return ends_with_icase_n(string, strlen(string), suffix, strlen(suffix));
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
bool ends_with_icase(const std::string &string, const char *suffix)
{
    return ends_with_icase_n(string.c_str(), string.size(),
                             suffix, strlen(suffix));
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
bool ends_with_icase(const char *string, const std::string &suffix)
{
    return ends_with_icase_n(string, strlen(string),
                             suffix.c_str(), suffix.size());
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
bool ends_with_icase(const std::string &string, const std::string &suffix)
{
    return ends_with_icase_n(string.c_str(), string.size(),
                             suffix.c_str(), suffix.size());
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
 *   * -1 if an error occurred, with `errno` set accordingly
 */
int mem_insert(void **mem, size_t *mem_size, size_t pos,
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
 *   * -1 if an error occurred, with `errno` set accordingly
 */
int str_insert(char **str, size_t pos, const char *s)
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

    return mem_insert(reinterpret_cast<void **>(str), &str_size, pos,
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
 *   * -1 if an error occurred, with `errno` set accordingly
 */
int mem_replace(void **mem, size_t *mem_size,
                const void *from, size_t from_size,
                const void *to, size_t to_size,
                size_t n, size_t *n_replaced)
{
    char *buf = nullptr;
    size_t buf_size = 0;
    void *target_ptr;
    auto base_ptr = static_cast<char *>(*mem);
    auto ptr = static_cast<char *>(*mem);
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
        if (buf_size >= SIZE_MAX - static_cast<size_t>(ptr - base_ptr)
                || buf_size + static_cast<size_t>(ptr - base_ptr)
                        >= SIZE_MAX - to_size) {
            free(buf);
            errno = EOVERFLOW;
            return -1;
        }

        size_t new_buf_size =
                buf_size + static_cast<size_t>(ptr - base_ptr) + to_size;
        auto new_buf = static_cast<char *>(realloc(buf, new_buf_size));
        if (!new_buf) {
            free(buf);
            return -1;
        }

        target_ptr = new_buf + buf_size;

        // Copy data left of the match
        target_ptr = _mb_mempcpy(target_ptr, base_ptr,
                                 static_cast<size_t>(ptr - base_ptr));

        // Copy replacement
        target_ptr = _mb_mempcpy(target_ptr, to, to_size);

        buf = new_buf;
        buf_size = new_buf_size;

        ptr += from_size;
        ptr_remain -= static_cast<size_t>(ptr - base_ptr);
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
        auto new_buf = static_cast<char *>(realloc(buf, new_buf_size));
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
 *   * -1 if an error occurred, with `errno` set accordingly
 */
int str_replace(char **str, const char *from, const char *to,
                size_t n, size_t *n_replaced)
{
    size_t str_size = strlen(*str) + 1;

    return mem_replace(reinterpret_cast<void **>(str), &str_size,
                       from, strlen(from), to, strlen(to), n, n_replaced);
}

}
