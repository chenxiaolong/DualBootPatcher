/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/guard_p.h"

#include <string>

#include <cstdint>

#include "mbcommon/flags.h"
#include "mbcommon/optional.h"

namespace mb
{
namespace bootimg
{

enum class EntryField : uint8_t
{
    Type = 1 << 0,
    Name = 1 << 1,
    Size = 1 << 2,
};
MB_DECLARE_FLAGS(EntryFields, EntryField)
MB_DECLARE_OPERATORS_FOR_FLAGS(EntryFields)

class EntryPrivate
{
public:
    struct {
        // Entry type
        optional<int> type;
        // Entry name
        optional<std::string> name;
        // Entry size
        optional<uint64_t> size;
    } field;
};

}
}
