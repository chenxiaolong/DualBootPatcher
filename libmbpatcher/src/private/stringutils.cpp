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

#include "mbpatcher/private/stringutils.h"

#include <algorithm>
#include <memory>
#include <cstdarg>
#include <cstring>

std::vector<std::string> StringUtils::split(const std::string &str, char delim)
{
    std::size_t begin = 0;
    std::size_t end;
    std::vector<std::string> result;

    while ((end = str.find(delim, begin)) != std::string::npos) {
        result.push_back(str.substr(begin, end - begin));
        begin = end + 1;
    }
    result.push_back(str.substr(begin));

    return result;
}

std::string StringUtils::join(std::vector<std::string> &list,
                              const std::string &delim)
{
    std::string result;
    bool first = true;

    for (const std::string &str : list) {
        if (!first) {
            result += delim;
        } else {
            first = false;
        }
        result += str;
    }

    return result;
}
