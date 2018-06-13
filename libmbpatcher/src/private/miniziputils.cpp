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

#include "mbpatcher/private/miniziputils.h"

#include <algorithm>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>

#ifdef __ANDROID__
#include <time.h>
#endif

#include "mbcommon/error_code.h"
#include "mbcommon/file/standard.h"
#include "mbcommon/finally.h"
#include "mbcommon/locale.h"

#include "mblog/logging.h"

#include "mbpio/directory.h"
#include "mbpio/path.h"

#include "mz_os.h"
#include "mz_strm_buf.h"
#if defined(_WIN32)
#  include "mz_strm_win32.h"
#elif defined(__ANDROID__)
#  include "mz_strm_android.h"
#else
#  include "mz_strm_posix.h"
#endif

#ifndef _WIN32
#  include <sys/stat.h>
#endif

#include "mbpatcher/private/fileutils.h"

#define LOG_TAG "mbpatcher/private/miniziputils"


namespace mb::patcher
{

struct ZipCtx
{
    void *stream;
    void *buf_stream;
    void *handle;
};

void * MinizipUtils::ctx_get_zip_handle(ZipCtx *ctx)
{
    return ctx->handle;
}

ZipCtx * MinizipUtils::open_zip_file(std::string path, ZipOpenMode mode)
{
    ZipCtx *ctx = new(std::nothrow) ZipCtx();
    if (!ctx) {
        return nullptr;
    }

    auto delete_ctx = finally([&] {
        delete ctx;
    });

#if defined(_WIN32)
    auto ret = mz_stream_win32_create(&ctx->stream);
#elif defined(__ANDROID__)
    auto ret = mz_stream_android_create(&ctx->stream);
#else
    auto ret = mz_stream_posix_create(&ctx->stream);
#endif
    if (!ret) {
        return nullptr;
    }

    auto destroy_stream = finally([&] {
        mz_stream_delete(&ctx->stream);
    });

    ret = mz_stream_buffered_create(&ctx->buf_stream);
    if (!ret) {
        return nullptr;
    }

    auto destroy_buf_stream = finally([&] {
        mz_stream_delete(&ctx->buf_stream);
    });

    if (mz_stream_set_base(ctx->buf_stream, ctx->stream) != MZ_OK) {
        return nullptr;
    }

    int file_mode;
    int zip_mode;
    switch (mode) {
    case ZipOpenMode::Read:
        file_mode = zip_mode = MZ_OPEN_MODE_READ;
        break;
    case ZipOpenMode::Write:
        file_mode = zip_mode = MZ_OPEN_MODE_READWRITE;
        file_mode |= MZ_OPEN_MODE_CREATE;
        break;
    default:
        return nullptr;
    }

    if (mz_stream_open(ctx->buf_stream, path.c_str(), file_mode) != MZ_OK) {
        return nullptr;
    }

    auto close_stream = finally([&] {
        mz_stream_close(ctx->buf_stream);
    });

    ctx->handle = mz_zip_open(ctx->buf_stream, zip_mode);
    if (!ctx->handle) {
        return nullptr;
    }

    close_stream.dismiss();
    destroy_buf_stream.dismiss();
    destroy_stream.dismiss();
    delete_ctx.dismiss();

    return ctx;
}

int MinizipUtils::close_zip_file(ZipCtx *ctx)
{
    int ret = mz_zip_close(ctx->handle);
    mz_stream_close(ctx->buf_stream);
    mz_stream_delete(&ctx->buf_stream);
    mz_stream_delete(&ctx->stream);
    delete ctx;
    return ret;
}

ErrorCode MinizipUtils::archive_stats(const std::string &path,
                                      MinizipUtils::ArchiveStats &stats,
                                      const std::vector<std::string> &ignore)
{
    ZipCtx *ctx = open_zip_file(path, ZipOpenMode::Read);
    if (!ctx) {
        LOGE("minizip: Failed to open for reading: %s", path.c_str());
        return ErrorCode::ArchiveReadOpenError;
    }

    auto close_ctx = finally([&] {
        close_zip_file(ctx);
    });

    uint64_t count = 0;
    uint64_t total_size = 0;
    mz_zip_file *file_info;

    int ret = mz_zip_goto_first_entry(ctx->handle);
    if (ret != MZ_OK && ret != MZ_END_OF_LIST) {
        LOGE("minizip: Failed to move to first file: %d", ret);
        return ErrorCode::ArchiveReadHeaderError;
    }

    if (ret != MZ_END_OF_LIST) {
        do {
            ret = mz_zip_entry_get_info(ctx->handle, &file_info);
            if (ret != MZ_OK) {
                LOGE("minizip: Failed to get inner file metadata: %d", ret);
                return ErrorCode::ArchiveReadHeaderError;
            }

            std::string filename{file_info->filename, file_info->filename_size};
            if (std::find(ignore.begin(), ignore.end(), filename) == ignore.end()) {
                ++count;
                total_size += file_info->uncompressed_size;
            }
        } while ((ret = mz_zip_goto_next_entry(ctx->handle)) == MZ_OK);

        if (ret != MZ_END_OF_LIST) {
            LOGE("minizip: Finished before EOF: %d", ret);
            return ErrorCode::ArchiveReadHeaderError;
        }
    }

    stats.files = count;
    stats.total_size = total_size;

    return ErrorCode::NoError;
}

bool MinizipUtils::copy_file_raw(void *source_handle,
                                 void *target_handle,
                                 const std::string &name,
                                 const std::function<void(uint64_t bytes)> &cb)
{
    mz_zip_file *file_info;

    int ret = mz_zip_entry_get_info(source_handle, &file_info);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to get inner file metadata: %d", ret);
        return false;
    }

    // Open raw file in input zip
    ret = mz_zip_entry_read_open(source_handle, 1, nullptr);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to open inner file: %d", ret);
        return false;
    }

    auto close_inner_read = finally([&] {
        mz_zip_entry_close(source_handle);
    });

    mz_zip_file target_file_info = *file_info;
    target_file_info.filename = name.c_str();
    target_file_info.filename_size = static_cast<uint16_t>(name.size());

    // Open raw file in output zip
    ret = mz_zip_entry_write_open(target_handle, &target_file_info, 0, nullptr);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to open inner file: %d", ret);
        return false;
    }

    auto close_inner_write = finally([&] {
        mz_zip_entry_close(target_handle);
    });

    uint64_t bytes = 0;

    // minizip no longer supports buffers larger than UINT16_MAX
    char buf[UINT16_MAX];
    int n_read;
    double ratio;

    while ((n_read = mz_zip_entry_read(source_handle, buf, sizeof(buf))) > 0) {
        bytes += static_cast<uint64_t>(n_read);
        if (cb) {
            // Scale this to the uncompressed size for the purposes of a
            // progress bar
            ratio = static_cast<double>(bytes)
                    / static_cast<double>(file_info->compressed_size);
            cb(static_cast<uint64_t>(
                    ratio * static_cast<double>(file_info->uncompressed_size)));
        }

        int n_written = mz_zip_entry_write(
                target_handle, buf, static_cast<uint32_t>(n_read));
        if (n_written != n_read) {
            LOGE("minizip: Failed to write data to inner file");
            return false;
        }
    }
    if (n_read != 0) {
        LOGE("minizip: Failed to read inner file");
        return false;
    }

    close_inner_read.dismiss();

    ret = mz_zip_entry_close(source_handle);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to close inner file: %d", ret);
        return false;
    }

    close_inner_write.dismiss();

    ret = mz_zip_entry_close_raw(target_handle, file_info->uncompressed_size,
                                 file_info->crc);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to close inner file: %d", ret);
        return false;
    }

    return true;
}

bool MinizipUtils::read_to_memory(void *handle, std::string &output,
                                  const std::function<void(uint64_t bytes)> &cb)
{
    mz_zip_file *file_info;

    int ret = mz_zip_entry_get_info(handle, &file_info);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to get inner file metadata: %d", ret);
        return false;
    }

    std::string data;
    data.reserve(static_cast<size_t>(file_info->uncompressed_size));

    ret = mz_zip_entry_read_open(handle, 0, nullptr);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to open inner file: %d", ret);
        return false;
    }

    auto close_inner_read = finally([&] {
        mz_zip_entry_close(handle);
    });

    int n;
    char buf[UINT16_MAX];

    while ((n = mz_zip_entry_read(handle, buf, sizeof(buf))) > 0) {
        if (cb) {
            cb(static_cast<uint64_t>(data.size()) + static_cast<uint64_t>(n));
        }

        data.insert(data.end(), buf, buf + n);
    }
    if (n != 0) {
        LOGE("minizip: Failed to read inner file");
        return false;
    }

    close_inner_read.dismiss();

    ret = mz_zip_entry_close(handle);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to close inner file: %d", ret);
        return false;
    }

    data.swap(output);
    return true;
}

bool MinizipUtils::extract_file(void *handle, const std::string &directory)
{
    mz_zip_file *file_info;

    int ret = mz_zip_entry_get_info(handle, &file_info);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to get inner file metadata: %d", ret);
        return false;
    }

    std::string full_path(directory);
#ifdef _WIN32
    full_path += "\\";
#else
    full_path += "/";
#endif
    full_path += std::string{file_info->filename, file_info->filename_size};

    std::string parent_path = io::dir_name(full_path);
    if (auto r = io::create_directories(parent_path); !r) {
        LOGW("%s: Failed to create directory: %s",
             parent_path.c_str(), r.error().message().c_str());
    }

    StandardFile file;

    auto open_ret = FileUtils::open_file(file, full_path,
                                         FileOpenMode::WriteOnly);
    if (!open_ret) {
        LOGE("%s: Failed to open for writing: %s",
             full_path.c_str(), open_ret.error().message().c_str());
        return false;
    }

    ret = mz_zip_entry_read_open(handle, 0, nullptr);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to open inner file: %d", ret);
        return false;
    }

    auto close_inner_read = finally([&] {
        mz_zip_entry_close(handle);
    });

    int n;
    char buf[UINT16_MAX];

    while ((n = mz_zip_entry_read(handle, buf, sizeof(buf))) > 0) {
        auto bytes_written = file.write(buf, static_cast<size_t>(n));
        if (!bytes_written) {
            LOGE("%s: Failed to write file: %s",
                 full_path.c_str(), bytes_written.error().message().c_str());
            return false;
        }
    }
    if (n != 0) {
        LOGE("minizip: Failed to read inner file");
        return false;
    }

    close_inner_read.dismiss();

    ret = mz_zip_entry_close(handle);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to close inner file: %d", ret);
        return false;
    }

    auto close_ret = file.close();
    if (!close_ret) {
        LOGE("%s: Failed to close file: %s",
             full_path.c_str(), close_ret.error().message().c_str());
        return false;
    }

    return n == 0;
}

ErrorCode MinizipUtils::add_file_from_data(void *handle,
                                           const std::string &name,
                                           const std::string &data)
{
    mz_zip_file file_info = {};
    file_info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
    file_info.filename = name.c_str();
    file_info.filename_size = static_cast<uint16_t>(name.size());

    int ret = mz_zip_entry_write_open(handle, &file_info,
                                      MZ_COMPRESS_LEVEL_DEFAULT, nullptr);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to open inner file: %d", ret);
        return ErrorCode::ArchiveWriteDataError;
    }

    auto close_inner_write = finally([&] {
        mz_zip_entry_close(handle);
    });

    // Write data to file
    int n = mz_zip_entry_write(handle, data.data(),
                               static_cast<uint32_t>(data.size()));
    if (n < 0 || static_cast<size_t>(n) != data.size()) {
        LOGE("minizip: Failed to write inner file data");
        return ErrorCode::ArchiveWriteDataError;
    }

    close_inner_write.dismiss();

    ret = mz_zip_entry_close(handle);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to close inner file: %d", ret);
        return ErrorCode::ArchiveWriteDataError;
    }

    return ErrorCode::NoError;
}

ErrorCode MinizipUtils::add_file_from_path(void *handle,
                                           const std::string &name,
                                           const std::string &path)
{
    // Copy file into archive
    StandardFile file;

    auto open_ret = FileUtils::open_file(file, path,
                                         FileOpenMode::ReadOnly);
    if (!open_ret) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), open_ret.error().message().c_str());
        return ErrorCode::FileOpenError;
    }

    mz_zip_file file_info = {};
    file_info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
    file_info.filename = name.c_str();
    file_info.filename_size = static_cast<uint16_t>(name.size());

    int ret = mz_os_get_file_date(path.c_str(), &file_info.modified_date,
                                  &file_info.accessed_date,
                                  &file_info.creation_date);
    if (ret != MZ_OK) {
        LOGE("%s: Failed to get modification time: %d", path.c_str(), ret);
        return ErrorCode::FileOpenError;
    }

    ret = mz_zip_entry_write_open(handle, &file_info,
                                  MZ_COMPRESS_LEVEL_DEFAULT, nullptr);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to open inner file: %d", ret);
        return ErrorCode::ArchiveWriteDataError;
    }

    auto close_inner_write = finally([&] {
        mz_zip_entry_close(handle);
    });

    // Write data to file
    char buf[UINT16_MAX];

    while (true) {
        auto bytes_read = file.read(buf, sizeof(buf));
        if (!bytes_read) {
            LOGE("%s: Failed to read data: %s",
                 path.c_str(), bytes_read.error().message().c_str());
            return ErrorCode::FileReadError;
        } else if (bytes_read.value() == 0) {
            break;
        }

        auto bytes_written = mz_zip_entry_write(
                handle, buf, static_cast<uint32_t>(bytes_read.value()));
        if (static_cast<int>(bytes_read.value()) != bytes_written) {
            LOGE("minizip: Failed to write inner file data");
            return ErrorCode::ArchiveWriteDataError;
        }
    }

    close_inner_write.dismiss();

    ret = mz_zip_entry_close(handle);
    if (ret != MZ_OK) {
        LOGE("minizip: Failed to close inner file: %d", ret);
        return ErrorCode::ArchiveWriteDataError;
    }

    return ErrorCode::NoError;
}

}
