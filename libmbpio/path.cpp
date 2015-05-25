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

#include <vector>

#include "libmbpio/private/common.h"
#include "libmbpio/private/utf8.h"

#if IO_PLATFORM_ANDROID || IO_PLATFORM_POSIX
#include <libgen.h>
#endif

namespace io
{

std::string baseName(const std::string &path)
{
#if IO_PLATFORM_WINDOWS
    std::wstring wPath = utf8::utf8ToUtf16(path);
    std::vector<wchar_t> buf(wPath.size());
    std::vector<wchar_t> extbuf(wPath.size());

    errno_t ret = _wsplitpath_s(
        wPath.c_str(),  // path
        nullptr,        // drive
        0,              // driveNumberOfElements
        nullptr,        // dir
        0,              // dirNumberOfElements
        buf.data(),     // fname
        buf.size(),     // nameNumberOfElements
        extbuf.data(),  // ext
        extbuf.size()   // extNumberOfElements
    );

    if (ret != 0) {
        return std::string();
    } else {
        return utf8::utf16ToUtf8(buf.data()) + utf8::utf16ToUtf8(extbuf.data());
    }
#else
    std::vector<char> buf(path.begin(), path.end());
    buf.push_back('\0');

    char *ptr = basename(buf.data());
    return std::string(ptr);
#endif
}

std::string dirName(const std::string &path)
{
#if IO_PLATFORM_WINDOWS
    std::wstring wPath = utf8::utf8ToUtf16(path);
    std::vector<wchar_t> buf(wPath.size());

    errno_t ret = _wsplitpath_s(
        wPath.c_str(),  // path
        nullptr,        // drive
        0,              // driveNumberOfElements
        buf.data(),     // dir
        buf.size(),     // dirNumberOfElements
        nullptr,        // fname
        0,              // nameNumberOfElements
        nullptr,        // ext
        0               // extNumberOfElements
    );

    if (ret != 0) {
        return std::string();
    } else {
        return utf8::utf16ToUtf8(buf.data());
    }
#else
    std::vector<char> buf(path.begin(), path.end());
    buf.push_back('\0');

    char *ptr = dirname(buf.data());
    return std::string(ptr);
#endif
}

}
