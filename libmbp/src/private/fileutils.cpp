/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbp/private/fileutils.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "mbcommon/locale.h"

#include "mblog/logging.h"

#include "mbpio/file.h"

#ifdef _WIN32
#include "mbp/private/win32.h"
#include <windows.h>
#else
#include <sys/stat.h>
#endif


namespace mbp
{

/*!
    \brief Read contents of a file into memory

    \param path Path to file
    \param contents Output vector (not modified unless reading succeeds)

    \return Success or not
 */
ErrorCode FileUtils::readToMemory(const std::string &path,
                                  std::vector<unsigned char> *contents)
{
    io::File file;
    if (!file.open(path, io::File::OpenRead)) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), file.errorString().c_str());
        return ErrorCode::FileOpenError;
    }

    uint64_t size;
    file.seek(0, io::File::SeekEnd);
    file.tell(&size);
    file.seek(0, io::File::SeekBegin);

    std::vector<unsigned char> data(size);

    uint64_t bytesRead;
    if (!file.read(data.data(), data.size(), &bytesRead) || bytesRead != size) {
        LOGE("%s: Failed to read file: %s",
             path.c_str(), file.errorString().c_str());
        return ErrorCode::FileReadError;
    }

    data.swap(*contents);

    return ErrorCode::NoError;
}

/*!
    \brief Read contents of a file into a string

    \param path Path to file
    \param contents Output string (not modified unless reading succeeds)

    \return Success or not
 */
ErrorCode FileUtils::readToString(const std::string &path,
                                  std::string *contents)
{
    io::File file;
    if (!file.open(path, io::File::OpenRead)) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), file.errorString().c_str());
        return ErrorCode::FileOpenError;
    }

    uint64_t size;
    file.seek(0, io::File::SeekEnd);
    file.tell(&size);
    file.seek(0, io::File::SeekBegin);

    std::string data;
    data.resize(size);

    uint64_t bytesRead;
    if (!file.read(&data[0], size, &bytesRead) || bytesRead != size) {
        LOGE("%s: Failed to read file: %s",
             path.c_str(), file.errorString().c_str());
        return ErrorCode::FileReadError;
    }

    data.swap(*contents);

    return ErrorCode::NoError;
}

ErrorCode FileUtils::writeFromMemory(const std::string &path,
                                     const std::vector<unsigned char> &contents)
{
    io::File file;
    if (!file.open(path, io::File::OpenWrite)) {
        LOGE("%s: Failed to open for writing: %s",
             path.c_str(), file.errorString().c_str());
        return ErrorCode::FileOpenError;
    }

    uint64_t bytesWritten;
    if (!file.write(contents.data(), contents.size(), &bytesWritten)) {
        LOGE("%s: Failed to write file: %s",
             path.c_str(), file.errorString().c_str());
        return ErrorCode::FileWriteError;
    }

    return ErrorCode::NoError;
}

ErrorCode FileUtils::writeFromString(const std::string &path,
                                     const std::string &contents)
{
    io::File file;
    if (!file.open(path, io::File::OpenWrite)) {
        LOGE("%s: Failed to open for writing: %s",
             path.c_str(), file.errorString().c_str());
        return ErrorCode::FileOpenError;
    }

    uint64_t bytesWritten;
    if (!file.write(contents.data(), contents.size(), &bytesWritten)) {
        LOGE("%s: Failed to write file: %s",
             path.c_str(), file.errorString().c_str());
        return ErrorCode::FileWriteError;
    }

    return ErrorCode::NoError;
}

#ifdef _WIN32
static bool directoryExists(const wchar_t *path)
{
    DWORD dwAttrib = GetFileAttributesW(path);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES)
            && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}
#else
static bool directoryExists(const char *path)
{
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}
#endif

std::string FileUtils::systemTemporaryDir()
{
#ifdef _WIN32
    const wchar_t *value;
    std::wstring wPath;
    std::string path;

    if ((value = _wgetenv(L"TMP")) && directoryExists(value)) {
        wPath = value;
        goto done;
    }
    if ((value = _wgetenv(L"TEMP")) && directoryExists(value)) {
        wPath = value;
        goto done;
    }
    if ((value = _wgetenv(L"LOCALAPPDATA"))) {
        wPath = value;
        wPath += L"\\Temp";
        if (directoryExists(wPath.c_str())) {
            goto done;
        }
    }
    if ((value = _wgetenv(L"USERPROFILE"))) {
        wPath = value;
        wPath += L"\\Temp";
        if (directoryExists(wPath.c_str())) {
            goto done;
        }
    }

done:
    char *temp = mb::wcs_to_utf8(wPath.c_str());
    if (temp) {
        path = temp;
        free(temp);
    }

    return path;
#else
    const char *value;

    if ((value = getenv("TMPDIR")) && directoryExists(value)) {
        return value;
    }
    if ((value = getenv("TMP")) && directoryExists(value)) {
        return value;
    }
    if ((value = getenv("TEMP")) && directoryExists(value)) {
        return value;
    }
    if ((value = getenv("TEMPDIR")) && directoryExists(value)) {
        return value;
    }

    return "/tmp";
#endif
}

std::string FileUtils::createTemporaryDir(const std::string &directory)
{
#ifdef _WIN32
    // This Win32 code is adapted from the awesome libuv library (MIT-licensed)
    // https://github.com/libuv/libuv/blob/v1.x/src/win/fs.c

    static const char *possible =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const size_t numChars = 62;
    static const size_t numX = 6;
    static const char *suffix = "\\mbp-";

    HCRYPTPROV hProv;
    std::string newPath;

    bool ret = CryptAcquireContext(
        &hProv,                 // phProv
        nullptr,                // pszContainer
        nullptr,                // pszProvider
        PROV_RSA_FULL,          // dwProvType
        CRYPT_VERIFYCONTEXT     // dwFlags
    );
    if (!ret) {
        LOGE("CryptAcquireContext() failed: %s",
             win32ErrorToString(GetLastError()).c_str());
        return std::string();
    }

    std::vector<char> buf(directory.begin(), directory.end()); // Path
    buf.insert(buf.end(), suffix, suffix + strlen(suffix));    // "\mbp-"
    buf.resize(buf.size() + numX);                             // Unique part
    buf.push_back('\0');                                       // 0-terminator
    char *unique = buf.data() + buf.size() - numX - 1;

    unsigned int tries = TMP_MAX;
    do {
        uint64_t v;

        ret = CryptGenRandom(
            hProv,      // hProv
            sizeof(v),  // dwLen
            (BYTE *) &v // pbBuffer
        );
        if (!ret) {
            LOGE("CryptGenRandom() failed: %s",
                 win32ErrorToString(GetLastError()).c_str());
            break;
        }

        for (size_t i = 0; i < numX; ++i) {
            unique[i] = possible[v % numChars];
            v /= numChars;
        }

        // This is not particularly fast, but it'll do for now
        wchar_t *wBuf = mb::utf8_to_wcs(buf.data());
        if (!wBuf) {
            LOGE("Failed to convert UTF-8 to WCS: %s",
                 win32ErrorToString(GetLastError()).c_str());
            break;
        }

        ret = CreateDirectoryW(
            wBuf,               // lpPathName
            nullptr             // lpSecurityAttributes
        );

        free(wBuf);

        if (ret) {
            newPath = buf.data();
            break;
        } else if (GetLastError() != ERROR_ALREADY_EXISTS) {
            LOGE("CreateDirectoryW() failed: %s",
                 win32ErrorToString(GetLastError()).c_str());
            break;
        }
    } while (--tries);

    bool released = CryptReleaseContext(
        hProv,  // hProv
        0       // dwFlags
    );
    assert(released);

    return newPath;
#else
    std::string dirTemplate(directory);
    dirTemplate += "/mbp-XXXXXX";

    // mkdtemp modifies buffer
    std::vector<char> buf(dirTemplate.begin(), dirTemplate.end());
    buf.push_back('\0');

    if (mkdtemp(buf.data())) {
        return buf.data();
    }

    return std::string();
#endif
}

}
