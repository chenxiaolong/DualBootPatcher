/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "packages.h"
#include "roms.h"
#include "romconfig.h"

namespace mb
{

struct RomConfigAndPackages
{
    std::shared_ptr<Rom> rom;
    RomConfig config;
    Packages packages;
};

class AppSyncManager
{
public:
    static void detect_directories();

    static std::string get_shared_data_path(const std::string &pkg);

    static bool initialize_directories();
    static bool create_shared_data_directory(const std::string &pkg, uid_t uid);
    static bool fix_shared_data_permissions();

    static bool mount_shared_directory(const std::string &pkg, uid_t uid);
    static bool unmount_shared_directory(const std::string &pkg);
};

}
