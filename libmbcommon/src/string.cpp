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

#include <algorithm>

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
#include "mbcommon/error_code.h"
#include "mbcommon/locale.h"

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
 * \param fmt Format string
 * \param ... Format arguments
 *
 * \return Resulting formatted string or error.
 */
oc::result<std::string> format_safe(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    auto result = format_v_safe(fmt, ap);
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
 * \param fmt Format string
 * \param ap Format arguments as `va_list`
 *
 * \return Resulting formatted string or error.
 */
oc::result<std::string> format_v_safe(const char *fmt, va_list ap)
{
    static_assert(INT_MAX <= SIZE_MAX, "INT_MAX > SIZE_MAX");

    ErrorRestorer restorer;
    int ret;
    va_list copy;
    std::string buf;

    va_copy(copy, ap);
    ret = vsnprintf(nullptr, 0, fmt, copy);
    va_end(copy);

    if (ret < 0) {
        return ec_from_errno();
    } else if (static_cast<size_t>(ret) == SIZE_MAX) {
        return std::make_error_code(std::errc::value_too_large);
    }

    // C++11 guarantees that the memory is contiguous, but does not guarantee
    // that the internal buffer is NULL-terminated, so we'll make room for '\0'
    // and then get rid of it.
    buf.resize(static_cast<size_t>(ret) + 1);

    va_copy(copy, ap);
    ret = vsnprintf(buf.data(), buf.size(), fmt, copy);
    va_end(copy);

    if (ret < 0) {
        return ec_from_errno();
    }

    buf.resize(static_cast<size_t>(ret));

    return std::move(buf);
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
    auto result = format_v_safe(fmt, ap);
    if (!result) {
        // TODO: This should use exceptions to report errors, but we currently
        // don't support them due to the substantial size increase of the
        // compiled binaries.
        std::terminate();
    }
    return std::move(result.value());
}

/*!
 * \brief Check if string has prefix (case sensitive)
 *
 * \param string String
 * \param prefix Prefix
 *
 * \return Whether the string starts with the prefix
 */
bool starts_with(std::string_view string, std::string_view prefix)
{
    return string.size() < prefix.size()
            ? false
            : strncmp(string.data(), prefix.data(), prefix.size()) == 0;
}

/*!
 * \brief Check if string has prefix (case insensitive)
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
bool starts_with_icase(std::string_view string, std::string_view prefix)
{
    return string.size() < prefix.size()
            ? false
            : strncasecmp(string.data(), prefix.data(), prefix.size()) == 0;
}

/*!
 * \brief Check if string has suffix (case sensitive)
 *
 * \param string String
 * \param suffix Suffix
 *
 * \return Whether the string ends with the suffix
 */
bool ends_with(std::string_view string, std::string_view suffix)
{
    return string.size() < suffix.size()
            ? false
            : strncmp(string.data() + string.size() - suffix.size(),
                      suffix.data(), suffix.size()) == 0;
}

/*!
 * \brief Check if string has suffix (case insensitive)
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
bool ends_with_icase(std::string_view string, std::string_view suffix)
{
    return string.size() < suffix.size()
            ? false
            : strncasecmp(string.data() + string.size() - suffix.size(),
                          suffix.data(), suffix.size()) == 0;
}

template<typename Iterator>
static Iterator find_first_non_space(Iterator begin, Iterator end)
{
    return std::find_if(begin, end, [](char c) {
        return !std::isspace(c);
    });
}

/*!
 * \brief Trim whitespace from the left (in place)
 *
 * \param s Reference to string to trim
 */
void trim_left(std::string &s)
{
    s.erase(s.begin(), find_first_non_space(s.begin(), s.end()));
}

/*!
 * \brief Trim whitespace from the right (in place)
 *
 * \param s Reference to string to trim
 */
void trim_right(std::string &s)
{
    s.erase(find_first_non_space(s.rbegin(), s.rend()).base(), s.end());
}

/*!
 * \brief Trim whitespace from the left and the right (in place)
 *
 * \param s Reference to string to trim
 */
void trim(std::string &s)
{
    trim_left(s);
    trim_right(s);
}

/*!
 * \brief Trim whitespace from the left
 *
 * \param s String view to trim
 */
std::string_view trimmed_left(std::string_view s)
{
    return s.substr(static_cast<size_t>(
            find_first_non_space(s.begin(), s.end()) - s.begin()));
}

/*!
 * \brief Trim whitespace from the right
 *
 * \param s String view to trim
 */
std::string_view trimmed_right(std::string_view s)
{
    return s.substr(0, static_cast<size_t>(find_first_non_space(
            s.rbegin(), s.rend()).base() - s.begin()));
}

/*!
 * \brief Trim whitespace from the left and the right
 *
 * \param s String view to trim
 */
std::string_view trimmed(std::string_view s)
{
    return trimmed_left(trimmed_right(s));
}

}
