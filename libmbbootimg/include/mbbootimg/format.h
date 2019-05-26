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
#include "mbcommon/flags.h"

namespace mb::bootimg
{

enum class Format : uint8_t
{
    Android = 1 << 0,
    Bump    = 1 << 1,
    Loki    = 1 << 2,
    Mtk     = 1 << 3,
    SonyElf = 1 << 4,
};
MB_DECLARE_FLAGS(Formats, Format)
MB_DECLARE_OPERATORS_FOR_FLAGS(Formats)

constexpr Formats ALL_FORMATS =
        Format::Android
        | Format::Bump
        | Format::Loki
        | Format::Mtk
        | Format::SonyElf;

MB_EXPORT std::string_view format_to_name(Format format);

MB_EXPORT std::optional<Format> name_to_format(std::string_view name);

}
