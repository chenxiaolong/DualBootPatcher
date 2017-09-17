/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <unordered_map>
#include <vector>

namespace mb
{

enum class ChecksumsGetResult
{
    Found,
    NotFound,
    Malformed,
};

ChecksumsGetResult checksums_get(std::unordered_map<std::string, std::string> *props,
                                 const std::string &rom_id,
                                 const std::string &image,
                                 std::string *sha512_out);
void checksums_update(std::unordered_map<std::string, std::string> *props,
                      const std::string &rom_id,
                      const std::string &image,
                      const std::string &sha512);
bool checksums_read(std::unordered_map<std::string, std::string> *props);
bool checksums_write(const std::unordered_map<std::string, std::string> &props);

enum class SwitchRomResult
{
    Succeeded,
    Failed,
    ChecksumNotFound,
    ChecksumInvalid,
};

SwitchRomResult switch_rom(const std::string &id,
                           const std::string &boot_blockdev,
                           const std::vector<std::string> &blockdev_base_dirs,
                           bool force_update_checksums);
bool set_kernel(const std::string &id, const std::string &boot_blockdev);

}
