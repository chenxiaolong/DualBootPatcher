/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <json/json.h>

static const std::string CONFIG_FILE = "syncdaemon.json";

// Config JSON constants
static const std::string CONF_VERSION = "jsonversion";
static const std::string CONF_PACKAGES = "packages";
static const std::string CONF_SYNC_ACROSS = "syncacross";
static const std::string CONF_SHARE_DATA = "sharedata";

class ConfigFile {
public:
    static std::string get_config_file();
    static std::string get_config_dir();
    static bool load_config();
    static bool contains_package(std::string package);

    static std::vector<std::string> get_packages();
    static std::vector<std::string> get_rom_ids(std::string package);
    static bool is_data_shared(std::string package);
    static bool remove_rom_id(std::string package, std::string rom_id);

private:
    static std::string m_configfile;
    static std::string m_configdir;
    static Json::Value m_root;
    static int m_version;

    static void write_config();

    // Config version 1
    static std::vector<std::string> get_packages_1();
    static std::vector<std::string> get_rom_ids_1(std::string package);
    static bool is_data_shared_1(std::string package);
    static bool remove_rom_id_1(std::string package, std::string rom_id);
};

#endif //CONFIGFILE_H
