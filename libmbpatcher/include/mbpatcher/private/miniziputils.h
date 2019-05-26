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

#pragma once

#include <functional>
#include <string>
#include <vector>

#include "mz.h"
#include "mz_zip.h"

#include "mbpatcher/errors.h"


namespace mb::patcher
{

struct ZipCtx;

enum class ZipOpenMode
{
    Read,
    Write,
};

class MinizipUtils
{
public:
    struct ArchiveStats
    {
        uint64_t files;
        uint64_t total_size;
    };

    static void * ctx_get_zip_handle(ZipCtx *ctx);

    static ZipCtx * open_zip_file(std::string path, ZipOpenMode mode);

    static int close_zip_file(ZipCtx *ctx);

    static ErrorCode archive_stats(const std::string &path,
                                   ArchiveStats &stats,
                                   const std::vector<std::string> &ignore);

    static bool copy_file_raw(void *source_handle,
                              void *target_handle,
                              const std::string &name,
                              const std::function<void(uint64_t bytes)> &cb);

    static bool read_to_memory(void *handle, std::string &output,
                               const std::function<void(uint64_t bytes)> &cb);

    static bool extract_file(void *handle,
                             const std::string &directory);

    static ErrorCode add_file_from_data(void *handle,
                                        const std::string &name,
                                        const std::string &data);

    static ErrorCode add_file_from_path(void *handle,
                                        const std::string &name,
                                        const std::string &path);
};

}
