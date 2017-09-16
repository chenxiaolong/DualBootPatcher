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

#include <vector>

#include <cstring>

#include "mbcommon/locale.h"
#include "mbcommon/string.h"

#include "mbpio/error.h"
#include "mbpio/private/common.h"

#if IO_PLATFORM_WINDOWS
#include "mbpio/win32/error.h"
#else
#include <cerrno>
#include <sys/stat.h>
#endif

namespace io
{

bool createDirectories(const std::string &path)
{
#if IO_PLATFORM_WINDOWS
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
#if IO_PLATFORM_WINDOWS
    std::wstring wTemp;
#endif

    if (path.empty()) {
        setLastError(Error::InvalidArguments, "Path cannot be empty");
        return false;
    }

    // Add leading separator if needed
    if (strchr(delim, path[0])) {
        temp += path[0];
    }

    p = strtok_r(&copy[0], delim, &save_ptr);
    while (p != nullptr) {
        temp += p;
        temp += pathsep;

#if IO_PLATFORM_WINDOWS
        if (!mb::utf8_to_wcs(wTemp, temp)) {
            setLastError(Error::PlatformError, mb::format(
                    "%s: Failed to convert UTF-16 to UTF-8: %s",
                    temp.c_str(), win32::errorToString(GetLastError()).c_str()));
            return false;
        }

        DWORD dwAttrib = GetFileAttributesW(wTemp.c_str());
        bool exists = (dwAttrib != INVALID_FILE_ATTRIBUTES)
                && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
        if (!exists && !CreateDirectoryW(wTemp.c_str(), nullptr)
                && GetLastError() != ERROR_ALREADY_EXISTS) {
            setLastError(Error::PlatformError, mb::format(
                    "%s: Failed to create directory: %s",
                    temp.c_str(), win32::errorToString(GetLastError()).c_str()));
            return false;
        }
#else
        if (mkdir(temp.c_str(), 0755) < 0 && errno != EEXIST) {
            setLastError(Error::PlatformError, mb::format(
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
