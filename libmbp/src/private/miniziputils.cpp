/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbp/private/miniziputils.h"

#include <algorithm>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>

#ifdef __ANDROID__
#include <time.h>
#endif

#include "mbcommon/file/filename.h"
#include "mbcommon/locale.h"

#include "mblog/logging.h"

#include "mbpio/directory.h"
#include "mbpio/error.h"
#include "mbpio/path.h"

#include "minizip/ioapi_buf.h"
#include "minizip/minishared.h"
#if defined(_WIN32)
#  define MINIZIP_WIN32
#  include "minizip/iowin32.h"
#  include <wchar.h>
#elif defined(__ANDROID__)
#  define MINIZIP_ANDROID
#  include "minizip/ioandroid.h"
#endif

#ifdef _WIN32
#include "mbp/private/win32.h"
#else
#include <sys/stat.h>
#endif

#include "mbp/private/fileutils.h"


typedef std::unique_ptr<MbFile, decltype(mb_file_free) *> ScopedMbFile;

namespace mbp
{

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

std::string MinizipUtils::unzErrorString(int ret)
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

std::string MinizipUtils::zipErrorString(int ret)
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

struct MinizipUtils::UnzCtx
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

struct MinizipUtils::ZipCtx
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

unzFile MinizipUtils::ctxGetUnzFile(UnzCtx *ctx)
{
    return ctx->uf;
}

zipFile MinizipUtils::ctxGetZipFile(ZipCtx *ctx)
{
    return ctx->zf;
}

MinizipUtils::UnzCtx * MinizipUtils::openInputFile(std::string path)
{
    UnzCtx *ctx = new(std::nothrow) UnzCtx();
    if (!ctx) {
        return nullptr;
    }

#if defined(MINIZIP_WIN32)
    wchar_t *wPath = mb::utf8_to_wcs(path.c_str());
    if (!wPath) {
        free(ctx);
        return nullptr;
    }

    fill_win32_filefunc64W(&ctx->buf.filefunc64);
    ctx->path = wPath;
    free(wPath);
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

MinizipUtils::ZipCtx * MinizipUtils::openOutputFile(std::string path)
{
    ZipCtx *ctx = new(std::nothrow) ZipCtx();
    if (!ctx) {
        return nullptr;
    }

#if defined(MINIZIP_WIN32)
    wchar_t *wPath = mb::utf8_to_wcs(path.c_str());
    if (!wPath) {
        free(ctx);
        return nullptr;
    }

    fill_win32_filefunc64W(&ctx->buf.filefunc64);
    ctx->path = wPath;
    free(wPath);
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

int MinizipUtils::closeInputFile(UnzCtx *ctx)
{
    int ret = unzClose(ctx->uf);
    delete ctx;
    return ret;
}

int MinizipUtils::closeOutputFile(ZipCtx *ctx)
{
    int ret = zipClose(ctx->zf, nullptr);
    delete ctx;
    return ret;
}

ErrorCode MinizipUtils::archiveStats(const std::string &path,
                                     MinizipUtils::ArchiveStats *stats,
                                     std::vector<std::string> ignore)
{
    assert(stats != nullptr);

    UnzCtx *ctx = openInputFile(path);

    if (!ctx) {
        LOGE("miniunz: Failed to open for reading: %s", path.c_str());
        return ErrorCode::ArchiveReadOpenError;
    }

    uint64_t count = 0;
    uint64_t totalSize = 0;
    std::string name;
    unz_file_info64 fi;
    memset(&fi, 0, sizeof(fi));

    int ret = unzGoToFirstFile(ctx->uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to move to first file: %s",
             unzErrorString(ret).c_str());
        closeInputFile(ctx);
        return ErrorCode::ArchiveReadHeaderError;
    }

    do {
        if (!getInfo(ctx->uf, &fi, &name)) {
            closeInputFile(ctx);
            return ErrorCode::ArchiveReadHeaderError;
        }

        if (std::find(ignore.begin(), ignore.end(), name) == ignore.end()) {
            ++count;
            totalSize += fi.uncompressed_size;
        }
    } while ((ret = unzGoToNextFile(ctx->uf)) == UNZ_OK);

    if (ret != UNZ_END_OF_LIST_OF_FILE) {
        LOGE("miniunz: Finished before EOF: %s",
             unzErrorString(ret).c_str());
        closeInputFile(ctx);
        return ErrorCode::ArchiveReadHeaderError;
    }

    closeInputFile(ctx);

    stats->files = count;
    stats->totalSize = totalSize;

    return ErrorCode::NoError;
}

bool MinizipUtils::getInfo(unzFile uf,
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
        LOGE("miniunz: Failed to get inner file metadata: %s",
             unzErrorString(ret).c_str());
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
            LOGE("miniunz: Failed to get inner filename: %s",
                 unzErrorString(ret).c_str());
            return false;
        }

        *filename = buf.data();
    }

    if (fi) {
        *fi = info;
    }

    return true;
}

bool MinizipUtils::copyFileRaw(unzFile uf,
                               zipFile zf,
                               const std::string &name,
                               void (*cb)(uint64_t bytes, void *), void *userData)
{
    unz_file_info64 ufi;

    if (!getInfo(uf, &ufi, nullptr)) {
        return false;
    }

    bool zip64 = ufi.uncompressed_size >= ((1ull << 32) - 1);

    zip_fileinfo zfi;
    memset(&zfi, 0, sizeof(zfi));

    zfi.dos_date = ufi.dos_date;
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
        LOGE("miniunz: Failed to open inner file: %s",
             unzErrorString(ret).c_str());
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
        LOGE("minizip: Failed to open inner file: %s",
             zipErrorString(ret).c_str());
        unzCloseCurrentFile(uf);
        return false;
    }

    uint64_t bytes = 0;

    // minizip no longer supports buffers larger than UINT16_MAX
    char buf[UINT16_MAX];
    int bytes_read;
    double ratio;

    while ((bytes_read = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
        bytes += bytes_read;
        if (cb) {
            // Scale this to the uncompressed size for the purposes of a
            // progress bar
            ratio = (double) bytes / ufi.compressed_size;
            cb(ratio * ufi.uncompressed_size, userData);
        }

        ret = zipWriteInFileInZip(zf, buf, bytes_read);
        if (ret != ZIP_OK) {
            LOGE("minizip: Failed to write data to inner file: %s",
                 zipErrorString(ret).c_str());
            unzCloseCurrentFile(uf);
            zipCloseFileInZip(zf);
            return false;
        }
    }
    if (bytes_read != 0) {
        LOGE("miniunz: Finished before reaching inner file's EOF: %s",
             unzErrorString(bytes_read).c_str());
    }

    bool closeSuccess = true;

    ret = unzCloseCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to close inner file: %s",
             unzErrorString(ret).c_str());
        closeSuccess = false;
    }
    ret = zipCloseFileInZipRaw64(zf, ufi.uncompressed_size, ufi.crc);
    if (ret != ZIP_OK) {
        LOGE("minizip: Failed to close inner file: %s",
             zipErrorString(ret).c_str());
        closeSuccess = false;
    }

    return bytes_read == 0 && closeSuccess;
}

bool MinizipUtils::readToMemory(unzFile uf,
                                std::vector<unsigned char> *output,
                                void (*cb)(uint64_t bytes, void *), void *userData)
{
    unz_file_info64 fi;

    if (!getInfo(uf, &fi, nullptr)) {
        return false;
    }

    std::vector<unsigned char> data;
    data.reserve(fi.uncompressed_size);

    int ret = unzOpenCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to open inner file: %s",
             unzErrorString(ret).c_str());
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
        LOGE("miniunz: Finished before reaching inner file's EOF: %s",
             unzErrorString(n).c_str());
    }

    ret = unzCloseCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to close inner file: %s",
             unzErrorString(ret).c_str());
        return false;
    }

    if (n != 0) {
        return false;
    }

    data.swap(*output);
    return true;
}

bool MinizipUtils::extractFile(unzFile uf,
                               const std::string &directory)
{
    unz_file_info64 fi;
    std::string filename;

    if (!getInfo(uf, &fi, &filename)) {
        return false;
    }

    std::string fullPath(directory);
#ifdef _WIN32
    fullPath += "\\";
#else
    fullPath += "/";
#endif
    fullPath += filename;

    std::string parentPath = io::dirName(fullPath);
    if (!io::createDirectories(parentPath)) {
        LOGW("%s: Failed to create directory: %s",
             parentPath.c_str(), io::lastErrorString().c_str());
    }

    ScopedMbFile file{mb_file_new(), &mb_file_free};
    int ret;

    auto error = FileUtils::openFile(file.get(), fullPath,
                                     MB_FILE_OPEN_WRITE_ONLY);
    if (error != ErrorCode::NoError) {
        LOGE("%s: Failed to open for writing: %s",
             fullPath.c_str(), mb_file_error_string(file.get()));
        return false;
    }

    ret = unzOpenCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to open inner file: %s",
             unzErrorString(ret).c_str());
        return false;
    }

    int n;
    char buf[32768];
    size_t bytesWritten;

    while ((n = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
        if (mb_file_write(file.get(), buf, n, &bytesWritten) != MB_FILE_OK) {
            LOGE("%s: Failed to write file: %s",
                 fullPath.c_str(), mb_file_error_string(file.get()));
            unzCloseCurrentFile(uf);
            return false;
        }
    }
    if (n != 0) {
        LOGE("miniunz: Finished before reaching inner file's EOF: %s",
             unzErrorString(n).c_str());
    }

    ret = unzCloseCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to close inner file: %s",
             unzErrorString(ret).c_str());
        return false;
    }

    if (mb_file_close(file.get()) != MB_FILE_OK) {
        LOGE("%s: Failed to close file: %s",
             fullPath.c_str(), mb_file_error_string(file.get()));
        return false;
    }

    return n == 0;
}

static bool getFileTime(const std::string &filename, uint32_t *dostime)
{
    // Don't fail when building with -Werror
    (void) filename;
    (void) dostime;

#ifdef _WIN32
    FILETIME ftLocal;
    HANDLE hFind;
    WIN32_FIND_DATAW ff32;

    wchar_t *wFilename = mb::utf8_to_wcs(filename.c_str());
    if (!wFilename) {
        return false;
    }

    hFind = FindFirstFileW(wFilename, &ff32);
    free(wFilename);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        FileTimeToLocalFileTime(&ff32.ftLastWriteTime, &ftLocal);
        FileTimeToDosDateTime(&ftLocal,
                              ((LPWORD) dostime) + 1,
                              ((LPWORD) dostime) + 0);
        FindClose(hFind);
        return true;
    } else {
        LOGE("%s: FindFirstFileW() failed: %s",
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

        *dostime = tm_to_dosdate(&t);

        return true;
    } else {
        LOGE("%s: stat() failed: %s", filename.c_str(), strerror(errno));
    }
#endif
    return false;
}

ErrorCode MinizipUtils::addFile(zipFile zf,
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
        LOGE("minizip: Failed to open inner file: %s",
             zipErrorString(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    // Write data to file
    ret = zipWriteInFileInZip(zf, contents.data(), contents.size());
    if (ret != ZIP_OK) {
        LOGE("minizip: Failed to write inner file data: %s",
             zipErrorString(ret).c_str());
        zipCloseFileInZip(zf);

        return ErrorCode::ArchiveWriteDataError;
    }

    ret = zipCloseFileInZip(zf);
    if (ret != ZIP_OK) {
        LOGE("minizip: Failed to close inner file: %s",
             zipErrorString(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    return ErrorCode::NoError;
}

ErrorCode MinizipUtils::addFile(zipFile zf,
                                const std::string &name,
                                const std::string &path)
{
    // Copy file into archive
    ScopedMbFile file{mb_file_new(), &mb_file_free};
    int ret;

    auto error = FileUtils::openFile(file.get(), path,
                                     MB_FILE_OPEN_READ_ONLY);
    if (error != ErrorCode::NoError) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), mb_file_error_string(file.get()));
        return ErrorCode::FileOpenError;
    }

    uint64_t size;
    if (mb_file_seek(file.get(), 0, SEEK_END, &size) != MB_FILE_OK
            || mb_file_seek(file.get(), 0, SEEK_SET, nullptr) != MB_FILE_OK) {
        LOGE("%s: Failed to seek file: %s",
             path.c_str(), mb_file_error_string(file.get()));
        return ErrorCode::FileSeekError;
    }

    bool zip64 = size >= ((1ull << 32) - 1);

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    if (!getFileTime(path, &zi.dos_date)) {
        LOGE("%s: Failed to get modification time", path.c_str());
        return ErrorCode::FileOpenError;
    }

    ret = zipOpenNewFileInZip2_64(
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
        LOGE("minizip: Failed to open inner file: %s",
             zipErrorString(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    // Write data to file
    char buf[32768];
    size_t bytesRead;

    while ((ret = mb_file_read(file.get(), buf, sizeof(buf), &bytesRead))
            == MB_FILE_OK && bytesRead > 0) {
        ret = zipWriteInFileInZip(zf, buf, bytesRead);
        if (ret != ZIP_OK) {
            LOGE("minizip: Failed to write inner file data: %s",
                 zipErrorString(ret).c_str());
            zipCloseFileInZip(zf);

            return ErrorCode::ArchiveWriteDataError;
        }
    }
    if (ret != MB_FILE_OK) {
        LOGE("%s: Failed to read data: %s",
             path.c_str(), mb_file_error_string(file.get()));
        zipCloseFileInZip(zf);

        return ErrorCode::FileReadError;
    }

    ret = zipCloseFileInZip(zf);
    if (ret != ZIP_OK) {
        LOGE("minizip: Failed to close inner file: %s",
             zipErrorString(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    return ErrorCode::NoError;
}

}
