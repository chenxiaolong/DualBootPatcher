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

#pragma once

#include <string>
#include <vector>

#include <archive.h>
#include <archive_entry.h>

namespace mb::util
{

struct ExtractInfo
{
    std::string from;
    std::string to;
};

struct ExistsInfo
{
    std::string path;
    bool exists;
};

enum class CompressionType : uint8_t
{
    None,
    Lz4,
    Gzip,
    Xz,
};

int libarchive_copy_data(archive *in, archive *out, archive_entry *entry);
bool libarchive_copy_data_disk_to_archive(archive *in, archive *out,
                                          archive_entry *entry);
int libarchive_copy_header_and_data(archive *in, archive *out,
                                    archive_entry *entry);
bool libarchive_tar_extract(const std::string &filename,
                            const std::string &target,
                            const std::vector<std::string> &patterns,
                            CompressionType compression);
bool libarchive_tar_create(const std::string &filename,
                           const std::string &base_dir,
                           const std::vector<std::string> &paths,
                           CompressionType compression);

bool extract_archive(const std::string &filename, const std::string &target);
bool extract_files(const std::string &filename, const std::string &target,
                   const std::vector<std::string> &files);
bool extract_files2(const std::string &filename,
                    const std::vector<ExtractInfo> &files);
bool archive_exists(const std::string &filename,
                    std::vector<ExistsInfo> &files);

}
