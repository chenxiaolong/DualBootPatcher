/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/error_code.h"

using namespace std::chrono;

namespace mb::util
{

/*!
 * \brief Format date and time
 *
 * \param format Date and time format string
 * \param tp System clock time point to format
 *
 * \return Formatted timestamp or error
 */
oc::result<std::string> format_time(std::string_view format,
                                    system_clock::time_point tp)
{
    struct tm tm;

    auto t = system_clock::to_time_t(tp);

    // Some systems call tzset() in localtime_r() and others don't. We'll
    // manually call it to ensure the behavior is the same on every platform.
    tzset();

    if (!localtime_r(&t, &tm)) {
        return ec_from_errno();
    }

    // strftime() does not differentiate between errors and empty strings, so
    // prepend a character to ensure that a valid result is never empty.
    std::string f("x");
    f += format;

    char buf[100];
    if (strftime(buf, sizeof(buf), f.c_str(), &tm) == 0) {
        return std::errc::value_too_large;
    }

    return buf + 1;
}

}
