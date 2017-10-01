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

#include "mbbootimg/header.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "mbbootimg/header_p.h"

#define IS_SUPPORTED(STRUCT, FLAG) \
    ((STRUCT)->fields_supported & (FLAG))

#define ENSURE_SUPPORTED(STRUCT, FLAG) \
    do { \
        if (!IS_SUPPORTED(STRUCT, FLAG)) { \
            return false; \
        } \
    } while (0)


namespace mb
{
namespace bootimg
{

Header::Header()
    : _priv_ptr(new HeaderPrivate())
{
    MB_PRIVATE(Header);
    priv->fields_supported = ALL_FIELDS;
}

Header::Header(const Header &header)
    : _priv_ptr(new HeaderPrivate(*header._priv_ptr))
{
}

Header::Header(Header &&header) noexcept
    : Header()
{
    _priv_ptr.swap(header._priv_ptr);
}

Header::~Header() = default;

Header & Header::operator=(const Header &header)
{
    *_priv_ptr = HeaderPrivate(*header._priv_ptr);
    return *this;
}

Header & Header::operator=(Header &&header) noexcept
{
    _priv_ptr.swap(header._priv_ptr);
    *header._priv_ptr = HeaderPrivate();
    return *this;
}

bool Header::operator==(const Header &rhs) const
{
    auto const &field1 = _priv_ptr->field;
    auto const &field2 = rhs._priv_ptr->field;

    return field1.kernel_addr == field2.kernel_addr
            && field1.ramdisk_addr == field2.ramdisk_addr
            && field1.second_addr == field2.second_addr
            && field1.tags_addr == field2.tags_addr
            && field1.ipl_addr == field2.ipl_addr
            && field1.rpm_addr == field2.rpm_addr
            && field1.appsbl_addr == field2.appsbl_addr
            && field1.page_size == field2.page_size
            && field1.board_name == field2.board_name
            && field1.cmdline == field2.cmdline
            && field1.hdr_kernel_size == field2.hdr_kernel_size
            && field1.hdr_ramdisk_size == field2.hdr_ramdisk_size
            && field1.hdr_second_size == field2.hdr_second_size
            && field1.hdr_dt_size == field2.hdr_dt_size
            && field1.hdr_unused == field2.hdr_unused
            && field1.hdr_id[0] == field2.hdr_id[0]
            && field1.hdr_id[1] == field2.hdr_id[1]
            && field1.hdr_id[2] == field2.hdr_id[2]
            && field1.hdr_id[3] == field2.hdr_id[3]
            && field1.hdr_id[4] == field2.hdr_id[4]
            && field1.hdr_id[5] == field2.hdr_id[5]
            && field1.hdr_id[6] == field2.hdr_id[6]
            && field1.hdr_id[7] == field2.hdr_id[7]
            && field1.hdr_entrypoint == field2.hdr_entrypoint;
}

bool Header::operator!=(const Header &rhs) const
{
    return !(*this == rhs);
}

void Header::clear()
{
    MB_PRIVATE(Header);
    *priv = HeaderPrivate();
}

// Supported fields

HeaderFields Header::supported_fields() const
{
    MB_PRIVATE(const Header);
    return priv->fields_supported;
}

void Header::set_supported_fields(HeaderFields fields)
{
    MB_PRIVATE(Header);
    priv->fields_supported = fields & ALL_FIELDS;
}

// Fields

optional<std::string> Header::board_name() const
{
    MB_PRIVATE(const Header);
    return priv->field.board_name;
}

bool Header::set_board_name(optional<std::string> name)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::BoardName);
    priv->field.board_name = std::move(name);
    return true;
}

optional<std::string> Header::kernel_cmdline() const
{
    MB_PRIVATE(const Header);
    return priv->field.cmdline;
}

bool Header::set_kernel_cmdline(optional<std::string> cmdline)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::KernelCmdline);
    priv->field.cmdline = std::move(cmdline);
    return true;
}

optional<uint32_t> Header::page_size() const
{
    MB_PRIVATE(const Header);
    return priv->field.page_size;
}

bool Header::set_page_size(optional<uint32_t> page_size)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::PageSize);
    priv->field.page_size = std::move(page_size);
    return true;
}

optional<uint32_t> Header::kernel_address() const
{
    MB_PRIVATE(const Header);
    return priv->field.kernel_addr;
}

bool Header::set_kernel_address(optional<uint32_t> address)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::KernelAddress);
    priv->field.kernel_addr = std::move(address);
    return true;
}

optional<uint32_t> Header::ramdisk_address() const
{
    MB_PRIVATE(const Header);
    return priv->field.ramdisk_addr;
}

bool Header::set_ramdisk_address(optional<uint32_t> address)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::RamdiskAddress);
    priv->field.ramdisk_addr = std::move(address);
    return true;
}

optional<uint32_t> Header::secondboot_address() const
{
    MB_PRIVATE(const Header);
    return priv->field.second_addr;
}

bool Header::set_secondboot_address(optional<uint32_t> address)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::SecondbootAddress);
    priv->field.second_addr = std::move(address);
    return true;
}

optional<uint32_t> Header::kernel_tags_address() const
{
    MB_PRIVATE(const Header);
    return priv->field.tags_addr;
}

bool Header::set_kernel_tags_address(optional<uint32_t> address)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::KernelTagsAddress);
    priv->field.tags_addr = std::move(address);
    return true;
}

optional<uint32_t> Header::sony_ipl_address() const
{
    MB_PRIVATE(const Header);
    return priv->field.ipl_addr;
}

bool Header::set_sony_ipl_address(optional<uint32_t> address)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::SonyIplAddress);
    priv->field.ipl_addr = std::move(address);
    return true;
}

optional<uint32_t> Header::sony_rpm_address() const
{
    MB_PRIVATE(const Header);
    return priv->field.rpm_addr;
}

bool Header::set_sony_rpm_address(optional<uint32_t> address)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::SonyRpmAddress);
    priv->field.rpm_addr = std::move(address);
    return true;
}

optional<uint32_t> Header::sony_appsbl_address() const
{
    MB_PRIVATE(const Header);
    return priv->field.appsbl_addr;
}

bool Header::set_sony_appsbl_address(optional<uint32_t> address)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::SonyAppsblAddress);
    priv->field.appsbl_addr = std::move(address);
    return true;
}

optional<uint32_t> Header::entrypoint_address() const
{
    MB_PRIVATE(const Header);
    return priv->field.hdr_entrypoint;
}

bool Header::set_entrypoint_address(optional<uint32_t> address)
{
    MB_PRIVATE(Header);
    ENSURE_SUPPORTED(priv, HeaderField::Entrypoint);
    priv->field.hdr_entrypoint = std::move(address);
    return true;
}

}
}
