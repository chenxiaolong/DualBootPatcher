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

#include "mbcommon/error_code.h"
#include "mbcommon/locale.h"
#include "mbcommon/string.h"

#include "mbpio/error.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <cerrno>
#  include <sys/stat.h>
#endif

namespace mb::io
{

bool create_directories(const std::string &path)
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
        set_last_error(Error::InvalidArguments, "Path cannot be empty");
        return false;
    }

    // Add leading separator if needed
    if (strchr(delim, path[0])) {
        temp += path[0];
    }

    p = strtok_r(copy.data(), delim, &save_ptr);
    while (p != nullptr) {
        temp += p;
        temp += pathsep;

#ifdef _WIN32
        auto w_temp = mb::utf8_to_wcs(temp);
        if (!w_temp) {
            set_last_error(Error::PlatformError, mb::format(
                    "%s: Failed to convert UTF-16 to UTF-8: %s",
                    temp.c_str(), w_temp.error().message().c_str()));
            return false;
        }

        DWORD dw_attrib = GetFileAttributesW(w_temp.value().c_str());
        bool exists = (dw_attrib != INVALID_FILE_ATTRIBUTES)
                && (dw_attrib & FILE_ATTRIBUTE_DIRECTORY);
        if (!exists && !CreateDirectoryW(w_temp.value().c_str(), nullptr)
                && GetLastError() != ERROR_ALREADY_EXISTS) {
            set_last_error(Error::PlatformError, mb::format(
                    "%s: Failed to create directory: %s",
                    temp.c_str(), mb::ec_from_win32().message().c_str()));
            return false;
        }
#else
        if (mkdir(temp.c_str(), 0755) < 0 && errno != EEXIST) {
            set_last_error(Error::PlatformError, mb::format(
                    "%s: Failed to create directory: %s",
                    temp.c_str(), strerror(errno)));
            return false;
        }
#endif

        p = strtok_r(nullptr, delim, &save_ptr);
    }

    return true;
}

}
