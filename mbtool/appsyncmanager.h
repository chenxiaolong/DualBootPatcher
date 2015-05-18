/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "packages.h"

namespace mb
{

class AppSyncManager
{
public:
    static void detect_directories();

    static std::string get_shared_apk_path(const std::string &pkg);
    static std::string get_shared_data_path(const std::string &pkg);

    static bool copy_apk_user_to_shared(const std::string &pkg);
    static bool link_apk_shared_to_user(const std::shared_ptr<Package> &pkg);

    static bool wipe_shared_libraries(const std::shared_ptr<Package> &pkg);

    static bool initialize_directories();
    static bool create_shared_data_directory(const std::string &pkg, uid_t uid);
    static bool fix_shared_apk_permissions();
    static bool fix_shared_data_permissions();
    static bool fix_user_apk_context();

    static bool mount_shared_directory(const std::string &pkg, uid_t uid);
    static bool unmount_shared_directory(const std::string &pkg);
};

}
