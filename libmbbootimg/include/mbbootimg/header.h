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
#include <string>

#include <cstdint>

#include "mbcommon/common.h"
#include "mbcommon/flags.h"
#include "mbcommon/optional.h"

namespace mb
{
namespace bootimg
{

enum class HeaderField : uint64_t
{
    KernelAddress       = 1ull << 0,
    RamdiskAddress      = 1ull << 1,
    SecondbootAddress   = 1ull << 2,
    KernelTagsAddress   = 1ull << 3,
    SonyIplAddress      = 1ull << 4,
    SonyRpmAddress      = 1ull << 5,
    SonyAppsblAddress   = 1ull << 6,
    PageSize            = 1ull << 7,
    BoardName           = 1ull << 8,
    KernelCmdline       = 1ull << 9,
    // Raw header fields
    // TODO TODO TODO
    KernelSize          = 1ull << 10,
    RamdiskSize         = 1ull << 11,
    SecondbootSize      = 1ull << 12,
    DeviceTreeSize      = 1ull << 13,
    Unused              = 1ull << 14,
    Id                  = 1ull << 15,
    Entrypoint          = 1ull << 16,
};
MB_DECLARE_FLAGS(HeaderFields, HeaderField)
MB_DECLARE_OPERATORS_FOR_FLAGS(HeaderFields)

constexpr HeaderFields ALL_FIELDS =
        HeaderField::KernelAddress
        | HeaderField::RamdiskAddress
        | HeaderField::SecondbootAddress
        | HeaderField::KernelTagsAddress
        | HeaderField::SonyIplAddress
        | HeaderField::SonyRpmAddress
        | HeaderField::SonyAppsblAddress
        | HeaderField::PageSize
        | HeaderField::BoardName
        | HeaderField::KernelCmdline
        | HeaderField::KernelSize
        | HeaderField::RamdiskSize
        | HeaderField::SecondbootSize
        | HeaderField::DeviceTreeSize
        | HeaderField::Unused
        | HeaderField::Id
        | HeaderField::Entrypoint;

class MB_EXPORT Header
{
public:
    Header();
    ~Header();

    MB_DEFAULT_COPY_CONSTRUCT_AND_ASSIGN(Header)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(Header)

    bool operator==(const Header &rhs) const;
    bool operator!=(const Header &rhs) const;

    void clear();

    HeaderFields supported_fields() const;
    void set_supported_fields(HeaderFields fields);

    optional<std::string> board_name() const;
    bool set_board_name(optional<std::string> name);

    optional<std::string> kernel_cmdline() const;
    bool set_kernel_cmdline(optional<std::string> cmdline);

    optional<uint32_t> page_size() const;
    bool set_page_size(optional<uint32_t> page_size);

    optional<uint32_t> kernel_address() const;
    bool set_kernel_address(optional<uint32_t> address);

    optional<uint32_t> ramdisk_address() const;
    bool set_ramdisk_address(optional<uint32_t> address);

    optional<uint32_t> secondboot_address() const;
    bool set_secondboot_address(optional<uint32_t> address);

    optional<uint32_t> kernel_tags_address() const;
    bool set_kernel_tags_address(optional<uint32_t> address);

    optional<uint32_t> sony_ipl_address() const;
    bool set_sony_ipl_address(optional<uint32_t> address);

    optional<uint32_t> sony_rpm_address() const;
    bool set_sony_rpm_address(optional<uint32_t> address);

    optional<uint32_t> sony_appsbl_address() const;
    bool set_sony_appsbl_address(optional<uint32_t> address);

    optional<uint32_t> entrypoint_address() const;
    bool set_entrypoint_address(optional<uint32_t> address);

private:
    // Bitmap of fields that are supported
    HeaderFields m_fields_supported;

    // Used in:                               | Android | Loki | Bump | Mtk | Sony |
    optional<uint32_t> m_kernel_addr;      // | X       | X    | X    | X   | X    |
    optional<uint32_t> m_ramdisk_addr;     // | X       | X    | X    | X   | X    |
    optional<uint32_t> m_second_addr;      // | X       | X    | X    | X   |      |
    optional<uint32_t> m_tags_addr;        // | X       | X    | X    | X   |      |
    optional<uint32_t> m_ipl_addr;         // |         |      |      |     | X    |
    optional<uint32_t> m_rpm_addr;         // |         |      |      |     | X    |
    optional<uint32_t> m_appsbl_addr;      // |         |      |      |     | X    |
    optional<uint32_t> m_page_size;        // | X       | X    | X    | X   |      |
    optional<std::string> m_board_name;    // | X       | X    | X    | X   |      |
    optional<std::string> m_cmdline;       // | X       | X    | X    | X   |      |
    // Raw header values                      |---------|------|------|-----|------|

    // TODO TODO TODO
    optional<uint32_t> m_hdr_kernel_size;  // | X       | X    | X    | X   |      |
    optional<uint32_t> m_hdr_ramdisk_size; // | X       | X    | X    | X   |      |
    optional<uint32_t> m_hdr_second_size;  // | X       | X    | X    | X   |      |
    optional<uint32_t> m_hdr_dt_size;      // | X       | X    | X    | X   |      |
    optional<uint32_t> m_hdr_unused;       // | X       | X    | X    | X   |      |
    optional<uint32_t> m_hdr_id[8];        // | X       | X    | X    | X   |      |
    optional<uint32_t> m_hdr_entrypoint;   // |         |      |      |     | X    |
    // TODO TODO TODO
};

}
}
