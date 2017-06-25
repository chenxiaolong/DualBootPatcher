/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "roms.h"

namespace mb
{

bool wipe_directory(const std::string &directory,
                    const std::vector<std::string> &exclusions);
bool wipe_system(const std::shared_ptr<Rom> &rom);
bool wipe_cache(const std::shared_ptr<Rom> &rom);
bool wipe_data(const std::shared_ptr<Rom> &rom);
bool wipe_dalvik_cache(const std::shared_ptr<Rom> &rom);
bool wipe_multiboot(const std::shared_ptr<Rom> &rom);

}
