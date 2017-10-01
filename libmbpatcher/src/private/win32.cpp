/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbpatcher/private/win32.h"

#include "mbcommon/locale.h"


namespace mb
{
namespace patcher
{

std::string win32_error_to_string(DWORD win32_error)
{
    LPWSTR message_buffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,            // dwFlags
        nullptr,                                        // lpSource
        win32_error,                                    // dwMessageId
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),      // dwLanguageId
        reinterpret_cast<LPWSTR>(&message_buffer),      // lpBuffer
        0,                                              // nSize
        nullptr                                         // Arguments
    );

    std::wstring message(message_buffer, size);
    LocalFree(message_buffer);

    return wcs_to_utf8(message);
}

}
}
