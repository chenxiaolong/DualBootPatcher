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

#define IS_SUPPORTED(FLAG) \
    (m_fields_supported & (FLAG))

#define ENSURE_SUPPORTED(FLAG) \
    do { \
        if (!IS_SUPPORTED(FLAG)) { \
            return false; \
        } \
    } while (0)


namespace mb::bootimg
{

Header::Header() noexcept
    : m_fields_supported(ALL_FIELDS)
{
}

Header::~Header() noexcept = default;

bool Header::operator==(const Header &rhs) const noexcept
{
    return m_kernel_addr == rhs.m_kernel_addr
            && m_ramdisk_addr == rhs.m_ramdisk_addr
            && m_second_addr == rhs.m_second_addr
            && m_tags_addr == rhs.m_tags_addr
            && m_ipl_addr == rhs.m_ipl_addr
            && m_rpm_addr == rhs.m_rpm_addr
            && m_appsbl_addr == rhs.m_appsbl_addr
            && m_page_size == rhs.m_page_size
            && m_board_name == rhs.m_board_name
            && m_cmdline == rhs.m_cmdline
            && m_hdr_kernel_size == rhs.m_hdr_kernel_size
            && m_hdr_ramdisk_size == rhs.m_hdr_ramdisk_size
            && m_hdr_second_size == rhs.m_hdr_second_size
            && m_hdr_dt_size == rhs.m_hdr_dt_size
            && m_hdr_unused == rhs.m_hdr_unused
            && m_hdr_id[0] == rhs.m_hdr_id[0]
            && m_hdr_id[1] == rhs.m_hdr_id[1]
            && m_hdr_id[2] == rhs.m_hdr_id[2]
            && m_hdr_id[3] == rhs.m_hdr_id[3]
            && m_hdr_id[4] == rhs.m_hdr_id[4]
            && m_hdr_id[5] == rhs.m_hdr_id[5]
            && m_hdr_id[6] == rhs.m_hdr_id[6]
            && m_hdr_id[7] == rhs.m_hdr_id[7]
            && m_hdr_entrypoint == rhs.m_hdr_entrypoint;
}

bool Header::operator!=(const Header &rhs) const noexcept
{
    return !(*this == rhs);
}

// Supported fields

HeaderFields Header::supported_fields() const
{
    return m_fields_supported;
}

void Header::set_supported_fields(HeaderFields fields)
{
    m_fields_supported = fields & ALL_FIELDS;
}

// Fields

std::optional<std::string> Header::board_name() const
{
    return m_board_name;
}

bool Header::set_board_name(std::optional<std::string> name)
{
    ENSURE_SUPPORTED(HeaderField::BoardName);
    m_board_name = std::move(name);
    return true;
}

std::optional<std::string> Header::kernel_cmdline() const
{
    return m_cmdline;
}

bool Header::set_kernel_cmdline(std::optional<std::string> cmdline)
{
    ENSURE_SUPPORTED(HeaderField::KernelCmdline);
    m_cmdline = std::move(cmdline);
    return true;
}

std::optional<uint32_t> Header::page_size() const
{
    return m_page_size;
}

bool Header::set_page_size(std::optional<uint32_t> page_size)
{
    ENSURE_SUPPORTED(HeaderField::PageSize);
    m_page_size = std::move(page_size);
    return true;
}

std::optional<uint32_t> Header::kernel_address() const
{
    return m_kernel_addr;
}

bool Header::set_kernel_address(std::optional<uint32_t> address)
{
    ENSURE_SUPPORTED(HeaderField::KernelAddress);
    m_kernel_addr = std::move(address);
    return true;
}

std::optional<uint32_t> Header::ramdisk_address() const
{
    return m_ramdisk_addr;
}

bool Header::set_ramdisk_address(std::optional<uint32_t> address)
{
    ENSURE_SUPPORTED(HeaderField::RamdiskAddress);
    m_ramdisk_addr = std::move(address);
    return true;
}

std::optional<uint32_t> Header::secondboot_address() const
{
    return m_second_addr;
}

bool Header::set_secondboot_address(std::optional<uint32_t> address)
{
    ENSURE_SUPPORTED(HeaderField::SecondbootAddress);
    m_second_addr = std::move(address);
    return true;
}

std::optional<uint32_t> Header::kernel_tags_address() const
{
    return m_tags_addr;
}

bool Header::set_kernel_tags_address(std::optional<uint32_t> address)
{
    ENSURE_SUPPORTED(HeaderField::KernelTagsAddress);
    m_tags_addr = std::move(address);
    return true;
}

std::optional<uint32_t> Header::sony_ipl_address() const
{
    return m_ipl_addr;
}

bool Header::set_sony_ipl_address(std::optional<uint32_t> address)
{
    ENSURE_SUPPORTED(HeaderField::SonyIplAddress);
    m_ipl_addr = std::move(address);
    return true;
}

std::optional<uint32_t> Header::sony_rpm_address() const
{
    return m_rpm_addr;
}

bool Header::set_sony_rpm_address(std::optional<uint32_t> address)
{
    ENSURE_SUPPORTED(HeaderField::SonyRpmAddress);
    m_rpm_addr = std::move(address);
    return true;
}

std::optional<uint32_t> Header::sony_appsbl_address() const
{
    return m_appsbl_addr;
}

bool Header::set_sony_appsbl_address(std::optional<uint32_t> address)
{
    ENSURE_SUPPORTED(HeaderField::SonyAppsblAddress);
    m_appsbl_addr = std::move(address);
    return true;
}

std::optional<uint32_t> Header::entrypoint_address() const
{
    return m_hdr_entrypoint;
}

bool Header::set_entrypoint_address(std::optional<uint32_t> address)
{
    ENSURE_SUPPORTED(HeaderField::Entrypoint);
    m_hdr_entrypoint = std::move(address);
    return true;
}

}
