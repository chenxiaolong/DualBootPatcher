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

// Idea and code from: http://www.nubaria.com/en/blog/?p=289

#include "libmbpio/private/utf8.h"

#include <vector>

#if IO_PLATFORM_WINDOWS
#include <windows.h>
#endif

namespace utf8
{

#if IO_PLATFORM_WINDOWS

std::string utf16ToUtf8(const wchar_t *wstr)
{
    std::string convertedString;
    int requiredSize = WideCharToMultiByte(
        CP_UTF8,        // CodePage
        0,              // dwFlags
        wstr,           // lpWideCharStr
        -1,             // cchWideChar
        0,              // lpMultiByteStr
        0,              // cbMultiByte
        0,              // lpDefaultChar
        0               // lpUsedDefaultChar
    );
    if (requiredSize > 0) {
        std::vector<char> buffer(requiredSize);
        WideCharToMultiByte(
            CP_UTF8,            // CodePage
            0,                  // dwFlags
            wstr,               // lpWideCharStr
            -1,                 // cchWideChar
            buffer.data(),      // lpMultiByteStr
            requiredSize,       // cbMultiByte
            0,                  // lpDefaultChar
            0                   // lpUsedDefaultChar
        );
        convertedString.assign(buffer.begin(), buffer.end() - 1);
    }

    return convertedString;
}

std::wstring utf8ToUtf16(const char *str)
{
    std::wstring convertedString;
    int requiredSize = MultiByteToWideChar(
        CP_UTF8,        // CodePage
        0,              // dwFlags
        str,            // lpMultiByteStr
        -1,             // cbMultiByte
        0,              // lpWideCharStr
        0               // cchWideChar
    );
    if (requiredSize > 0) {
        std::vector<wchar_t> buffer(requiredSize);
        MultiByteToWideChar(
            CP_UTF8,            // CodePage
            0,                  // dwFlags
            str,                // lpMultiByteStr
            -1,                 // cbMultiByte
            buffer.data(),      // lpWideCharStr
            requiredSize        // cchWideChar
        );
        convertedString.assign(buffer.begin(), buffer.end() - 1);
    }

    return convertedString;

}

std::string utf16ToUtf8(const std::wstring &wstr)
{
    return utf16ToUtf8(wstr.c_str());
}

std::wstring utf8ToUtf16(const std::string &str)
{
    return utf8ToUtf16(str.c_str());
}

#endif

}
