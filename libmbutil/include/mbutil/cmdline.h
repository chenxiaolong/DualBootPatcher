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

#include <optional>
#include <string>

namespace mb::util
{

enum class CmdlineIterAction
{
    Continue,
    Stop,
    Error
};

typedef CmdlineIterAction (*CmdlineIterFn)(const std::string &name,
                                           const std::optional<std::string> &value,
                                           void *userdata);

bool kernel_cmdline_iter(CmdlineIterFn fn, void *userdata);

bool kernel_cmdline_get_option(const std::string &option,
                               std::optional<std::string> &value);

}
