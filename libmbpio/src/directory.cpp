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

#include "mbpio/directory.h"

#include <vector>

#include <cstring>

#include "mbpio/error.h"
#include "mbpio/private/common.h"
#include "mbpio/private/string.h"
#include "mbpio/private/utf8.h"

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
    std::wstring wTemp;
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
        wTemp = utf8::utf8ToUtf16(temp.data());
        if (!CreateDirectoryW(wTemp.c_str(), nullptr)
                && GetLastError() != ERROR_ALREADY_EXISTS) {
            setLastError(Error::PlatformError, priv::format(
                    "%s: Failed to create directory: %s",
                    temp.data(), win32::errorToString(GetLastError()).c_str()));
#else
        if (mkdir(temp.data(), 0755) < 0 && errno != EEXIST) {
            setLastError(Error::PlatformError, priv::format(
                    "%s: Failed to create directory: %s",
                    temp.data(), strerror(errno)));
#endif
            return false;
        }

        p = strtok_r(nullptr, delim, &save_ptr);
    }

    return true;
}

}
