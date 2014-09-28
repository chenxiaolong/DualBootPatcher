/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "configfile.h"
#include "common.h"

#include <fstream>

std::string ConfigFile::m_configfile;
std::string ConfigFile::m_configdir;
Json::Value ConfigFile::m_root;
int ConfigFile::m_version;

std::string ConfigFile::get_config_file() {
    if (m_configfile.empty()) {
        if (exists_directory(RAW_DATA)) {
            m_configfile = RAW_DATA + SEP + CONFIG_FILE;
        } else {
            m_configfile = DATA + SEP + CONFIG_FILE;
        }
    }

    return m_configfile;
}

std::string ConfigFile::get_config_dir() {
    if (m_configdir.empty()) {
        if (exists_directory(RAW_DATA)) {
            m_configdir = RAW_DATA;
        } else {
            m_configdir = DATA;
        }
    }

    return m_configdir;
}

bool ConfigFile::load_config() {
    std::ifstream file;
    file.open(get_config_file());

    if (!file) {
        LOGE("Config file %s not found", get_config_file().c_str());
        return false;
    }

    Json::Reader reader;
    bool success = reader.parse(file, m_root, false);

    file.close();

    if (!success) {
        LOGE("Failed to parse configuration file");
        return false;
    }

    const Json::Value jsonversion = m_root[CONF_VERSION];
    if (jsonversion.isInt()) {
        m_version = jsonversion.asInt();
    } else {
        return false;
    }

    return true;
}

void ConfigFile::write_config() {
    std::ofstream file;
    file.open(get_config_file());
    file << m_root;
    file.close();
}

bool ConfigFile::contains_package(std::string package) {
    return !m_root[CONF_PACKAGES][package].isNull();
}

std::vector<std::string> ConfigFile::get_packages() {
    if (m_version == 1) {
        return get_packages_1();
    }

    return std::vector<std::string>();
}

std::vector<std::string> ConfigFile::get_rom_ids(std::string package) {
    if (m_version == 1) {
        return get_rom_ids_1(package);
    }

    return std::vector<std::string>();
}

bool ConfigFile::is_data_shared(std::string package) {
    if (m_version == 1) {
        return is_data_shared_1(package);
    }

    return false;
}

bool ConfigFile::remove_rom_id(std::string package, std::string rom_id) {
    if (m_version == 1) {
        return remove_rom_id_1(package, rom_id);
    }

    return false;
}

std::vector<std::string> ConfigFile::get_packages_1() {
    const Json::Value packages = m_root[CONF_PACKAGES];
    return packages.getMemberNames();
}

std::vector<std::string> ConfigFile::get_rom_ids_1(std::string package) {
    const Json::Value syncacross =
            m_root[CONF_PACKAGES][package][CONF_SYNC_ACROSS];

    std::vector<std::string> rom_ids;

    if (syncacross.isNull() || !syncacross.isArray()) {
        return rom_ids;
    }

    for (unsigned int i = 0; i < syncacross.size(); i++) {
        rom_ids.push_back(syncacross[i].asString());
    }

    return rom_ids;
}

bool ConfigFile::is_data_shared_1(std::string package) {
    const Json::Value sharedata =
            m_root[CONF_PACKAGES][package][CONF_SHARE_DATA];

    if (!sharedata.isNull() && !sharedata.isBool()) {
        return sharedata.asBool();
    }

    return false;
}

bool ConfigFile::remove_rom_id_1(std::string package, std::string rom_id) {
    const Json::Value syncacross =
            m_root[CONF_PACKAGES][package][CONF_SYNC_ACROSS];

    if (syncacross.isNull() || !syncacross.isArray()) {
        return false;
    }

    bool removed = false;

    // jsoncpp has no built-in way of removing items from an array
    Json::Value new_array(Json::arrayValue);
    for (unsigned int i = 0; i < syncacross.size(); i++) {
        std::string value = syncacross[i].asString();
        if (rom_id == value) {
            removed = true;
            continue;
        }
        new_array.append(value);
    }

    if (new_array.size() == 0) {
        m_root[CONF_PACKAGES].removeMember(package);
        removed = true;
    } else {
        m_root[CONF_PACKAGES][package][CONF_SYNC_ACROSS] = new_array;
    }

    if (removed) {
        write_config();
    }

    return removed;
}
