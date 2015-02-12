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

#include <memory>
#include <string>
#include <vector>

#include "packages.h"

namespace mb
{

class Rom
{
public:
    std::string id;
    std::string system_path;
    std::string cache_path;
    std::string data_path;
    bool use_raw_paths;
    std::vector<std::shared_ptr<Package>> pkgs;

    Rom();
};

bool mb_roms_add_builtin(std::vector<std::shared_ptr<Rom>> *roms);
bool mb_roms_add_installed(std::vector<std::shared_ptr<Rom>> *roms);
std::shared_ptr<Rom> mb_find_rom_by_id(std::vector<std::shared_ptr<Rom>> *roms,
                                       const std::string &id);

}