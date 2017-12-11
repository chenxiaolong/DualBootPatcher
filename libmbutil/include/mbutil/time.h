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

#pragma once

#include <string>

#include <cinttypes>
#include <ctime>

namespace mb
{
namespace util
{

inline void timespec_diff(const timespec &start, const timespec &end,
                          timespec &result)
{
    result.tv_sec = end.tv_sec - start.tv_sec;
    result.tv_nsec = end.tv_nsec - start.tv_nsec;
    if (result.tv_nsec < 0) {
        --result.tv_sec;
        result.tv_nsec += 1e9;
    }
}

inline void timeval_diff(const timeval &start, const timeval &end,
                         timeval &result)
{
    result.tv_sec = end.tv_sec - start.tv_sec;
    result.tv_usec = end.tv_usec - start.tv_usec;
    if (result.tv_usec < 0) {
        --result.tv_sec;
        result.tv_usec += 1e6;
    }
}

inline int64_t timespec_diff_s(const timespec &start, const timespec &end)
{
    return end.tv_sec - start.tv_sec;
}

inline int64_t timeval_diff_s(const timeval &start, const timeval &end)
{
    return end.tv_sec - start.tv_sec;
}

inline int64_t timespec_diff_ms(const timespec &start, const timespec &end)
{
    return (static_cast<int64_t>(end.tv_sec) * 1000 + end.tv_nsec / 1000000)
            - (static_cast<int64_t>(start.tv_sec) * 1000 + start.tv_nsec / 1000000);
}

inline int64_t timeval_diff_ms(const timeval &start, const timeval &end)
{
    return (static_cast<int64_t>(end.tv_sec) * 1000 + end.tv_usec / 1000)
            - (static_cast<int64_t>(start.tv_sec) * 1000 + start.tv_usec / 1000);
}

inline int64_t timespec_diff_us(const timespec &start, const timespec &end)
{
    return (static_cast<int64_t>(end.tv_sec) * 1000000 + end.tv_nsec / 1000)
            - (static_cast<int64_t>(start.tv_sec) * 1000000 + start.tv_nsec / 1000);
}

inline int64_t timeval_diff_us(const timeval &start, const timeval &end)
{
    return (static_cast<int64_t>(end.tv_sec) * 1000000 + end.tv_usec)
            - (static_cast<int64_t>(start.tv_sec) * 1000000 + start.tv_usec);
}

inline int64_t timespec_diff_ns(const timespec &start, const timespec &end)
{
    return (static_cast<int64_t>(end.tv_sec) * 1000000000 + end.tv_nsec)
            - (static_cast<int64_t>(start.tv_sec) * 1000000000 + start.tv_nsec);
}

uint64_t current_time_ms();
bool format_time(const std::string &format, std::string &out);
std::string format_time(const std::string &format);

}
}
