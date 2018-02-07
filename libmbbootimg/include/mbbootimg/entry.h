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

#include <memory>
#include <optional>
#include <string>

#include <cstdint>

#include "mbcommon/common.h"

namespace mb::bootimg
{

constexpr int ENTRY_TYPE_KERNEL             = 1 << 0;
constexpr int ENTRY_TYPE_RAMDISK            = 1 << 1;
constexpr int ENTRY_TYPE_SECONDBOOT         = 1 << 2;
constexpr int ENTRY_TYPE_DEVICE_TREE        = 1 << 3;
constexpr int ENTRY_TYPE_ABOOT              = 1 << 4;
constexpr int ENTRY_TYPE_MTK_KERNEL_HEADER  = 1 << 5;
constexpr int ENTRY_TYPE_MTK_RAMDISK_HEADER = 1 << 6;
constexpr int ENTRY_TYPE_SONY_IPL           = 1 << 7;
constexpr int ENTRY_TYPE_SONY_RPM           = 1 << 8;
constexpr int ENTRY_TYPE_SONY_APPSBL        = 1 << 9;

class MB_EXPORT Entry
{
public:
    Entry();
    ~Entry();

    MB_DEFAULT_COPY_CONSTRUCT_AND_ASSIGN(Entry)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(Entry)

    bool operator==(const Entry &rhs) const;
    bool operator!=(const Entry &rhs) const;

    void clear();

    std::optional<int> type() const;
    void set_type(std::optional<int> type);

    std::optional<std::string> name() const;
    void set_name(std::optional<std::string> name);

    std::optional<uint64_t> size() const;
    void set_size(std::optional<uint64_t> size);

private:
    std::optional<int> m_type;
    std::optional<std::string> m_name;
    std::optional<uint64_t> m_size;
};

}
