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

#include "mbpio/directory.h"

#include <vector>

#include <cstring>

#include "mbcommon/locale.h"

#include "mbpio/error.h"
#include "mbpio/private/common.h"
#include "mbpio/private/string.h"

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
    static const char *delim = "/\\";
    static const char *pathsep = "\\";
#else
    static const char *delim = "/";
    static const char *pathsep = "/";
#endif
    char *p;
    char *save_ptr;
    std::vector<char> temp;
    std::vector<char> copy;
#if IO_PLATFORM_WINDOWS
    wchar_t *wTemp;
#endif

    if (path.empty()) {
        setLastError(Error::InvalidArguments, "Path cannot be empty");
        return false;
    }

    copy.assign(path.begin(), path.end());
    copy.push_back('\0');
    temp.resize(path.size() + 2);
    temp[0] = '\0';

    // Add leading separator if needed
    if (strchr(delim, path[0])) {
        temp[0] = path[0];
        temp[1] = '\0';
    }

    p = strtok_r(copy.data(), delim, &save_ptr);
    while (p != nullptr) {
        strcat(temp.data(), p);
        strcat(temp.data(), pathsep);

#if IO_PLATFORM_WINDOWS
        wTemp = mb::utf8_to_wcs(temp.data());
        if (!wTemp) {
            setLastError(Error::PlatformError, priv::format(
                    "%s: Failed to convert UTF-16 to UTF-8: %s",
                    temp.data(), win32::errorToString(GetLastError()).c_str()));
            return false;
        }

        DWORD dwAttrib = GetFileAttributesW(wTemp);
        bool exists = (dwAttrib != INVALID_FILE_ATTRIBUTES)
                && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
        if (!exists && !CreateDirectoryW(wTemp, nullptr)
                && GetLastError() != ERROR_ALREADY_EXISTS) {
            free(wTemp);
            setLastError(Error::PlatformError, priv::format(
                    "%s: Failed to create directory: %s",
                    temp.data(), win32::errorToString(GetLastError()).c_str()));
            return false;
        }
        free(wTemp);
#else
        if (mkdir(temp.data(), 0755) < 0 && errno != EEXIST) {
            setLastError(Error::PlatformError, priv::format(
                    "%s: Failed to create directory: %s",
                    temp.data(), strerror(errno)));
            return false;
        }
#endif

        p = strtok_r(nullptr, delim, &save_ptr);
    }

    return true;
}

}
