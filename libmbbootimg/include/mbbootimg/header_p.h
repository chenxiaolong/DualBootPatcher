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

#include "mbcommon/optional.h"

namespace mb
{
namespace bootimg
{

class HeaderPrivate
{
public:
    // Bitmap of fields that are supported
    HeaderFields fields_supported;

    struct {
        // Used in:                             | Android | Loki | Bump | Mtk | Sony |
        optional<uint32_t> kernel_addr;      // | X       | X    | X    | X   | X    |
        optional<uint32_t> ramdisk_addr;     // | X       | X    | X    | X   | X    |
        optional<uint32_t> second_addr;      // | X       | X    | X    | X   |      |
        optional<uint32_t> tags_addr;        // | X       | X    | X    | X   |      |
        optional<uint32_t> ipl_addr;         // |         |      |      |     | X    |
        optional<uint32_t> rpm_addr;         // |         |      |      |     | X    |
        optional<uint32_t> appsbl_addr;      // |         |      |      |     | X    |
        optional<uint32_t> page_size;        // | X       | X    | X    | X   |      |
        optional<std::string> board_name;    // | X       | X    | X    | X   |      |
        optional<std::string> cmdline;       // | X       | X    | X    | X   |      |
        // Raw header values                    |---------|------|------|-----|------|

        // TODO TODO TODO
        optional<uint32_t> hdr_kernel_size;  // | X       | X    | X    | X   |      |
        optional<uint32_t> hdr_ramdisk_size; // | X       | X    | X    | X   |      |
        optional<uint32_t> hdr_second_size;  // | X       | X    | X    | X   |      |
        optional<uint32_t> hdr_dt_size;      // | X       | X    | X    | X   |      |
        optional<uint32_t> hdr_unused;       // | X       | X    | X    | X   |      |
        optional<uint32_t> hdr_id[8];        // | X       | X    | X    | X   |      |
        optional<uint32_t> hdr_entrypoint;   // |         |      |      |     | X    |
        // TODO TODO TODO
    } field;
};

}
}
