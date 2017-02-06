/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbbootimg/header.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "mbbootimg/defs.h"
#include "mbbootimg/header_p.h"
#include "mbbootimg/macros_p.h"


MB_BEGIN_C_DECLS

MbBiHeader * mb_bi_header_new()
{
    MbBiHeader *header = static_cast<MbBiHeader *>(calloc(1, sizeof(*header)));
    if (header) {
        header->fields_supported = MB_BI_HEADER_ALL_FIELDS;
    }
    return header;
}

void mb_bi_header_free(MbBiHeader *header)
{
    mb_bi_header_clear(header);
    free(header);
}

void mb_bi_header_clear(MbBiHeader *header)
{
    if (header) {
        uint64_t supported = header->fields_supported;
        free(header->field.board_name);
        free(header->field.cmdline);
        memset(header, 0, sizeof(*header));
        header->fields_supported = supported;
    }
}

MbBiHeader * mb_bi_header_clone(MbBiHeader *header)
{
    MbBiHeader *dup;

    dup = mb_bi_header_new();
    if (!dup) {
        return nullptr;
    }

    // Copy global options
    dup->fields_supported = header->fields_supported;
    dup->fields_set = header->fields_set;

    // Shallow copy trivial fields
    dup->field.kernel_addr = header->field.kernel_addr;
    dup->field.ramdisk_addr = header->field.ramdisk_addr;
    dup->field.second_addr = header->field.second_addr;
    dup->field.tags_addr = header->field.tags_addr;
    dup->field.ipl_addr = header->field.ipl_addr;
    dup->field.rpm_addr = header->field.rpm_addr;
    dup->field.appsbl_addr = header->field.appsbl_addr;
    dup->field.page_size = header->field.page_size;
    dup->field.hdr_kernel_size = header->field.hdr_kernel_size;
    dup->field.hdr_ramdisk_size = header->field.hdr_ramdisk_size;
    dup->field.hdr_second_size = header->field.hdr_second_size;
    dup->field.hdr_dt_size = header->field.hdr_dt_size;
    dup->field.hdr_unused = header->field.hdr_unused;
    memcpy(dup->field.hdr_id, header->field.hdr_id,
           sizeof(header->field.hdr_id));
    dup->field.hdr_entrypoint = header->field.hdr_entrypoint;

    // Deep copy strings
    bool deep_copy_error =
            (header->field.board_name
                    && !(dup->field.board_name = strdup(header->field.board_name)))
            || (header->field.cmdline
                    && !(dup->field.cmdline = strdup(header->field.cmdline)));

    if (deep_copy_error) {
        mb_bi_header_free(dup);
        return nullptr;
    }

    return dup;
}

uint64_t mb_bi_header_supported_fields(MbBiHeader *header)
{
    return header->fields_supported;
}

void mb_bi_header_set_supported_fields(MbBiHeader *header, uint64_t fields)
{
    header->fields_supported = fields & MB_BI_HEADER_ALL_FIELDS;
}

// Fields

const char * mb_bi_header_board_name(MbBiHeader *header)
{
    return header->field.board_name;
}

int mb_bi_header_set_board_name(MbBiHeader *header, const char *name)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_BOARD_NAME);
    SET_STRING_FIELD(header, MB_BI_HEADER_FIELD_BOARD_NAME, board_name, name);
    return MB_BI_OK;
}

const char * mb_bi_header_kernel_cmdline(MbBiHeader *header)
{
    return header->field.cmdline;
}

int mb_bi_header_set_kernel_cmdline(MbBiHeader *header, const char *cmdline)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_KERNEL_CMDLINE);
    SET_STRING_FIELD(header, MB_BI_HEADER_FIELD_KERNEL_CMDLINE,
                     cmdline, cmdline);
    return MB_BI_OK;
}

int mb_bi_header_page_size_is_set(MbBiHeader *header)
{
    return IS_SET(header, MB_BI_HEADER_FIELD_PAGE_SIZE);
}

uint32_t mb_bi_header_page_size(MbBiHeader *header)
{
    return header->field.page_size;
}

int mb_bi_header_set_page_size(MbBiHeader *header, uint32_t page_size)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_PAGE_SIZE);
    SET_FIELD(header, MB_BI_HEADER_FIELD_PAGE_SIZE, page_size, page_size);
    return MB_BI_OK;
}

int mb_bi_header_unset_page_size(MbBiHeader *header)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_PAGE_SIZE);
    UNSET_FIELD(header, MB_BI_HEADER_FIELD_PAGE_SIZE, page_size, 0);
    return MB_BI_OK;
}

int mb_bi_header_kernel_address_is_set(MbBiHeader *header)
{
    return IS_SET(header, MB_BI_HEADER_FIELD_KERNEL_ADDRESS);
}

uint32_t mb_bi_header_kernel_address(MbBiHeader *header)
{
    return header->field.kernel_addr;
}

int mb_bi_header_set_kernel_address(MbBiHeader *header, uint32_t address)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_KERNEL_ADDRESS);
    SET_FIELD(header, MB_BI_HEADER_FIELD_KERNEL_ADDRESS, kernel_addr, address);
    return MB_BI_OK;
}

int mb_bi_header_unset_kernel_address(MbBiHeader *header)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_KERNEL_ADDRESS);
    UNSET_FIELD(header, MB_BI_HEADER_FIELD_KERNEL_ADDRESS, kernel_addr, 0);
    return MB_BI_OK;
}

int mb_bi_header_ramdisk_address_is_set(MbBiHeader *header)
{
    return IS_SET(header, MB_BI_HEADER_FIELD_RAMDISK_ADDRESS);
}

uint32_t mb_bi_header_ramdisk_address(MbBiHeader *header)
{
    return header->field.ramdisk_addr;
}

int mb_bi_header_set_ramdisk_address(MbBiHeader *header, uint32_t address)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_RAMDISK_ADDRESS);
    SET_FIELD(header, MB_BI_HEADER_FIELD_RAMDISK_ADDRESS,
              ramdisk_addr, address);
    return MB_BI_OK;
}

int mb_bi_header_unset_ramdisk_address(MbBiHeader *header)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_RAMDISK_ADDRESS);
    UNSET_FIELD(header, MB_BI_HEADER_FIELD_RAMDISK_ADDRESS,
                ramdisk_addr, 0);
    return MB_BI_OK;
}

int mb_bi_header_secondboot_address_is_set(MbBiHeader *header)
{
    return IS_SET(header, MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS);
}

uint32_t mb_bi_header_secondboot_address(MbBiHeader *header)
{
    return header->field.second_addr;
}

int mb_bi_header_set_secondboot_address(MbBiHeader *header, uint32_t address)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS);
    SET_FIELD(header, MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS,
              second_addr, address);
    return MB_BI_OK;
}

int mb_bi_header_unset_secondboot_address(MbBiHeader *header)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS);
    UNSET_FIELD(header, MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS, second_addr, 0);
    return MB_BI_OK;
}

int mb_bi_header_kernel_tags_address_is_set(MbBiHeader *header)
{
    return IS_SET(header, MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS);
}

uint32_t mb_bi_header_kernel_tags_address(MbBiHeader *header)
{
    return header->field.tags_addr;
}

int mb_bi_header_set_kernel_tags_address(MbBiHeader *header, uint32_t address)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS);
    SET_FIELD(header, MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS,
              tags_addr, address);
    return MB_BI_OK;
}

int mb_bi_header_unset_kernel_tags_address(MbBiHeader *header)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS);
    UNSET_FIELD(header, MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS, tags_addr, 0);
    return MB_BI_OK;
}

int mb_bi_header_sony_ipl_address_is_set(MbBiHeader *header)
{
    return IS_SET(header, MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS);
}

uint32_t mb_bi_header_sony_ipl_address(MbBiHeader *header)
{
    return header->field.ipl_addr;
}

int mb_bi_header_set_sony_ipl_address(MbBiHeader *header, uint32_t address)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS);
    SET_FIELD(header, MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS, ipl_addr, address);
    return MB_BI_OK;
}

int mb_bi_header_unset_sony_ipl_address(MbBiHeader *header)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS);
    UNSET_FIELD(header, MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS, ipl_addr, 0);
    return MB_BI_OK;
}

int mb_bi_header_sony_rpm_address_is_set(MbBiHeader *header)
{
    return IS_SET(header, MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS);
}

uint32_t mb_bi_header_sony_rpm_address(MbBiHeader *header)
{
    return header->field.rpm_addr;
}

int mb_bi_header_set_sony_rpm_address(MbBiHeader *header, uint32_t address)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS);
    SET_FIELD(header, MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS, rpm_addr, address);
    return MB_BI_OK;
}

int mb_bi_header_unset_sony_rpm_address(MbBiHeader *header)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS);
    UNSET_FIELD(header, MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS, rpm_addr, 0);
    return MB_BI_OK;
}

int mb_bi_header_sony_appsbl_address_is_set(MbBiHeader *header)
{
    return IS_SET(header, MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS);
}

uint32_t mb_bi_header_sony_appsbl_address(MbBiHeader *header)
{
    return header->field.appsbl_addr;
}

int mb_bi_header_set_sony_appsbl_address(MbBiHeader *header, uint32_t address)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS);
    SET_FIELD(header, MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS,
              appsbl_addr, address);
    return MB_BI_OK;
}

int mb_bi_header_unset_sony_appsbl_address(MbBiHeader *header)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS);
    UNSET_FIELD(header, MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS, appsbl_addr, 0);
    return MB_BI_OK;
}

int mb_bi_header_entrypoint_address_is_set(MbBiHeader *header)
{
    return IS_SET(header, MB_BI_HEADER_FIELD_ENTRYPOINT);
}

uint32_t mb_bi_header_entrypoint_address(MbBiHeader *header)
{
    return header->field.hdr_entrypoint;
}

int mb_bi_header_set_entrypoint_address(MbBiHeader *header, uint32_t address)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_ENTRYPOINT);
    SET_FIELD(header, MB_BI_HEADER_FIELD_ENTRYPOINT, hdr_entrypoint, address);
    return MB_BI_OK;
}

int mb_bi_header_unset_entrypoint_address(MbBiHeader *header)
{
    ENSURE_SUPPORTED(header, MB_BI_HEADER_FIELD_ENTRYPOINT);
    UNSET_FIELD(header, MB_BI_HEADER_FIELD_ENTRYPOINT, hdr_entrypoint, 0);
    return MB_BI_OK;
}

MB_END_C_DECLS
