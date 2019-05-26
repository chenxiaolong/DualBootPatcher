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

#include <memory>
#include <optional>

#include <cstdint>

#include "mbcommon/common.h"
#include "mbcommon/flags.h"

namespace mb::bootimg
{

enum class EntryType
{
    Kernel           = 1 << 0,
    Ramdisk          = 1 << 1,
    SecondBoot       = 1 << 2,
    DeviceTree       = 1 << 3,
    Aboot            = 1 << 4,
    MtkKernelHeader  = 1 << 5,
    MtkRamdiskHeader = 1 << 6,
    SonyCmdline      = 1 << 7,
    SonyIpl          = 1 << 8,
    SonyRpm          = 1 << 9,
    SonyAppsbl       = 1 << 10,
};
MB_DECLARE_FLAGS(EntryTypes, EntryType)
MB_DECLARE_OPERATORS_FOR_FLAGS(EntryTypes)

class MB_EXPORT Entry
{
public:
    Entry(EntryType type) noexcept;
    ~Entry() noexcept;

    MB_DEFAULT_COPY_CONSTRUCT_AND_ASSIGN(Entry)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(Entry)

    bool operator==(const Entry &rhs) const noexcept;
    bool operator!=(const Entry &rhs) const noexcept;

    EntryType type() const;

    std::optional<uint64_t> size() const;
    void set_size(std::optional<uint64_t> size);

private:
    EntryType m_type;
    std::optional<uint64_t> m_size;
};

}
