/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file.h"

#include "mbp/errors.h"


namespace mbp
{

class FileUtils
{
public:
    static ErrorCode openFile(MbFile *file, const std::string &path, int mode);

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
};

}
