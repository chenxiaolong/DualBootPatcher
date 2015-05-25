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

#include "libmbpio/win32/delete.h"

#include <windows.h>

#include "libmbpio/error.h"
#include "libmbpio/private/string.h"
#include "libmbpio/private/utf8.h"
#include "libmbpio/win32/error.h"

namespace io
{
namespace win32
{

class ScopedFindHandle {
public:
    ScopedFindHandle(HANDLE h) : handle(h)
    {
    }

    ~ScopedFindHandle() {
        FindClose(handle);
    }

private:
    HANDLE handle;
};

static bool win32RecursiveDelete(const std::wstring &path)
{
    std::wstring mask(path);
    mask += L"\\*";

    WIN32_FIND_DATAW findData;

    // First, delete the contents of the directory, recursively for subdirectories
    HANDLE searchHandle = FindFirstFileExW(
        mask.c_str(),           // lpFileName
        FindExInfoBasic,        // fInfoLevelId
        &findData,              // lpFindFileData
        FindExSearchNameMatch,  // fSearchOp
        nullptr,                // lpSearchFilter
        0                       // dwAdditionalFlags
    );
    if (searchHandle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND) {
            setLastError(Error::PlatformError, priv::format(
                    "%s: FindFirstFileExW() failed: %s",
                    utf8::utf16ToUtf8(path).c_str(),
                    errorToString(error).c_str()));
            return false;
        }
    } else {
        ScopedFindHandle scope(searchHandle);
        while (true) {
            if (wcscmp(findData.cFileName, L".") != 0
                    && wcscmp(findData.cFileName, L"..") != 0) {
                bool isDirectory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        || (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
                std::wstring childPath(path);
                childPath += L'\\';
                childPath += findData.cFileName;

                if (isDirectory) {
                    if (!win32RecursiveDelete(childPath)) {
                        return false;
                    }
                } else {
                    if (!DeleteFileW(childPath.c_str())) {
                        DWORD error = GetLastError();
                        setLastError(Error::PlatformError, priv::format(
                                "%s: DeleteFileW() failed: %s",
                                utf8::utf16ToUtf8(childPath).c_str(),
                                errorToString(error).c_str()));
                        return false;
                    }
                }
            }

            // Advance to the next file in the directory
            if (!FindNextFileW(searchHandle, &findData)) {
                DWORD error = GetLastError();
                if (error != ERROR_NO_MORE_FILES) {
                    setLastError(Error::PlatformError, priv::format(
                            "%s: FindNextFileW() failed: %s",
                            utf8::utf16ToUtf8(path).c_str(),
                            errorToString(error).c_str()));
                    return false;
                }
                break;
            }
        }
    }

    if (!RemoveDirectoryW(path.c_str())) {
        DWORD error = GetLastError();
        setLastError(Error::PlatformError, priv::format(
                "%s: RemoveDirectoryW() failed: %s",
                utf8::utf16ToUtf8(path).c_str(),
                errorToString(error).c_str()));
        return false;
    }

    return true;
}

bool deleteRecursively(const std::string &path)
{
    std::wstring wPath = utf8::utf8ToUtf16(path);
    return win32RecursiveDelete(wPath);
}

}
}
