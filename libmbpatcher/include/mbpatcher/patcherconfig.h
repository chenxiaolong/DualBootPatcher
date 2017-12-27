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

#include <memory>
#include <vector>

#include "mbcommon/common.h"

#include "mbpatcher/errors.h"
#include "mbpatcher/fileinfo.h"


namespace mb::patcher
{

class Patcher;
class AutoPatcher;

class MB_EXPORT PatcherConfig
{
public:
    PatcherConfig();
    ~PatcherConfig();

    ErrorCode error() const;

    std::string data_directory() const;
    std::string temp_directory() const;

    void set_data_directory(std::string path);
    void set_temp_directory(std::string path);

    std::vector<std::string> patchers() const;
    std::vector<std::string> auto_patchers() const;

    Patcher * create_patcher(const std::string &id);
    AutoPatcher * create_auto_patcher(const std::string &id,
                                      const FileInfo &info);

    void destroy_patcher(Patcher *patcher);
    void destroy_auto_patcher(AutoPatcher *patcher);

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(PatcherConfig)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(PatcherConfig)

private:
    // Directories
    std::string m_data_dir;
    std::string m_temp_dir;

    // Errors
    ErrorCode m_error;

    // Created patchers
    std::vector<std::unique_ptr<Patcher>> m_patchers;
    std::vector<std::unique_ptr<AutoPatcher>> m_auto_patchers;
};

}
