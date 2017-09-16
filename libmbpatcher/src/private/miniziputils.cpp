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

#include "mbpatcher/private/miniziputils.h"

#include <algorithm>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>

#ifdef __ANDROID__
#include <time.h>
#endif

#include "mbcommon/file/standard.h"
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
#include "mbpatcher/private/win32.h"
#else
#include <sys/stat.h>
#endif

#include "mbpatcher/private/fileutils.h"

#define LOG_TAG "mbpatcher/private/miniziputils"


namespace mb
{
namespace patcher
{

static std::string zlib_error_string(int ret)
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

std::string MinizipUtils::unz_error_string(int ret)
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
        return zlib_error_string(ret);
    }
}

std::string MinizipUtils::zip_error_string(int ret)
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
        return zlib_error_string(ret);
    }
}

struct MinizipUtils::UnzCtx
{
    unzFile uf;
    zlib_filefunc64_def z_func;
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
    zlib_filefunc64_def z_func;
    ourbuffer_t buf;
#ifdef MINIZIP_WIN32
    std::wstring path;
#else
    std::string path;
#endif
};

unzFile MinizipUtils::ctx_get_unz_file(UnzCtx *ctx)
{
    return ctx->uf;
}

zipFile MinizipUtils::ctx_get_zip_file(ZipCtx *ctx)
{
    return ctx->zf;
}

MinizipUtils::UnzCtx * MinizipUtils::open_input_file(std::string path)
{
    UnzCtx *ctx = new(std::nothrow) UnzCtx();
    if (!ctx) {
        return nullptr;
    }

#if defined(MINIZIP_WIN32)
    if (!utf8_to_wcs(ctx->path, path)) {
        delete ctx;
        return nullptr;
    }

    fill_win32_filefunc64W(&ctx->buf.filefunc64);
#elif defined(MINIZIP_ANDROID)
    fill_android_filefunc64(&ctx->buf.filefunc64);
    ctx->path = std::move(path);
#else
    fill_fopen64_filefunc(&ctx->buf.filefunc64);
    ctx->path = std::move(path);
#endif

    fill_buffer_filefunc64(&ctx->z_func, &ctx->buf);
    ctx->uf = unzOpen2_64(ctx->path.c_str(), &ctx->z_func);
    if (!ctx->uf) {
        delete ctx;
        return nullptr;
    }

    return ctx;
}

MinizipUtils::ZipCtx * MinizipUtils::open_output_file(std::string path)
{
    ZipCtx *ctx = new(std::nothrow) ZipCtx();
    if (!ctx) {
        return nullptr;
    }

#if defined(MINIZIP_WIN32)
    if (!utf8_to_wcs(ctx->path, path)) {
        delete ctx;
        return nullptr;
    }

    fill_win32_filefunc64W(&ctx->buf.filefunc64);
#elif defined(MINIZIP_ANDROID)
    fill_android_filefunc64(&ctx->buf.filefunc64);
    ctx->path = std::move(path);
#else
    fill_fopen64_filefunc(&ctx->buf.filefunc64);
    ctx->path = std::move(path);
#endif

    fill_buffer_filefunc64(&ctx->z_func, &ctx->buf);
    ctx->zf = zipOpen2_64(ctx->path.c_str(), 0, nullptr, &ctx->z_func);
    if (!ctx->zf) {
        delete ctx;
        return nullptr;
    }

    return ctx;
}

int MinizipUtils::close_input_file(UnzCtx *ctx)
{
    int ret = unzClose(ctx->uf);
    delete ctx;
    return ret;
}

int MinizipUtils::close_output_file(ZipCtx *ctx)
{
    int ret = zipClose(ctx->zf, nullptr);
    delete ctx;
    return ret;
}

ErrorCode MinizipUtils::archive_stats(const std::string &path,
                                      MinizipUtils::ArchiveStats *stats,
                                      std::vector<std::string> ignore)
{
    assert(stats != nullptr);

    UnzCtx *ctx = open_input_file(path);

    if (!ctx) {
        LOGE("miniunz: Failed to open for reading: %s", path.c_str());
        return ErrorCode::ArchiveReadOpenError;
    }

    uint64_t count = 0;
    uint64_t total_size = 0;
    std::string name;
    unz_file_info64 fi;
    memset(&fi, 0, sizeof(fi));

    int ret = unzGoToFirstFile(ctx->uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to move to first file: %s",
             unz_error_string(ret).c_str());
        close_input_file(ctx);
        return ErrorCode::ArchiveReadHeaderError;
    }

    do {
        if (!get_info(ctx->uf, &fi, &name)) {
            close_input_file(ctx);
            return ErrorCode::ArchiveReadHeaderError;
        }

        if (std::find(ignore.begin(), ignore.end(), name) == ignore.end()) {
            ++count;
            total_size += fi.uncompressed_size;
        }
    } while ((ret = unzGoToNextFile(ctx->uf)) == UNZ_OK);

    if (ret != UNZ_END_OF_LIST_OF_FILE) {
        LOGE("miniunz: Finished before EOF: %s",
             unz_error_string(ret).c_str());
        close_input_file(ctx);
        return ErrorCode::ArchiveReadHeaderError;
    }

    close_input_file(ctx);

    stats->files = count;
    stats->total_size = total_size;

    return ErrorCode::NoError;
}

bool MinizipUtils::get_info(unzFile uf,
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
             unz_error_string(ret).c_str());
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
                 unz_error_string(ret).c_str());
            return false;
        }

        *filename = buf.data();
    }

    if (fi) {
        *fi = info;
    }

    return true;
}

bool MinizipUtils::copy_file_raw(unzFile uf,
                                 zipFile zf,
                                 const std::string &name,
                                 void (*cb)(uint64_t bytes, void *),
                                 void *userData)
{
    unz_file_info64 ufi;

    if (!get_info(uf, &ufi, nullptr)) {
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
             unz_error_string(ret).c_str());
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
             zip_error_string(ret).c_str());
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
                 zip_error_string(ret).c_str());
            unzCloseCurrentFile(uf);
            zipCloseFileInZip(zf);
            return false;
        }
    }
    if (bytes_read != 0) {
        LOGE("miniunz: Finished before reaching inner file's EOF: %s",
             unz_error_string(bytes_read).c_str());
    }

    bool close_success = true;

    ret = unzCloseCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to close inner file: %s",
             unz_error_string(ret).c_str());
        close_success = false;
    }
    ret = zipCloseFileInZipRaw64(zf, ufi.uncompressed_size, ufi.crc);
    if (ret != ZIP_OK) {
        LOGE("minizip: Failed to close inner file: %s",
             zip_error_string(ret).c_str());
        close_success = false;
    }

    return bytes_read == 0 && close_success;
}

bool MinizipUtils::read_to_memory(unzFile uf,
                                  std::vector<unsigned char> *output,
                                  void (*cb)(uint64_t bytes, void *),
                                  void *userData)
{
    unz_file_info64 fi;

    if (!get_info(uf, &fi, nullptr)) {
        return false;
    }

    std::vector<unsigned char> data;
    data.reserve(fi.uncompressed_size);

    int ret = unzOpenCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to open inner file: %s",
             unz_error_string(ret).c_str());
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
             unz_error_string(n).c_str());
    }

    ret = unzCloseCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to close inner file: %s",
             unz_error_string(ret).c_str());
        return false;
    }

    if (n != 0) {
        return false;
    }

    data.swap(*output);
    return true;
}

bool MinizipUtils::extract_file(unzFile uf, const std::string &directory)
{
    unz_file_info64 fi;
    std::string filename;

    if (!get_info(uf, &fi, &filename)) {
        return false;
    }

    std::string full_path(directory);
#ifdef _WIN32
    full_path += "\\";
#else
    full_path += "/";
#endif
    full_path += filename;

    std::string parent_path = io::dirName(full_path);
    if (!io::createDirectories(parent_path)) {
        LOGW("%s: Failed to create directory: %s",
             parent_path.c_str(), io::lastErrorString().c_str());
    }

    StandardFile file;
    int ret;

    auto error = FileUtils::open_file(file, full_path,
                                      FileOpenMode::WRITE_ONLY);
    if (error != ErrorCode::NoError) {
        LOGE("%s: Failed to open for writing: %s",
             full_path.c_str(), file.error_string().c_str());
        return false;
    }

    ret = unzOpenCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to open inner file: %s",
             unz_error_string(ret).c_str());
        return false;
    }

    int n;
    char buf[32768];
    size_t bytes_written;

    while ((n = unzReadCurrentFile(uf, buf, sizeof(buf))) > 0) {
        if (!file.write(buf, n, bytes_written)) {
            LOGE("%s: Failed to write file: %s",
                 full_path.c_str(), file.error_string().c_str());
            unzCloseCurrentFile(uf);
            return false;
        }
    }
    if (n != 0) {
        LOGE("miniunz: Finished before reaching inner file's EOF: %s",
             unz_error_string(n).c_str());
    }

    ret = unzCloseCurrentFile(uf);
    if (ret != UNZ_OK) {
        LOGE("miniunz: Failed to close inner file: %s",
             unz_error_string(ret).c_str());
        return false;
    }

    if (!file.close()) {
        LOGE("%s: Failed to close file: %s",
             full_path.c_str(), file.error_string().c_str());
        return false;
    }

    return n == 0;
}

static bool get_file_time(const std::string &filename, uint32_t *dostime)
{
    // Don't fail when building with -Werror
    (void) filename;
    (void) dostime;

#ifdef _WIN32
    FILETIME ft_local;
    HANDLE h_find;
    WIN32_FIND_DATAW ff32;

    std::wstring w_filename;
    if (!utf8_to_wcs(w_filename, filename)) {
        return false;
    }

    h_find = FindFirstFileW(w_filename.c_str(), &ff32);

    if (h_find != INVALID_HANDLE_VALUE)
    {
        FileTimeToLocalFileTime(&ff32.ftLastWriteTime, &ft_local);
        FileTimeToDosDateTime(&ft_local,
                              ((LPWORD) dostime) + 1,
                              ((LPWORD) dostime) + 0);
        FindClose(h_find);
        return true;
    } else {
        LOGE("%s: FindFirstFileW() failed: %s",
             filename.c_str(), win32_error_to_string(GetLastError()).c_str());
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

ErrorCode MinizipUtils::add_file(zipFile zf,
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
             zip_error_string(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    // Write data to file
    ret = zipWriteInFileInZip(zf, contents.data(), contents.size());
    if (ret != ZIP_OK) {
        LOGE("minizip: Failed to write inner file data: %s",
             zip_error_string(ret).c_str());
        zipCloseFileInZip(zf);

        return ErrorCode::ArchiveWriteDataError;
    }

    ret = zipCloseFileInZip(zf);
    if (ret != ZIP_OK) {
        LOGE("minizip: Failed to close inner file: %s",
             zip_error_string(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    return ErrorCode::NoError;
}

ErrorCode MinizipUtils::add_file(zipFile zf,
                                 const std::string &name,
                                 const std::string &path)
{
    // Copy file into archive
    StandardFile file;
    bool file_ret;
    int ret;

    auto error = FileUtils::open_file(file, path,
                                      FileOpenMode::READ_ONLY);
    if (error != ErrorCode::NoError) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), file.error_string().c_str());
        return ErrorCode::FileOpenError;
    }

    uint64_t size;
    if (!file.seek(0, SEEK_END, &size) || !file.seek(0, SEEK_SET, nullptr)) {
        LOGE("%s: Failed to seek file: %s",
             path.c_str(), file.error_string().c_str());
        return ErrorCode::FileSeekError;
    }

    bool zip64 = size >= ((1ull << 32) - 1);

    zip_fileinfo zi;
    memset(&zi, 0, sizeof(zi));

    if (!get_file_time(path, &zi.dos_date)) {
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
             zip_error_string(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    // Write data to file
    char buf[32768];
    size_t bytes_read;

    while ((file_ret = file.read(buf, sizeof(buf), bytes_read))
            && bytes_read > 0) {
        ret = zipWriteInFileInZip(zf, buf, bytes_read);
        if (ret != ZIP_OK) {
            LOGE("minizip: Failed to write inner file data: %s",
                 zip_error_string(ret).c_str());
            zipCloseFileInZip(zf);

            return ErrorCode::ArchiveWriteDataError;
        }
    }
    if (!file_ret) {
        LOGE("%s: Failed to read data: %s",
             path.c_str(), file.error_string().c_str());
        zipCloseFileInZip(zf);

        return ErrorCode::FileReadError;
    }

    ret = zipCloseFileInZip(zf);
    if (ret != ZIP_OK) {
        LOGE("minizip: Failed to close inner file: %s",
             zip_error_string(ret).c_str());

        return ErrorCode::ArchiveWriteDataError;
    }

    return ErrorCode::NoError;
}

}
}
