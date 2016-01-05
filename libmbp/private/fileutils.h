/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <string>
#include <vector>

#include "external/minizip/unzip.h"
#include "external/minizip/zip.h"

#include "errors.h"


namespace mbp
{

class FileUtils
{
public:
    static ErrorCode readToMemory(const std::string &path,
                                  std::vector<unsigned char> *contents);
    static ErrorCode readToString(const std::string &path,
                                  std::string *contents);

    static ErrorCode writeFromMemory(const std::string &path,
                                     const std::vector<unsigned char> &contents);
    static ErrorCode writeFromString(const std::string &path,
                                     const std::string &contents);

    static std::string systemTemporaryDir();

    static std::string createTemporaryDir(const std::string &directory);

    struct ArchiveStats {
        uint64_t files;
        uint64_t totalSize;
    };

    struct MzUnzCtx;
    struct MzZipCtx;

    static unzFile mzCtxGetUnzFile(MzUnzCtx *ctx);

    static zipFile mzCtxGetZipFile(MzZipCtx *ctx);

    static MzUnzCtx * mzOpenInputFile(const std::string &path);

    static MzZipCtx * mzOpenOutputFile(const std::string &path);

    static int mzCloseInputFile(MzUnzCtx *ctx);

    static int mzCloseOutputFile(MzZipCtx *ctx);

    static ErrorCode mzArchiveStats(const std::string &path,
                                    ArchiveStats *stats,
                                    std::vector<std::string> ignore);

    static bool mzGetInfo(unzFile uf,
                          unz_file_info64 *fi,
                          std::string *filename);

    static bool mzCopyFileRaw(unzFile uf,
                              zipFile zf,
                              const std::string &name,
                              void (*cb)(uint64_t bytes, void *), void *userData);

    static bool mzReadToMemory(unzFile uf,
                               std::vector<unsigned char> *output,
                               void (*cb)(uint64_t bytes, void *), void *userData);

    static bool mzExtractFile(unzFile uf,
                              const std::string &directory);

    static ErrorCode mzAddFile(zipFile zf,
                               const std::string &name,
                               const std::vector<unsigned char> &contents);

    static ErrorCode mzAddFile(zipFile zf,
                               const std::string &name,
                               const std::string &path);
};

}
