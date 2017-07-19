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

#include <limits>
#include <type_traits>

#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cstdlib>

namespace mb
{
namespace util
{

template<typename SIntType>
inline bool str_to_snum(const char *str, int base, SIntType *out)
{
    static_assert(std::is_signed<SIntType>::value,
                  "Integer type is not signed");
    static_assert(std::numeric_limits<SIntType>::min() >= LLONG_MIN
                  && std::numeric_limits<SIntType>::max() <= LLONG_MAX,
                  "Integer type to too large to handle");

    char *end;
    errno = 0;
    auto num = strtoll(str, &end, base);
    if (errno == ERANGE
            || num < std::numeric_limits<SIntType>::min()
            || num > std::numeric_limits<SIntType>::max()) {
        errno = ERANGE;
        return false;
    } else if (*str == '\0' || *end != '\0') {
        errno = EINVAL;
        return false;
    }
    *out = static_cast<SIntType>(num);
    return true;
}

template<typename UIntType>
inline bool str_to_unum(const char *str, int base, UIntType *out)
{
    static_assert(!std::is_signed<UIntType>::value,
                  "Integer type is not unsigned");
    static_assert(std::numeric_limits<UIntType>::max() <= ULLONG_MAX,
                  "Integer type to too large to handle");

    char *end;
    errno = 0;
    auto num = strtoull(str, &end, base);
    if (errno == ERANGE
            || num > std::numeric_limits<UIntType>::max()) {
        errno = ERANGE;
        return false;
    } else if (*str == '\0' || *end != '\0') {
        errno = EINVAL;
        return false;
    }
    *out = static_cast<UIntType>(num);
    return true;
}

}
}
