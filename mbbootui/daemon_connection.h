/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <string>
#include <vector>

#include <flatbuffers/flatbuffers.h>

class Rom
{
public:
    std::string id;
    std::string system_path;
    std::string cache_path;
    std::string data_path;
    std::string version;
    std::string build;
};

enum class SwitchRomResult
{
    Succeeded,
    Failed,
    ChecksumInvalid,
    ChecksumNotFound,
};

class MbtoolInterface
{
public:
    virtual ~MbtoolInterface() {}

    virtual bool get_installed_roms(std::vector<Rom> &result) = 0;
    virtual bool get_booted_rom_id(std::string &result) = 0;
    virtual bool switch_rom(const std::string &id,
                            const std::string &boot_block_dev,
                            const std::vector<std::string> &block_dev_base_dirs,
                            bool force_checksums_update,
                            SwitchRomResult &result) = 0;
    virtual bool reboot(const std::string &arg, bool &result) = 0;
    virtual bool shutdown(bool &result) = 0;
    virtual bool version(std::string &result) = 0;
};

class MbtoolConnection
{
public:
    MbtoolConnection();
    ~MbtoolConnection();
    bool connect();
    bool disconnect();

    MbtoolInterface * interface();

    MbtoolConnection(const MbtoolConnection &) = delete;
    MbtoolConnection(MbtoolConnection &&) = default;
    MbtoolConnection & operator=(const MbtoolConnection &) & = delete;
    MbtoolConnection & operator=(MbtoolConnection &&) & = default;

private:
    int _fd;
    MbtoolInterface *_iface;
};

extern MbtoolConnection mbtool_connection;
extern MbtoolInterface *mbtool_interface;
