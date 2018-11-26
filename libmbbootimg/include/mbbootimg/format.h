/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <string_view>

#include "mbcommon/common.h"

namespace mb::bootimg
{

enum class Format
{
    Android,
    Bump,
    Loki,
    Mtk,
    SonyElf,
};

std::string_view format_to_name(Format format);

std::optional<Format> name_to_format(std::string_view name);

// TODO: Switch to span eventually
std::basic_string_view<Format> formats();

}