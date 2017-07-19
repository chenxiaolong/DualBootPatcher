/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <string>
#include <vector>

namespace mb
{

class Rom
{
public:
    friend class Roms;

    enum class Source
    {
        SYSTEM,
        CACHE,
        DATA,
        EXTERNAL_SD
    };

    std::string id;
    Source system_source;
    Source cache_source;
    Source data_source;
private:
    std::string system_path;
    std::string cache_path;
    std::string data_path;
public:
    bool system_is_image;
    bool cache_is_image;
    bool data_is_image;

    std::string full_system_path();
    std::string full_cache_path();
    std::string full_data_path();

    std::string boot_image_path();
    std::string config_path();
    std::string thumbnail_path();
};

class Roms
{
public:
    std::vector<std::shared_ptr<Rom>> roms;

private:
    static std::shared_ptr<Rom> create_rom_primary();
    static std::shared_ptr<Rom> create_rom_dual();
    static std::shared_ptr<Rom> create_rom_multi_slot(unsigned int num);
    static std::shared_ptr<Rom> create_rom_data_slot(const std::string &id);
    static std::shared_ptr<Rom> create_rom_extsd_slot(const std::string &id);

    void add_builtin();
    void add_data_roms();
    void add_extsd_roms();
public:
    void add_installed();

    std::shared_ptr<Rom> find_by_id(const std::string &id) const;

    static std::shared_ptr<Rom> get_current_rom();

    static std::shared_ptr<Rom> create_rom(const std::string &id);
    static bool is_valid(const std::string &id);

    static std::string get_system_partition();
    static std::string get_cache_partition();
    static std::string get_data_partition();
    static std::string get_extsd_partition();
    static std::string get_mountpoint(Rom::Source source);
};

std::string get_raw_path(const std::string &path);

}
