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

#include "mbpio/win32/delete.h"

#include <windows.h>

#include "mbcommon/locale.h"

#include "mbpio/error.h"
#include "mbpio/private/string.h"
#include "mbpio/win32/error.h"

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
                    "FindFirstFileExW() failed: %s",
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
                                "DeleteFileW() failed: %s",
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
                            "FindNextFileW() failed: %s",
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
                "RemoveDirectoryW() failed: %s",
                errorToString(error).c_str()));
        return false;
    }

    return true;
}

bool deleteRecursively(const std::string &path)
{
    wchar_t *wPath = mb::utf8_to_wcs(path.c_str());
    if (!wPath) {
        return false;
    }

    bool ret = win32RecursiveDelete(wPath);
    free(wPath);
    return ret;
}

}
}
