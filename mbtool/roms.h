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

namespace mb
{

class Rom
{
public:
    std::string id;
    std::string system_path;
    std::string cache_path;
    std::string data_path;
};

class Roms
{
public:
    std::vector<std::shared_ptr<Rom>> roms;

    void add_builtin();
    void add_data_roms();
    void add_installed();

    std::shared_ptr<Rom> find_by_id(const std::string &id) const;

    static std::shared_ptr<Rom> get_current_rom();

    static bool is_named_rom(const std::string &id);
    static std::shared_ptr<Rom> create_named_rom(const std::string &id);
};

std::string get_raw_path(const std::string &path);

}
