/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/directory.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>

#include "mbcommon/error_code.h"
#include "mbutil/path.h"

namespace mb::util
{

oc::result<void> mkdir_recursive(std::string dir, mode_t perms)
{
    if (dir.empty()) {
        return std::errc::invalid_argument;
    }

    std::string temp;
    temp.reserve(dir.size() + 1);

    if (dir[0] == '/') {
        temp += '/';
    }

    char *save_ptr;
    for (char *p = strtok_r(dir.data(), "/", &save_ptr); p;
            p = strtok_r(nullptr, "/", &save_ptr)) {
        temp += p;
        temp += '/';

        if (mkdir(temp.data(), perms) < 0 && errno != EEXIST) {
            return ec_from_errno();
        }
    }

    return oc::success();
}

oc::result<void> mkdir_parent(const std::string &path, mode_t perms)
{
    if (path.empty()) {
        return std::errc::invalid_argument;
    }

    struct stat sb;
    std::string dir = dir_name(path);

    if (auto r = mkdir_recursive(dir, perms);
            !r && r.error() != std::errc::file_exists) {
        return r.as_failure();
    }

    if (stat(dir.c_str(), &sb) < 0) {
        return ec_from_errno();
    }

    if (!S_ISDIR(sb.st_mode)) {
        return std::errc::not_a_directory;
    }

    return oc::success();
}

}
