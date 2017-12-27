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
#include <string_view>
#include <vector>

#include <cstdarg>
#include <cstdbool>
#include <cstddef>

#include "mbcommon/outcome.h"

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
MB_PRINTF(1, 2)
MB_EXPORT oc::result<std::string> format_safe(const char *fmt, ...);
MB_PRINTF(1, 2)
MB_EXPORT std::string format(const char *fmt, ...);
MB_EXPORT oc::result<std::string> format_v_safe(const char *fmt, va_list ap);
MB_EXPORT std::string format_v(const char *fmt, va_list ap);

// String starts with
MB_EXPORT bool starts_with(std::string_view string, std::string_view prefix);
MB_EXPORT bool starts_with_icase(std::string_view string, std::string_view prefix);

// String ends with
MB_EXPORT bool ends_with(std::string_view string, std::string_view suffix);
MB_EXPORT bool ends_with_icase(std::string_view string, std::string_view suffix);

// String insert
MB_EXPORT oc::result<void> mem_insert(void **mem, size_t *mem_size, size_t pos,
                                      const void *data, size_t data_size);
MB_EXPORT oc::result<void> str_insert(char **str, size_t pos, const char *s);

// String replace
MB_EXPORT oc::result<void> mem_replace(void **mem, size_t *mem_size,
                                       const void *from, size_t from_size,
                                       const void *to, size_t to_size,
                                       size_t n, size_t *n_replaced);
MB_EXPORT oc::result<void> str_replace(char **str, const char *from, const char *to,
                                       size_t n, size_t *n_replaced);

// String split/join

/*!
 * \brief Split a string by one or more delimiters
 *
 * \param str Input string
 * \param delim `char` delimiter or string of delimiters
 *
 * \return Vector of split components. If \p str or \p delim is empty, then
 *         a vector consisting a single element equal to \p str is returned.
 */
template<typename DelimType>
std::vector<std::string> split(std::string_view str, const DelimType &delim)
{
    std::size_t begin = 0;
    std::size_t end;
    std::vector<std::string> result;

    while ((end = str.find_first_of(delim, begin)) != std::string::npos) {
        result.emplace_back(str.substr(begin, end - begin));
        begin = end + 1;
    }
    result.emplace_back(str.substr(begin));

    return result;
}

/*!
 * \brief Split a string by one or more delimiters as string views
 *
 * \param str Input string
 * \param delim `char` delimiter or string of delimiters
 *
 * \return Vector of split components as string views. If \p str or \p delim is
 *         empty, then a vector consisting a single element equal to \p str is
 *         returned.
 */
template<typename DelimType>
std::vector<std::string_view> split_sv(std::string_view str, const DelimType &delim)
{
    std::size_t begin = 0;
    std::size_t end;
    std::vector<std::string_view> result;

    while ((end = str.find_first_of(delim, begin)) != std::string::npos) {
        result.push_back(str.substr(begin, end - begin));
        begin = end + 1;
    }
    result.push_back(str.substr(begin));

    return result;
}

/*!
 * \brief Join a string from a vector of components
 *
 * \param list Vector of strings or string views
 * \param delim Delimiter to join the strings. Can be any type accepted by
 *              `std::string`'s operator+=().
 *
 * \return Joined string
 */
template<typename StringType, typename DelimType>
std::string join(const std::vector<StringType> &list, const DelimType &delim)
{
    std::string result;
    bool first = true;

    for (auto const &str : list) {
        if (!first) {
            result += delim;
        } else {
            first = false;
        }
        result += str;
    }

    return result;
}

}
