/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

class ChecksumProps
{
public:
    bool load_file();
    bool save_file();

    ChecksumsGetResult get(const std::string &rom_id, const std::string &image,
                           std::string &sha512_out);
    void set(const std::string &rom_id, const std::string &image,
             const std::string &sha512);

private:
    std::unordered_map<std::string, std::string> m_props;
};

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
