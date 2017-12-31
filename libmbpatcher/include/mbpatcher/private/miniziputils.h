/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <string>
#include <vector>

#include "minizip/unzip.h"
#include "minizip/zip.h"

#include "mbpatcher/errors.h"


namespace mb::patcher
{

struct UnzCtx;
struct ZipCtx;

class MinizipUtils
{
public:
    struct ArchiveStats {
        uint64_t files;
        uint64_t total_size;
    };

    static std::string unz_error_string(int ret);

    static std::string zip_error_string(int ret);

    static unzFile ctx_get_unz_file(UnzCtx *ctx);

    static zipFile ctx_get_zip_file(ZipCtx *ctx);

    static UnzCtx * open_input_file(std::string path);

    static ZipCtx * open_output_file(std::string path);

    static int close_input_file(UnzCtx *ctx);

    static int close_output_file(ZipCtx *ctx);

    static ErrorCode archive_stats(const std::string &path,
                                   ArchiveStats *stats,
                                   std::vector<std::string> ignore);

    static bool get_info(unzFile uf,
                         unz_file_info64 *fi,
                         std::string *filename);

    static bool copy_file_raw(unzFile uf,
                              zipFile zf,
                              const std::string &name,
                              void (*cb)(uint64_t bytes, void *),
                              void *userData);

    static bool read_to_memory(unzFile uf,
                               std::vector<unsigned char> *output,
                               void (*cb)(uint64_t bytes, void *),
                               void *userData);

    static bool extract_file(unzFile uf,
                             const std::string &directory);

    static ErrorCode add_file(zipFile zf,
                              const std::string &name,
                              const std::vector<unsigned char> &contents);

    static ErrorCode add_file(zipFile zf,
                              const std::string &name,
                              const std::string &path);
};

}
