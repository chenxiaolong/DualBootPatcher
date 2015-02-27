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

#pragma once

#include <string>
#include <vector>

#include <archive.h>
#include <archive_entry.h>

#include "patchererror.h"


namespace mbp
{

class FileUtils
{
public:
    static PatcherError readToMemory(const std::string &path,
                                     std::vector<unsigned char> *contents);

    static PatcherError writeFromMemory(const std::string &path,
                                        const std::vector<unsigned char> &contents);

    // libarchive
    static bool laReadToByteArray(archive *aInput,
                                  archive_entry *entry,
                                  std::vector<unsigned char> *output);

    static bool laCopyData(archive *aInput, archive *aOutput);

    static bool laExtractFile(archive *aInput,
                              archive_entry *entry,
                              const std::string &directory);

    static PatcherError laAddFile(archive * const aOutput,
                                  const std::string &name,
                                  const std::vector<unsigned char> &contents);

    static PatcherError laAddFile(archive * const aOutput,
                                  const std::string &name,
                                  const std::string &path);

    static PatcherError laCountFiles(const std::string &path,
                                     unsigned int *count,
                                     std::vector<std::string> ignore = std::vector<std::string>());
};

}