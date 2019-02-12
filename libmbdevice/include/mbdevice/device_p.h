/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/flags.h"

namespace mb::device::detail
{

struct BaseOptions
{
    std::string id;
    std::vector<std::string> codenames;
    std::string name;
    std::string architecture;
    DeviceFlags flags;

    std::vector<std::string> base_dirs;
    std::vector<std::string> system_devs;
    std::vector<std::string> cache_devs;
    std::vector<std::string> data_devs;
    std::vector<std::string> boot_devs;
    std::vector<std::string> recovery_devs;
    std::vector<std::string> extra_devs;

    bool operator==(const BaseOptions &other) const;
};

struct TwOptions
{
    bool supported;

    TwPixelFormat pixel_format;

    bool operator==(const TwOptions &other) const;
};

}
