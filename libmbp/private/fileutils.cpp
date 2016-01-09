/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "private/fileutils.h"

#include <algorithm>

#include <cassert>
#include <cerrno>
#include <cstring>

#include "libmbpio/directory.h"
#include "libmbpio/error.h"
#include "libmbpio/file.h"
#include "libmbpio/path.h"
#include "libmbpio/private/utf8.h"

#include "private/logging.h"

#include "external/minizip/ioapi_buf.h"
#if defined(_WIN32)
#  define MINIZIP_WIN32
#  include "external/minizip/iowin32.h"
#  include <wchar.h>
#elif defined(__ANDROID__)
#  define MINIZIP_ANDROID
#  include "external/minizip/ioandroid.h"
#endif

#ifndef _WIN32
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
        FLOGE("%s: Failed to open for reading: %s",
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
        FLOGE("%s: Failed to read file: %s",
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
        FLOGE("%s: Failed to open for reading: %s",
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
        FLOGE("%s: Failed to read file: %s",
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
        FLOGE("%s: Failed to open for writing: %s",
              path.c_str(), file.errorString().c_str());
        return ErrorCode::FileOpenError;
    }

    uint64_t bytesWritten;
    if (!file.write(contents.data(), contents.size(), &bytesWritten)) {
        FLOGE("%s: Failed to write file: %s",
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
        FLOGE("%s: Failed to open for writing: %s",
              path.c_str(), file.errorString().c_str());
        return ErrorCode::FileOpenError;
    }

    uint64_t bytesWritten;
    if (!file.write(contents.data(), contents.size(), &bytesWritten)) {
        FLOGE("%s: Failed to write file: %s",
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

    if ((value = _wgetenv(L"TMP")) && directoryExists(value)) {
        return utf8::utf16ToUtf8(value);
    }
    if ((value = _wgetenv(L"TEMP")) && directoryExists(value)) {
        return utf8::utf16ToUtf8(value);
    }
    if ((value = _wgetenv(L"LOCALAPPDATA"))) {
        std::wstring path(value);
        path += L"\\Temp";
        if (directoryExists(path.c_str())) {
            return utf8::utf16ToUtf8(path);
        }
    }
    if ((value = _wgetenv(L"USERPROFILE"))) {
        std::wstring path(value);
        path += L"\\Temp";
        if (directoryExists(path.c_str())) {
            return utf8::utf16ToUtf8(path);
        }
    }

    return std::string();
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

#ifdef _WIN32
static std::string win32ErrorToString(DWORD win32Error)
{
    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,            // dwFlags
        nullptr,                                        // lpSource
        win32Error,                                     // dwMessageId
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),      // dwLanguageId
        (LPWSTR) &messageBuffer,                        // lpBuffer
        0,                                              // nSize
        nullptr                                         // Arguments
    );

    std::wstring message(messageBuffer, size);
    LocalFree(messageBuffer);

    return utf8::utf16ToUtf8(message);
}
#endif

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
        FLOGE("CryptAcquireContext() failed: %s",
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
            FLOGE("CryptGenRandom() failed: %s",
                  win32ErrorToString(GetLastError()).c_str());
            break;
        }

        for (size_t i = 0; i < numX; ++i) {
            unique[i] = possible[v % numChars];
            v /= numChars;
        }

        // This is not particularly fast, but it'll do for now
        std::wstring wBuf = utf8::utf8ToUtf16(buf.data());

        ret = CreateDirectoryW(
            wBuf.c_str(),       // lpPathName
            nullptr             // lpSecurityAttributes
        );
        if (ret) {
            newPath = buf.data();
            break;
        } else if (GetLastError() != ERROR_ALREADY_EXISTS) {
            FLOGE("CreateDirectoryW() failed: %s",
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

static std::string zlibErrorString(int ret)
{
    switch (ret) {
    case Z_ERRNO:
        return strerror(errno);
    case Z_STREAM_ERROR:
        return "Z_STREAM_ERROR";
    case Z_DATA_ERROR:
        return "Z_DATA_ERROR";
    case Z_MEM_ERROR:
        return "Z_MEM_ERROR";
    case Z_BUF_ERROR:
        return "Z_BUF_ERROR";
    case Z_VERSION_ERROR:
        return "Z_VERSION_ERROR";
    default:
        return "(unknown)";
    }
}

std::string FileUtils::mzUnzErrorString(int ret)
{
    switch (ret) {
    case UNZ_OK:
    //case UNZ_EOF:
        return "UNZ_OK or UNZ_EOF";
    case UNZ_END_OF_LIST_OF_FILE:
        return "UNZ_END_OF_LIST_OF_FILE";
    case UNZ_PARAMERROR:
        return "UNZ_PARAMERROR";
    case UNZ_BADZIPFILE:
        return "UNZ_BADZIPFILE";
    case UNZ_INTERNALERROR:
        return "UNZ_INTERNALERROR";
    case UNZ_CRCERROR:
        return "UNZ_CRCERROR";
    case UNZ_ERRNO:
        return strerror(errno);
    default:
        return zlibErrorString(ret);
    }
}

std::string FileUtils::mzZipErrorString(int ret)
{
    switch (ret) {
    case ZIP_OK:
    //case ZIP_EOF:
        return "ZIP_OK or ZIP_EOF";
    case ZIP_PARAMERROR:
        return "ZIP_PARAMERROR";
    case ZIP_BADZIPFILE:
        return "ZIP_BADZIPFILE";
    case ZIP_INTERNALERROR:
        return "ZIP_INTERNALERROR";
    case ZIP_ERRNO:
        return strerror(errno);
    default:
        return zlibErrorString(ret);
    }
}

struct FileUtils::MzUnzCtx
{
    unzFile uf;
    zlib_filefunc64_def zFunc;
    ourbuffer_t buf;
#ifdef MINIZIP_WIN32
    std::wstring path;
#else
    std::string path;
#endif
};

struct FileUtils::MzZipCtx
{
    zipFile zf;
    zlib_filefunc64_def zFunc;
    ourbuffer_t buf;
#ifdef MINIZIP_WIN32
    std::wstring path;
#else
    std::string path;
#endif
};

unzFile FileUtils::mzCtxGetUnzFile(MzUnzCtx *ctx)
{
    return ctx->uf;
}

zipFile FileUtils::mzCtxGetZipFile(MzZipCtx *ctx)
{
    return ctx->zf;
}

FileUtils::MzUnzCtx * FileUtils::mzOpenInputFile(std::string path)
{
    MzUnzCtx *ctx = new(std::nothrow) MzUnzCtx();
    if (!ctx) {
        return nullptr;
    }

#if defined(MINIZIP_WIN32)
    fill_win32_filefunc64W(&ctx->buf.filefunc64);
    ctx->path = utf8::utf8ToUtf16(path);
#elif defined(MINIZIP_ANDROID)
    fill_android_filefunc64(&ctx->buf.filefunc64);
    ctx->path = std::move(path);
#else
    fill_fopen64_filefunc(&ctx->buf.filefunc64);
    ctx->path = std::move(path);
#endif

    fill_buffer_filefunc64(&ctx->zFunc, &ctx->buf);
    ctx->uf = unzOpen2_64(ctx->path.c_str(), &ctx->zFunc);
    if (!ctx->uf) {
        free(ctx);
        return nullptr;
    }

    return ctx;
}

FileUtils::MzZipCtx * FileUtils::mzOpenOutputFile(std::string path)
{
    MzZipCtx *ctx = new(std::nothrow) MzZipCtx();
    if (!ctx) {
        return nullptr;
    }

#if defined(MINIZIP_WIN32)
    fill_win32_filefunc64W(&ctx->buf.filefunc64);
    ctx->path = utf8::utf8ToUtf16(path);
#elif defined(MINIZIP_ANDROID)
    fill_android_filefunc64(&ctx->buf.filefunc64);
    ctx->path = std::move(path);
#else
    fill_fopen64_filefunc(&ctx->buf.filefunc64);
    ctx->path = std::move(path);
#endif

    fill_buffer_filefunc64(&ctx->zFunc, &ctx->buf);
    ctx->zf = zipOpen2_64(ctx->path.c_str(), 0, nullptr, &ctx->zFunc);
    if (!ctx->zf) {
        free(ctx);
        return nullptr;
    }

    return ctx;
}

int FileUtils::mzCloseInputFile(MzUnzCtx *ctx)
{
    int ret = unzClose(ctx->uf);
    delete ctx;
    return ret;
}

int FileUtils::mzCloseOutputFile(MzZipCtx *ctx)
{
    int ret = zipClose(ctx->zf, nullptr);
    delete ctx;
    return ret;
}

ErrorCode FileUtils::mzArchiveStats(const std::string &path,
                                    FileUtils::ArchiveStats *stats,
                                    std::vector<std::string> ignore)
{
    assert(stats != nullptr);

    MzUnzCtx *ctx = mzOpenInputFile(path);

    if (!ctx) {
        FLOGE("miniunz: Failed to open for reading: %s", path.c_str());
        return ErrorCode::ArchiveReadOpenError;
    }

    uint64_t count = 0;
    uint64_t totalSize = 0;
    std::string name;
    unz_file_info64 fi;
    memset(&fi, 0, sizeof(fi));

    int ret = unzGoToFirstFile(ctx->uf);
    if (ret != UNZ_OK) {
        FLOGE("miniunz: Failed to move to first file: %s",
              mzUnzErrorString(ret).c_str());
        mzCloseInputFile(ctx);
        return ErrorCode::ArchiveReadHeaderError;
    }

    do {
        if (!mzGetInfo(ctx->uf, &fi, &name)) {
            mzCloseInputFile(ctx);
            return ErrorCode::ArchiveReadHeaderError;
        }

        if (std::find(ignore.begin(), ignore.end(), name) == ignore.end()) {
            ++count;
            totalSize += fi.uncompressed_size;
        }
    } while ((ret = unzGoToNextFile(ctx->uf)) == UNZ_OK);

    if (ret != UNZ_END_OF_LIST_OF_FILE) {
        FLOGE("miniunz: Finished before EOF: %s",
              mzUnzErrorString(ret).c_str());
        mzCloseInputFile(ctx);
        return ErrorCode::ArchiveReadHeaderError;
    }

    mzCloseInputFile(ctx);

    stats->files = count;
    stats->totalSize = totalSize;

    return ErrorCode::NoError;
}

bool FileUtils::mzGetInfo(unzFile uf,
                          unz_file_info64 *fi,
                          std::string *filename)
{
    unz_file_info64 info;
    memset(&info, 0, sizeof(info));

    // First query to get filename size
    int ret = unzGetCurrentFileInfo64(
        uf,                     // file
        &info,                  // pfile_info
        nullptr,                // filename
        0,                      // filename_size
        nullptr,                // extrafield
        0,                      // extrafield_size
        nullptr,                // comment
        0                       // comment_size
    );

    if (ret != UNZ_OK) {
        FLOGE("miniunz: Failed to get inner file metadata: %s",
              mzUnzErrorString(ret).c_str());
        return false;
    }

    if (filename) {
        std::vector<char> buf(info.size_filename + 1);

        ret = unzGetCurrentFileInfo64(
            uf,             // file
            &info,          // pfile_info
            buf.data(),     // filename
            buf.size(),     // filename_size
            nullptr,        // extrafield
            0,              // extrafield_size
            nullptr,        // comment
            0               // comment_size
        );

        if (ret != UNZ_OK) {
            FLOGE("miniunz: Failed to get inner filename: %s",
                  mzUnzErrorString(ret).c_str());
            return false;
        }

        *filename = buf.data();
    }

    if (fi) {
        *fi = info;
    }

    return true;
}

bool FileUtils::mzCopyFileRaw(unzFile uf,
                              zipFile zf,
                              const std::string &name,
                              void (*cb)(uint64_t bytes, void *), void *userData)
{
    unz_file_info64 ufi;

    if (!mzGetInfo(uf, &ufi, nullptr)) {
        return false;
    }

    bool zip64 = ufi.uncompressed_size >= ((1ull << 32) - 1);

    zip_fileinfo zfi;
    memset(&zfi, 0, sizeof(zfi));

    zfi.dosDate = ufi.dosDate;
    zfi.tmz_date.tm_sec = ufi.tmu_date.tm_sec;
    zfi.tmz_date.tm_min = ufi.tmu_date.tm_min;
    zfi.tmz_date.tm_hour = ufi.tmu_date.tm_hour;
    zfi.tmz_date.tm_mday = ufi.tmu_date.tm_mday;
    zfi.tmz_date.tm_mon = ufi.tmu_date.tm_mon;
    zfi.tmz_date.tm_year = ufi.tmu_date.tm_year;
    zfi.internal_fa = ufi.internal_fa;
    zfi.external_fa = ufi.external_fa;

    int method;
    int level;

    // Open raw file in input zip
    int ret = unzOpenCurrentFile2(
        uf,                     // file
        &method,                // method
        &level,                 // level
        1                       // raw
    );
    if (ret != UNZ_OK) {
        FLOGE("miniunz: Failed to open inner file: %s",
              mzUnzErrorString(ret).c_str());
        return false;
    }

    // Open raw file in output zip
    ret = zipOpenNewFileInZip2_64(
        zf,             // file
        name.c_str(),   // filename
        &zfi,           // zip_fileinfo
        nullptr,        // extrafield_local
        0,              // size_extrafield_local
        nullptr,        // extrafield_global
        0,              // size_extrafield_global
        nullptr,        // comment
        method,         // method
        level,          // level
        1,              // raw
        zip64           // zip64
    );
    if (ret != ZIP_OK) {
        FLOGE("minizip: Failed to open inner file: %s",
              mzZipErrorString(ret).c_str());
        unzCloseCurrentFile(uf);
        return false;
    }

    uint64_t bytes = 0;

    // Exceeds Android's default stack size, unfortunately, so allocate
    // on the heap
    std::vector<unsigned char> buf(1024 * 1024); // 1MiB
    int bytes_read;
    double ratio;

    while ((bytes_read = unzReadCurrentFile(uf, buf.data(), buf.size())) > 0) {
        bytes += bytes_read;
        if (cb) {
            // Scale this to the uncompressed size for the purposes of a
            // progress bar
            ratio = (double) bytes / ufi.compressed_size;
            cb(ratio * ufi.uncompressed_size, userData);
        }

        ret = zipWriteInFileInZip(zf, buf.data(), bytes_read);
        if (ret != ZIP_OK) {
            FLOGE("minizip: Failed to write data to inner file: %s",
                  mzZipErrorString(ret).c_str());
            unzCloseCurrentFile(uf);
            zipCloseFileInZip(zf);
            return false;
        }
    }
    if (bytes_read != 0) {
        FLOGE("miniunz: Finished before reaching inner file's EOF: %s",
              mzUnzErrorString(bytes_read).c_str());
    }

    bool closeSuccess = true;

    ret = unzCloseCurrentFile(uf);
    if (ret != UNZ_OK) {
        FLOGE("miniunz: Failed to close inner file: %s",
              mzUnzErrorString(ret).c_str());
        closeSuccess = false;
    }
    ret = zipCloseFileInZipRaw64(zf, ufi.uncompressed_size, ufi.crc);
    if (ret != ZIP_OK) {
        FLOGE("minizip: Failed to close inner file: %s",
              mzZipErrorString(ret).c_str());
        closeSuccess = false;
    }

    return bytes_read == 0 && closeSuccess;
}

bool FileUtils::mzReadToMemory(unzFile uf,
                               std::vector<unsigned char> *output,
                               void (*cb)(uint64_t bytes, void *), void *userData)
{
    unz_file_info64 fi;

    if (!mzGetInfo(uf, &fi, nullptr)) {
        return false;
    }

    std::vector<unsigned char> data;
    data.reserve(fi.uncompressed_size);

    int ret = unzOpenCurrentFile(uf);
    if (ret != UNZ_OK) {
        FLOGE("miniunz: Failed to open inner file: %s",
              mzUnzErrorString(ret).c_str());
        return false;
    }

    int n;
    char buf[32768];

    while ((n = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
        if (cb) {
            cb(data.size() + n, userData);
        }

        data.insert(data.end(), buf, buf + n);
    }
    if (n != 0) {
        FLOGE("miniunz: Finished before reaching inner file's EOF: %s",
              mzUnzErrorString(n).c_str());
    }

    ret = unzCloseCurrentFile(uf);
    if (ret != UNZ_OK) {
        FLOGE("miniunz: Failed to close inner file: %s",
              mzUnzErrorString(ret).c_str());
        return false;
    }

    if (n != 0) {
        return false;
    }

    data.swap(*output);
    return true;
}

bool FileUtils::mzExtractFile(unzFile uf,
                              const std::string &directory)
{
    unz_file_info64 fi;
    std::string filename;

    if (!mzGetInfo(uf, &fi, &filename)) {
        return false;
    }

    std::string fullPath(directory);
    fullPath += "/";
    fullPath += filename;

    std::string parentPath = io::dirName(fullPath);
    if (!io::createDirectories(parentPath)) {
        FLOGW("%s: Failed to create directory: %s",
              parentPath.c_str(), io::lastErrorString().c_str());
    }

    io::File file;
    if (!file.open(fullPath, io::File::OpenWrite)) {
        FLOGE("%s: Failed to open for writing: %s",
              fullPath.c_str(), file.errorString().c_str());
        return false;
    }

    int ret = unzOpenCurrentFile(uf);
    if (ret != UNZ_OK) {
        FLOGE("miniunz: Failed to open inner file: %s",
              mzUnzErrorString(ret).c_str());
        return false;
    }

    int n;
    char buf[32768];
    uint64_t bytesWritten;

    while ((n = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
        if (!file.write(buf, n, &bytesWritten)) {
            FLOGE("%s: Failed to write file: %s",
                  fullPath.c_str(), file.errorString().c_str());
            unzCloseCurrentFile(uf);
            return false;
        }
    }
    if (n != 0) {
        FLOGE("miniunz: Finished before reaching inner file's EOF: %s",
              mzUnzErrorString(n).c_str());
    }

    ret = unzCloseCurrentFile(uf);
    if (ret != UNZ_OK) {
        FLOGE("miniunz: Failed to close inner file: %s",
              mzUnzErrorString(ret).c_str());
        return false;
    }

    return n == 0;
}

static bool mzGetFileTime(const std::string &filename,
                          tm_zip *tmzip, uLong *dostime)
{
    // Don't fail when building with -Werror
    (void) filename;
    (void) tmzip;
    (void) dostime;

#ifdef _WIN32
    FILETIME ftLocal;
    HANDLE hFind;
    WIN32_FIND_DATAW ff32;

    std::wstring wFilename = utf8::utf8ToUtf16(filename);
    hFind = FindFirstFileW(wFilename.c_str(), &ff32);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FileTimeToLocalFileTime(&ff32.ftLastWriteTime, &ftLocal);
        FileTimeToDosDateTime(&ftLocal,
                              ((LPWORD) dostime) + 1,
                              ((LPWORD) dostime) + 0);
        FindClose(hFind);
        return true;
    } else {
        FLOGE("%s: FindFirstFileW() failed: %s",
              filename.c_str(), win32ErrorToString(GetLastError()).c_str());
    }
#elif defined unix || defined __APPLE__ || defined __ANDROID__
    struct stat sb;
    struct tm t;

    if (stat(filename.c_str(), &sb) == 0) {
        time_t mtime = sb.st_mtime;
        if (!localtime_r(&mtime, &t)) {
            LOGE("localtime() failed");
            return false;
        }

        tmzip->tm_sec  = t.tm_sec;
        tmzip->tm_min  = t.tm_min;
        tmzip->tm_hour = t.tm_hour;
        tmzip->tm_mday = t.tm_mday;
        tmzip->tm_mon  = t.tm_mon ;
        tmzip->tm_year = t.tm_year;

        return true;
    } else {
        FLOGE("%s: stat() failed: %s", filename.c_str(), strerror(errno));
    }
#endif
    return false;
}

ErrorCode FileUtils::mzAddFile(zipFile zf,
                               const std::string &name,
                               const std::vector<unsigned char> &contents)
{
    // Obviously never true, but we'll keep it here just in case
    bool zip64 = (uint64_t) contents.size() >= ((1ull << 32) - 1);

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    int ret = zipOpenNewFileInZip2_64(
        zf,                     // file
        name.c_str(),           // filename
        &zi,                    // zip_fileinfo
        nullptr,                // extrafield_local
        0,                      // size_extrafield_local
        nullptr,                // extrafield_global
        0,                      // size_extrafield_global
        nullptr,                // comment
        Z_DEFLATED,             // method
        Z_DEFAULT_COMPRESSION,  // level
        0,                      // raw
        zip64                   // zip64
    );

    if (ret != ZIP_OK) {
        FLOGE("minizip: Failed to open inner file: %s",
              mzZipErrorString(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    // Write data to file
    ret = zipWriteInFileInZip(zf, contents.data(), contents.size());
    if (ret != ZIP_OK) {
        FLOGE("minizip: Failed to write inner file data: %s",
              mzZipErrorString(ret).c_str());
        zipCloseFileInZip(zf);

        return ErrorCode::ArchiveWriteDataError;
    }

    ret = zipCloseFileInZip(zf);
    if (ret != ZIP_OK) {
        FLOGE("minizip: Failed to close inner file: %s",
              mzZipErrorString(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    return ErrorCode::NoError;
}

ErrorCode FileUtils::mzAddFile(zipFile zf,
                               const std::string &name,
                               const std::string &path)
{
    // Copy file into archive
    io::File file;
    if (!file.open(path, io::File::OpenRead)) {
        FLOGE("%s: Failed to open for reading: %s",
              path.c_str(), file.errorString().c_str());
        return ErrorCode::FileOpenError;
    }

    uint64_t size;
    file.seek(0, io::File::SeekEnd);
    file.tell(&size);
    file.seek(0, io::File::SeekBegin);

    bool zip64 = size >= ((1ull << 32) - 1);

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    if (!mzGetFileTime(path, &zi.tmz_date, &zi.dosDate)) {
        FLOGE("%s: Failed to get modification time", path.c_str());
        return ErrorCode::FileOpenError;
    }

    int ret = zipOpenNewFileInZip2_64(
        zf,                     // file
        name.c_str(),           // filename
        &zi,                    // zip_fileinfo
        nullptr,                // extrafield_local
        0,                      // size_extrafield_local
        nullptr,                // extrafield_global
        0,                      // size_extrafield_global
        nullptr,                // comment
        Z_DEFLATED,             // method
        Z_DEFAULT_COMPRESSION,  // level
        0,                      // raw
        zip64                   // zip64
    );

    if (ret != ZIP_OK) {
        FLOGE("minizip: Failed to open inner file: %s",
              mzZipErrorString(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    // Write data to file
    char buf[32768];
    uint64_t bytesRead;

    while (file.read(buf, sizeof(buf), &bytesRead)) {
        ret = zipWriteInFileInZip(zf, buf, bytesRead);
        if (ret != ZIP_OK) {
            FLOGE("minizip: Failed to write inner file data: %s",
                  mzZipErrorString(ret).c_str());
            zipCloseFileInZip(zf);

            return ErrorCode::ArchiveWriteDataError;
        }
    }
    if (file.error() != io::File::ErrorEndOfFile) {
        FLOGE("%s: Failed to read data: %s",
              path.c_str(), file.errorString().c_str());
        zipCloseFileInZip(zf);

        return ErrorCode::FileReadError;
    }

    ret = zipCloseFileInZip(zf);
    if (ret != ZIP_OK) {
        FLOGE("minizip: Failed to close inner file: %s",
              mzZipErrorString(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    return ErrorCode::NoError;
}

}
