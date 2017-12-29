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

#include "mbpio/path.h"

namespace mb::io
{

#ifdef _WIN32
static const char *delims = "/\\";
static const char *pathsep = "\\";
#else
static const char *delims = "/";
static const char *pathsep = "/";
#endif

/*!
 * \brief Get filename part of a path
 *
 * Returns everything after the last path separator (slash)
 */
std::string base_name(const std::string &path)
{
    std::size_t pos = path.find_last_of(delims);
    if (pos == std::string::npos) {
        // No slash, return full path
        return path;
    }

    return path.substr(pos + 1);
}

/*!
 * \brief Get directory part of a path
 *
 * Returns everything up to and including the last path separator (slash)
 */
std::string dir_name(const std::string &path)
{
    std::size_t pos = path.find_last_of(delims);
    if (pos == std::string::npos) {
        // No slash, return empty string
        return {};
    }

    return path.substr(0, pos + 1);
}

std::string path_join(const std::vector<std::string> &components)
{
    std::string path;
    for (auto it = components.begin(); it != components.end(); ++it) {
        if (it->empty()) {
            path += pathsep;
        } else {
            path += *it;
            if (it != components.end() - 1) {
                path += pathsep;
            }
        }
    }
    return path;
}

}
