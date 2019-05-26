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

#include <string>

#include "mbcommon/file/standard.h"

#include "mbpatcher/errors.h"


namespace mb::patcher
{

class FileUtils
{
public:
    static oc::result<void> open_file(StandardFile &file,
                                      const std::string &path,
                                      FileOpenMode mode);

    static ErrorCode read_to_string(const std::string &path,
                                    std::string *contents);

    static ErrorCode write_from_string(const std::string &path,
                                       const std::string &contents);

    static std::string system_temporary_dir();

    static std::string create_temporary_dir(const std::string &directory);
};

}
