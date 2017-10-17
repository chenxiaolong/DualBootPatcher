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

#include "mbpio/directory.h"

#include <cstring>

#include "mbcommon/error/error.h"
#include "mbcommon/error/type/ec_error.h"
#include "mbcommon/locale.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <cerrno>
#  include <sys/stat.h>
#endif

namespace mb
{
namespace io
{

Expected<void> create_directories(const std::string &path)
{
#ifdef _WIN32
    constexpr char delim[] = "/\\";
    constexpr char pathsep[] = "\\";
#else
    constexpr char delim[] = "/";
    constexpr char pathsep[] = "/";
#endif
    char *p;
    char *save_ptr;
    std::string temp;
    std::string copy = path;
#ifdef _WIN32
    std::wstring w_temp;
#endif

    if (path.empty()) {
        return make_error<ECError>(
                std::make_error_code(std::errc::invalid_argument));
    }

    // Add leading separator if needed
    if (strchr(delim, path[0])) {
        temp += path[0];
    }

    p = strtok_r(&copy[0], delim, &save_ptr);
    while (p != nullptr) {
        temp += p;
        temp += pathsep;

#ifdef _WIN32
        auto w_temp = mb::utf8_to_wcs(temp);
        if (!w_temp) {
            return w_temp.take_error();
        }

        DWORD dw_attrib = GetFileAttributesW(w_temp->c_str());
        bool exists = (dw_attrib != INVALID_FILE_ATTRIBUTES)
                && (dw_attrib & FILE_ATTRIBUTE_DIRECTORY);
        if (!exists && !CreateDirectoryW(w_temp->c_str(), nullptr)
                && GetLastError() != ERROR_ALREADY_EXISTS) {
            return make_error<ECError>(
                    ECError::from_win32_error(GetLastError()));
        }
#else
        if (mkdir(temp.c_str(), 0755) < 0 && errno != EEXIST) {
            return make_error<ECError>(ECError::from_errno(errno));
        }
#endif

        p = strtok_r(nullptr, delim, &save_ptr);
    }

    return {};
}

}
}
