/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libmbpio/path.h"

#include "libmbpio/private/common.h"

namespace io
{

#if IO_PLATFORM_WINDOWS
static const char *delims = "/\\";
#else
static const char *delims = "/";
#endif

/*!
 * \brief Get filename part of a path
 *
 * Returns everything after the last path separator (slash)
 */
std::string baseName(const std::string &path)
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
std::string dirName(const std::string &path)
{
    std::size_t pos = path.find_last_of(delims);
    if (pos == std::string::npos) {
        // No slash, return empty string
        return std::string();
    }

    return path.substr(0, pos + 1);
}

}
