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

#include "mbp/errors.h"


namespace mbp
{

class MinizipUtils
{
public:
    struct ArchiveStats {
        uint64_t files;
        uint64_t totalSize;
    };

    static std::string unzErrorString(int ret);

    static std::string zipErrorString(int ret);

    struct UnzCtx;
    struct ZipCtx;

    static unzFile ctxGetUnzFile(UnzCtx *ctx);

    static zipFile ctxGetZipFile(ZipCtx *ctx);

    static UnzCtx * openInputFile(std::string path);

    static ZipCtx * openOutputFile(std::string path);

    static int closeInputFile(UnzCtx *ctx);

    static int closeOutputFile(ZipCtx *ctx);

    static ErrorCode archiveStats(const std::string &path,
                                  ArchiveStats *stats,
                                  std::vector<std::string> ignore);

    static bool getInfo(unzFile uf,
                        unz_file_info64 *fi,
                        std::string *filename);

    static bool copyFileRaw(unzFile uf,
                            zipFile zf,
                            const std::string &name,
                            void (*cb)(uint64_t bytes, void *), void *userData);

    static bool readToMemory(unzFile uf,
                             std::vector<unsigned char> *output,
                             void (*cb)(uint64_t bytes, void *), void *userData);

    static bool extractFile(unzFile uf,
                            const std::string &directory);

    static ErrorCode addFile(zipFile zf,
                             const std::string &name,
                             const std::vector<unsigned char> &contents);

    static ErrorCode addFile(zipFile zf,
                             const std::string &name,
                             const std::string &path);
};

}
