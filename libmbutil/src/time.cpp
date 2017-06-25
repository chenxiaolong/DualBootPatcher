/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/time.h"

#include <ctime>

namespace mb
{
namespace util
{

uint64_t current_time_ms()
{
    struct timespec res;
    clock_gettime(CLOCK_REALTIME, &res);
    return 1000u * res.tv_sec + res.tv_nsec / 1e6;
}

/*!
 * \brief Format date and time
 *
 * \note: Because strftime does not differentiate between a buffer size error
 *        and an empty string result, if \a format results in an empty string,
 *        then this function will return false and \a out will not be modified.
 *
 * \param format Date and time format string
 * \param out Output string
 *
 * \return Whether the date and time was successfully formatted
 */
bool format_time(const std::string &format, std::string *out)
{
    struct timespec res;
    struct tm tm;
    if (clock_gettime(CLOCK_REALTIME, &res) < 0) {
        return false;
    }
    if (!localtime_r(&res.tv_sec, &tm)) {
        return false;
    }

    char buf[100];
    if (strftime(buf, sizeof(buf), format.c_str(), &tm) == 0) {
        return false;
    }

    *out = buf;
    return true;
}

std::string format_time(const std::string &format)
{
    std::string result;
    format_time(format, &result);
    return result;
}

}
}
