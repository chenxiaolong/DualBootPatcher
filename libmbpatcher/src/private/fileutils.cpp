/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbpatcher/private/fileutils.h"

#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "mbcommon/error_code.h"
#include "mbcommon/locale.h"

#include "mblog/logging.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/stat.h>
#endif

#define LOG_TAG "mbpatcher/private/fileutils"


namespace mb::patcher
{

oc::result<void> FileUtils::open_file(StandardFile &file,
                                      const std::string &path,
                                      FileOpenMode mode)
{
#ifdef _WIN32
    auto w_filename = utf8_to_wcs(path);

    if (!w_filename) {
        LOGE("%s: Failed to convert from UTF8 to WCS: %s",
             path.c_str(), w_filename.error().message().c_str());
        return w_filename.as_failure();
    }

    return file.open(w_filename.value(), mode);
#else
    return file.open(path, mode);
#endif
}

/*!
 * \brief Read contents of a file into a string
 *
 * \param path Path to file
 * \param contents Output string (not modified unless reading succeeds)
 *
 * \return Success or not
 */
ErrorCode FileUtils::read_to_string(const std::string &path,
                                    std::string *contents)
{
    StandardFile file;

    auto ret = open_file(file, path, FileOpenMode::ReadOnly);
    if (!ret) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), ret.error().message().c_str());
        return ErrorCode::FileOpenError;
    }

    auto size = file.seek(0, SEEK_END);
    if (!size) {
        LOGE("%s: Failed to seek file: %s",
             path.c_str(), size.error().message().c_str());
        return ErrorCode::FileSeekError;
    }
    auto seek_ret = file.seek(0, SEEK_SET);
    if (!seek_ret) {
        LOGE("%s: Failed to seek file: %s",
             path.c_str(), seek_ret.error().message().c_str());
        return ErrorCode::FileSeekError;
    }

    std::string data;
    data.resize(static_cast<size_t>(size.value()));

    auto bytes_read = file.read(data.data(), data.size());
    if (!bytes_read || bytes_read.value() != size.value()) {
        LOGE("%s: Failed to read file: %s",
             path.c_str(), bytes_read.error().message().c_str());
        return ErrorCode::FileReadError;
    }

    data.swap(*contents);

    return ErrorCode::NoError;
}

ErrorCode FileUtils::write_from_string(const std::string &path,
                                       const std::string &contents)
{
    StandardFile file;

    auto ret = open_file(file, path, FileOpenMode::WriteOnly);
    if (!ret) {
        LOGE("%s: Failed to open for writing: %s",
             path.c_str(), ret.error().message().c_str());
        return ErrorCode::FileOpenError;
    }

    auto bytes_written = file.write(contents.data(), contents.size());
    if (!bytes_written || bytes_written.value() != contents.size()) {
        LOGE("%s: Failed to write file: %s",
             path.c_str(), bytes_written.error().message().c_str());
        return ErrorCode::FileWriteError;
    }

    return ErrorCode::NoError;
}

#ifdef _WIN32
static bool directory_exists(const wchar_t *path)
{
    DWORD dwAttrib = GetFileAttributesW(path);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES)
            && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}
#else
static bool directory_exists(const char *path)
{
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}
#endif

std::string FileUtils::system_temporary_dir()
{
#ifdef _WIN32
    const wchar_t *value;
    std::wstring w_path;
    std::string path;

    if ((value = _wgetenv(L"TMP")) && directory_exists(value)) {
        w_path = value;
        goto done;
    }
    if ((value = _wgetenv(L"TEMP")) && directory_exists(value)) {
        w_path = value;
        goto done;
    }
    if ((value = _wgetenv(L"LOCALAPPDATA"))) {
        w_path = value;
        w_path += L"\\Temp";
        if (directory_exists(w_path.c_str())) {
            goto done;
        }
    }
    if ((value = _wgetenv(L"USERPROFILE"))) {
        w_path = value;
        w_path += L"\\Temp";
        if (directory_exists(w_path.c_str())) {
            goto done;
        }
    }

done:
    auto result = wcs_to_utf8(w_path);
    return result ? std::move(result.value()) : std::string();
#else
    const char *value;

    if ((value = getenv("TMPDIR")) && directory_exists(value)) {
        return value;
    }
    if ((value = getenv("TMP")) && directory_exists(value)) {
        return value;
    }
    if ((value = getenv("TEMP")) && directory_exists(value)) {
        return value;
    }
    if ((value = getenv("TEMPDIR")) && directory_exists(value)) {
        return value;
    }

    return "/tmp";
#endif
}

std::string FileUtils::create_temporary_dir(const std::string &directory)
{
#ifdef _WIN32
    // This Win32 code is adapted from the awesome libuv library (MIT-licensed)
    // https://github.com/libuv/libuv/blob/v1.x/src/win/fs.c

    constexpr char possible[] =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    constexpr size_t num_chars = 62;
    constexpr size_t num_x = 6;
    constexpr char suffix[] = "\\mbpatcher-";

    HCRYPTPROV h_prov;

    bool ret = CryptAcquireContext(
        &h_prov,                // phProv
        nullptr,                // pszContainer
        nullptr,                // pszProvider
        PROV_RSA_FULL,          // dwProvType
        CRYPT_VERIFYCONTEXT     // dwFlags
    );
    if (!ret) {
        LOGE("CryptAcquireContext() failed: %s",
             ec_from_win32().message().c_str());
        return std::string();
    }

    std::string new_path = directory;           // Path
    new_path += suffix;                         // "\mbpatcher-"
    new_path.resize(new_path.size() + num_x);   // Unique part

    char *unique = &new_path[new_path.size() - num_x];

    unsigned int tries = TMP_MAX;
    do {
        uint64_t v;

        ret = CryptGenRandom(
            h_prov,                      // hProv
            sizeof(v),                   // dwLen
            reinterpret_cast<BYTE *>(&v) // pbBuffer
        );
        if (!ret) {
            LOGE("CryptGenRandom() failed: %s",
                 ec_from_win32().message().c_str());
            break;
        }

        for (size_t i = 0; i < num_x; ++i) {
            unique[i] = possible[v % num_chars];
            v /= num_chars;
        }

        // This is not particularly fast, but it'll do for now
        auto w_new_path = utf8_to_wcs(new_path);
        if (!w_new_path) {
            LOGE("Failed to convert UTF-8 to WCS: %s",
                 w_new_path.error().message().c_str());
            break;
        }

        ret = CreateDirectoryW(
            w_new_path.value().c_str(), // lpPathName
            nullptr                     // lpSecurityAttributes
        );

        if (ret) {
            break;
        } else if (GetLastError() != ERROR_ALREADY_EXISTS) {
            LOGE("CreateDirectoryW() failed: %s",
                 ec_from_win32().message().c_str());
            new_path.clear();
            break;
        }
    } while (--tries);

    bool released = CryptReleaseContext(
        h_prov, // hProv
        0       // dwFlags
    );
    assert(released);

    return new_path;
#else
    std::string dir_template(directory);
    dir_template += "/mbpatcher-XXXXXX";

    if (mkdtemp(dir_template.data())) {
        return dir_template;
    }

    return {};
#endif
}

}
