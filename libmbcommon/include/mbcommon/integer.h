/*
 * Copyright (C) 2016-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <charconv>
#include <string_view>
#include <type_traits>

#include <cctype>
#include <cerrno>

namespace mb
{

/*!
 * \brief Convert a string to an integer
 *
 * \param[in] str String to parse
 * \param[in] base Base of the integer value (valid: {2, 3, ..., 36})
 * \param[out] out Reference to store result
 *
 * \return
 *   * If conversion is successful and result is in range, then returns true and
 *     sets \p out to the result.
 *   * If conversion is successful, but result is out of range, then returns
 *     false and sets `errno` to ERANGE.
 *   * If IntType is unsigned and \p str represents a negative number, then
 *     returns false and sets `errno` to ERANGE. TODO TODO TODO
 *   * If \p str is an empty string, then returns false and sets `errno` to
 *     EINVAL.
 *   * If \p str contains characters that could not be parsed as a number, then
 *     returns false and sets `errno` to EINVAL.
 */
template<typename IntType>
inline bool str_to_num(std::string_view str, int base, IntType &out)
{
    // Skip leading whitespace
    while (!str.empty() && isspace(str.front())) {
        str.remove_prefix(1);
    }

    // Don't allow negative numbers with unsigned integers
    if constexpr (std::is_unsigned_v<IntType>) {
        if (!str.empty() && str.front() == '-') {
            errno = ERANGE;
            return false;
        }
    }

    auto result = std::from_chars(
            str.data(), str.data() + str.size(), out, base);

    if (result.ptr != str.data() + str.size()) {
        errno = EINVAL;
        return false;
    } else if (result.ec != std::errc()) {
        errno = static_cast<int>(result.ec);
        return false;
    }

    return true;
}

/*!
 * \brief Cast an integer to its unsigned representation
 *
 * \param num Number to cast
 *
 * \return Unsigned representation (of the same size) of \p num
 */
template<typename IntType>
inline std::make_unsigned_t<IntType> make_unsigned_v(IntType num)
{
    return static_cast<std::make_unsigned_t<IntType>>(num);
}

/*!
 * \brief Cast an integer to its signed representation
 *
 * \param num Number to cast
 *
 * \return Signed representation (of the same size) of \p num
 */
template<typename IntType>
inline std::make_signed_t<IntType> make_signed_v(IntType num)
{
    return static_cast<std::make_signed_t<IntType>>(num);
}

}
