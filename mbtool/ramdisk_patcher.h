/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <functional>
#include <string>
//#include <vector>

namespace mb
{

typedef bool (RamdiskPatcherFn)(const std::string &dir);

std::function<RamdiskPatcherFn>
rp_write_rom_id(const std::string &rom_id);

std::function<RamdiskPatcherFn>
rp_patch_default_prop(const std::string &device_id, bool use_fuse_exfat);

std::function<RamdiskPatcherFn>
rp_add_binaries(const std::string &binaries_dir);

std::function<RamdiskPatcherFn>
rp_symlink_fuse_exfat();

std::function<RamdiskPatcherFn>
rp_symlink_init();

std::function<RamdiskPatcherFn>
rp_add_device_json(const std::string &device_json_file);

}
