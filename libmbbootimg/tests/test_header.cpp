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

#include <gtest/gtest.h>

#include <memory>

#include "mbbootimg/defs.h"
#include "mbbootimg/header.h"
#include "mbbootimg/header_p.h"

typedef std::unique_ptr<MbBiHeader, decltype(mb_bi_header_free) *> ScopedHeader;


TEST(BootImgHeaderTest, CheckDefaultValues)
{
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    // Check private fields
    ASSERT_EQ(header->fields_supported, MB_BI_HEADER_ALL_FIELDS);
    ASSERT_EQ(header->fields_set, 0);
    ASSERT_EQ(header->field.kernel_addr, 0);
    ASSERT_EQ(header->field.ramdisk_addr, 0);
    ASSERT_EQ(header->field.second_addr, 0);
    ASSERT_EQ(header->field.tags_addr, 0);
    ASSERT_EQ(header->field.ipl_addr, 0);
    ASSERT_EQ(header->field.rpm_addr, 0);
    ASSERT_EQ(header->field.appsbl_addr, 0);
    ASSERT_EQ(header->field.page_size, 0);
    ASSERT_EQ(header->field.board_name, nullptr);
    ASSERT_EQ(header->field.cmdline, nullptr);
    ASSERT_EQ(header->field.hdr_kernel_size, 0);
    ASSERT_EQ(header->field.hdr_ramdisk_size, 0);
    ASSERT_EQ(header->field.hdr_second_size, 0);
    ASSERT_EQ(header->field.hdr_dt_size, 0);
    ASSERT_EQ(header->field.hdr_unused, 0);
    ASSERT_EQ(header->field.hdr_id[0], 0);
    ASSERT_EQ(header->field.hdr_id[1], 0);
    ASSERT_EQ(header->field.hdr_id[2], 0);
    ASSERT_EQ(header->field.hdr_id[3], 0);
    ASSERT_EQ(header->field.hdr_id[4], 0);
    ASSERT_EQ(header->field.hdr_id[5], 0);
    ASSERT_EQ(header->field.hdr_id[6], 0);
    ASSERT_EQ(header->field.hdr_id[7], 0);
    ASSERT_EQ(header->field.hdr_entrypoint, 0);

    // Check public API
    ASSERT_EQ(mb_bi_header_board_name(header.get()), nullptr);
    ASSERT_EQ(mb_bi_header_kernel_cmdline(header.get()), nullptr);
    ASSERT_FALSE(mb_bi_header_page_size_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_page_size(header.get()), 0);
    ASSERT_FALSE(mb_bi_header_kernel_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_kernel_address(header.get()), 0);
    ASSERT_FALSE(mb_bi_header_ramdisk_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_ramdisk_address(header.get()), 0);
    ASSERT_FALSE(mb_bi_header_secondboot_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_secondboot_address(header.get()), 0);
    ASSERT_FALSE(mb_bi_header_kernel_tags_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_kernel_tags_address(header.get()), 0);
    ASSERT_FALSE(mb_bi_header_sony_ipl_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_sony_ipl_address(header.get()), 0);
    ASSERT_FALSE(mb_bi_header_sony_rpm_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_sony_rpm_address(header.get()), 0);
    ASSERT_FALSE(mb_bi_header_sony_appsbl_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_sony_appsbl_address(header.get()), 0);
    ASSERT_FALSE(mb_bi_header_entrypoint_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_entrypoint_address(header.get()), 0);
}

TEST(BootImgHeaderTest, CheckGettersSetters)
{
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    // Board name field

    ASSERT_EQ(mb_bi_header_set_board_name(header.get(), "test"), MB_BI_OK);
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_BOARD_NAME);
    ASSERT_STREQ(header->field.board_name, "test");
    ASSERT_STREQ(mb_bi_header_board_name(header.get()), "test");

    ASSERT_EQ(mb_bi_header_set_board_name(header.get(), nullptr), MB_BI_OK);
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_BOARD_NAME);
    ASSERT_EQ(header->field.board_name, nullptr);
    ASSERT_EQ(mb_bi_header_board_name(header.get()), nullptr);

    // Kernel cmdline field

    ASSERT_EQ(mb_bi_header_set_kernel_cmdline(header.get(), "test"), MB_BI_OK);
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_KERNEL_CMDLINE);
    ASSERT_STREQ(header->field.cmdline, "test");
    ASSERT_STREQ(mb_bi_header_kernel_cmdline(header.get()), "test");

    ASSERT_EQ(mb_bi_header_set_kernel_cmdline(header.get(), nullptr), MB_BI_OK);
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_KERNEL_CMDLINE);
    ASSERT_EQ(header->field.cmdline, nullptr);
    ASSERT_EQ(mb_bi_header_kernel_cmdline(header.get()), nullptr);

    // Page size field

    ASSERT_EQ(mb_bi_header_set_page_size(header.get(), 1234), MB_BI_OK);
    ASSERT_TRUE(mb_bi_header_page_size_is_set(header.get()));
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_PAGE_SIZE);
    ASSERT_EQ(header->field.page_size, 1234);
    ASSERT_EQ(mb_bi_header_page_size(header.get()), 1234);

    ASSERT_EQ(mb_bi_header_unset_page_size(header.get()), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_page_size_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_PAGE_SIZE);
    ASSERT_EQ(header->field.page_size, 0);
    ASSERT_EQ(mb_bi_header_page_size(header.get()), 0);

    // Kernel address field

    ASSERT_EQ(mb_bi_header_set_kernel_address(header.get(), 1234), MB_BI_OK);
    ASSERT_TRUE(mb_bi_header_kernel_address_is_set(header.get()));
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_KERNEL_ADDRESS);
    ASSERT_EQ(header->field.kernel_addr, 1234);
    ASSERT_EQ(mb_bi_header_kernel_address(header.get()), 1234);

    ASSERT_EQ(mb_bi_header_unset_kernel_address(header.get()), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_kernel_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_KERNEL_ADDRESS);
    ASSERT_EQ(header->field.kernel_addr, 0);
    ASSERT_EQ(mb_bi_header_kernel_address(header.get()), 0);

    // Ramdisk address field

    ASSERT_EQ(mb_bi_header_set_ramdisk_address(header.get(), 1234), MB_BI_OK);
    ASSERT_TRUE(mb_bi_header_ramdisk_address_is_set(header.get()));
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_RAMDISK_ADDRESS);
    ASSERT_EQ(header->field.ramdisk_addr, 1234);
    ASSERT_EQ(mb_bi_header_ramdisk_address(header.get()), 1234);

    ASSERT_EQ(mb_bi_header_unset_ramdisk_address(header.get()), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_ramdisk_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_RAMDISK_ADDRESS);
    ASSERT_EQ(header->field.ramdisk_addr, 0);
    ASSERT_EQ(mb_bi_header_ramdisk_address(header.get()), 0);

    // Second bootloader address field

    ASSERT_EQ(mb_bi_header_set_secondboot_address(header.get(), 1234),
              MB_BI_OK);
    ASSERT_TRUE(mb_bi_header_secondboot_address_is_set(header.get()));
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS);
    ASSERT_EQ(header->field.second_addr, 1234);
    ASSERT_EQ(mb_bi_header_secondboot_address(header.get()), 1234);

    ASSERT_EQ(mb_bi_header_unset_secondboot_address(header.get()), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_secondboot_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS);
    ASSERT_EQ(header->field.second_addr, 0);
    ASSERT_EQ(mb_bi_header_secondboot_address(header.get()), 0);

    // Kernel tags address field

    ASSERT_EQ(mb_bi_header_set_kernel_tags_address(header.get(), 1234),
              MB_BI_OK);
    ASSERT_TRUE(mb_bi_header_kernel_tags_address_is_set(header.get()));
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS);
    ASSERT_EQ(header->field.tags_addr, 1234);
    ASSERT_EQ(mb_bi_header_kernel_tags_address(header.get()), 1234);

    ASSERT_EQ(mb_bi_header_unset_kernel_tags_address(header.get()), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_kernel_tags_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS);
    ASSERT_EQ(header->field.tags_addr, 0);
    ASSERT_EQ(mb_bi_header_kernel_tags_address(header.get()), 0);

    // Sony IPL address field

    ASSERT_EQ(mb_bi_header_set_sony_ipl_address(header.get(), 1234), MB_BI_OK);
    ASSERT_TRUE(mb_bi_header_sony_ipl_address_is_set(header.get()));
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS);
    ASSERT_EQ(header->field.ipl_addr, 1234);
    ASSERT_EQ(mb_bi_header_sony_ipl_address(header.get()), 1234);

    ASSERT_EQ(mb_bi_header_unset_sony_ipl_address(header.get()), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_sony_ipl_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS);
    ASSERT_EQ(header->field.ipl_addr, 0);
    ASSERT_EQ(mb_bi_header_sony_ipl_address(header.get()), 0);

    // Sony RPM address field

    ASSERT_EQ(mb_bi_header_set_sony_rpm_address(header.get(), 1234), MB_BI_OK);
    ASSERT_TRUE(mb_bi_header_sony_rpm_address_is_set(header.get()));
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS);
    ASSERT_EQ(header->field.rpm_addr, 1234);
    ASSERT_EQ(mb_bi_header_sony_rpm_address(header.get()), 1234);

    ASSERT_EQ(mb_bi_header_unset_sony_rpm_address(header.get()), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_sony_rpm_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS);
    ASSERT_EQ(header->field.rpm_addr, 0);
    ASSERT_EQ(mb_bi_header_sony_rpm_address(header.get()), 0);

    // Sony APPSBL address field

    ASSERT_EQ(mb_bi_header_set_sony_appsbl_address(header.get(), 1234),
              MB_BI_OK);
    ASSERT_TRUE(mb_bi_header_sony_appsbl_address_is_set(header.get()));
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS);
    ASSERT_EQ(header->field.appsbl_addr, 1234);
    ASSERT_EQ(mb_bi_header_sony_appsbl_address(header.get()), 1234);

    ASSERT_EQ(mb_bi_header_unset_sony_appsbl_address(header.get()), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_sony_appsbl_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS);
    ASSERT_EQ(header->field.appsbl_addr, 0);
    ASSERT_EQ(mb_bi_header_sony_appsbl_address(header.get()), 0);

    // Entrypoint address field

    ASSERT_EQ(mb_bi_header_set_entrypoint_address(header.get(), 1234), MB_BI_OK);
    ASSERT_TRUE(mb_bi_header_entrypoint_address_is_set(header.get()));
    ASSERT_TRUE(header->fields_set & MB_BI_HEADER_FIELD_ENTRYPOINT);
    ASSERT_EQ(header->field.hdr_entrypoint, 1234);
    ASSERT_EQ(mb_bi_header_entrypoint_address(header.get()), 1234);

    ASSERT_EQ(mb_bi_header_unset_entrypoint_address(header.get()), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_entrypoint_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_ENTRYPOINT);
    ASSERT_EQ(header->field.hdr_entrypoint, 0);
    ASSERT_EQ(mb_bi_header_entrypoint_address(header.get()), 0);
}

TEST(BootImgHeaderTest, CheckSettingUnsupported)
{
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    // Board name field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_BOARD_NAME;

    ASSERT_EQ(mb_bi_header_set_board_name(header.get(), "test"),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_BOARD_NAME);
    ASSERT_EQ(header->field.board_name, nullptr);
    ASSERT_EQ(mb_bi_header_board_name(header.get()), nullptr);
    ASSERT_EQ(mb_bi_header_set_board_name(header.get(), nullptr),
              MB_BI_UNSUPPORTED);

    // Kernel cmdline field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_KERNEL_CMDLINE;

    ASSERT_EQ(mb_bi_header_set_kernel_cmdline(header.get(), "test"),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_KERNEL_CMDLINE);
    ASSERT_EQ(header->field.cmdline, nullptr);
    ASSERT_EQ(mb_bi_header_kernel_cmdline(header.get()), nullptr);
    ASSERT_EQ(mb_bi_header_set_kernel_cmdline(header.get(), nullptr),
              MB_BI_UNSUPPORTED);

    // Page size field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_PAGE_SIZE;

    ASSERT_EQ(mb_bi_header_set_page_size(header.get(), 1234),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(mb_bi_header_page_size_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_PAGE_SIZE);
    ASSERT_EQ(header->field.page_size, 0);
    ASSERT_EQ(mb_bi_header_page_size(header.get()), 0);
    ASSERT_EQ(mb_bi_header_unset_page_size(header.get()), MB_BI_UNSUPPORTED);

    // Kernel address field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_KERNEL_ADDRESS;

    ASSERT_EQ(mb_bi_header_set_kernel_address(header.get(), 1234),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(mb_bi_header_kernel_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_KERNEL_ADDRESS);
    ASSERT_EQ(header->field.kernel_addr, 0);
    ASSERT_EQ(mb_bi_header_kernel_address(header.get()), 0);
    ASSERT_EQ(mb_bi_header_unset_kernel_address(header.get()),
              MB_BI_UNSUPPORTED);

    // Ramdisk address field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_RAMDISK_ADDRESS;

    ASSERT_EQ(mb_bi_header_set_ramdisk_address(header.get(), 1234),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(mb_bi_header_ramdisk_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_RAMDISK_ADDRESS);
    ASSERT_EQ(header->field.ramdisk_addr, 0);
    ASSERT_EQ(mb_bi_header_ramdisk_address(header.get()), 0);
    ASSERT_EQ(mb_bi_header_unset_ramdisk_address(header.get()),
              MB_BI_UNSUPPORTED);

    // Second bootloader address field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS;

    ASSERT_EQ(mb_bi_header_set_secondboot_address(header.get(), 1234),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(mb_bi_header_secondboot_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS);
    ASSERT_EQ(header->field.second_addr, 0);
    ASSERT_EQ(mb_bi_header_secondboot_address(header.get()), 0);
    ASSERT_EQ(mb_bi_header_unset_secondboot_address(header.get()),
              MB_BI_UNSUPPORTED);

    // Kernel tags address field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS;

    ASSERT_EQ(mb_bi_header_set_kernel_tags_address(header.get(), 1234),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(mb_bi_header_kernel_tags_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS);
    ASSERT_EQ(header->field.tags_addr, 0);
    ASSERT_EQ(mb_bi_header_kernel_tags_address(header.get()), 0);
    ASSERT_EQ(mb_bi_header_unset_kernel_tags_address(header.get()),
              MB_BI_UNSUPPORTED);

    // Sony IPL address field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS;

    ASSERT_EQ(mb_bi_header_set_sony_ipl_address(header.get(), 1234),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(mb_bi_header_sony_ipl_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS);
    ASSERT_EQ(header->field.ipl_addr, 0);
    ASSERT_EQ(mb_bi_header_sony_ipl_address(header.get()), 0);
    ASSERT_EQ(mb_bi_header_unset_sony_ipl_address(header.get()),
              MB_BI_UNSUPPORTED);

    // Sony RPM address field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS;

    ASSERT_EQ(mb_bi_header_set_sony_rpm_address(header.get(), 1234),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(mb_bi_header_sony_rpm_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS);
    ASSERT_EQ(header->field.rpm_addr, 0);
    ASSERT_EQ(mb_bi_header_sony_rpm_address(header.get()), 0);
    ASSERT_EQ(mb_bi_header_unset_sony_rpm_address(header.get()),
              MB_BI_UNSUPPORTED);

    // Sony APPSBL address field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS;

    ASSERT_EQ(mb_bi_header_set_sony_appsbl_address(header.get(), 1234),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(mb_bi_header_sony_appsbl_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS);
    ASSERT_EQ(header->field.appsbl_addr, 0);
    ASSERT_EQ(mb_bi_header_sony_appsbl_address(header.get()), 0);
    ASSERT_EQ(mb_bi_header_unset_sony_appsbl_address(header.get()),
              MB_BI_UNSUPPORTED);

    // Entrypoint address field

    header->fields_supported &= ~MB_BI_HEADER_FIELD_ENTRYPOINT;

    ASSERT_EQ(mb_bi_header_set_entrypoint_address(header.get(), 1234),
              MB_BI_UNSUPPORTED);
    ASSERT_FALSE(mb_bi_header_entrypoint_address_is_set(header.get()));
    ASSERT_FALSE(header->fields_set & MB_BI_HEADER_FIELD_ENTRYPOINT);
    ASSERT_EQ(header->field.hdr_entrypoint, 0);
    ASSERT_EQ(mb_bi_header_entrypoint_address(header.get()), 0);
    ASSERT_EQ(mb_bi_header_unset_entrypoint_address(header.get()),
              MB_BI_UNSUPPORTED);
}

TEST(BootImgHeaderTest, CheckClone)
{
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    // Set fields
    header->fields_supported = header->fields_set = MB_BI_HEADER_ALL_FIELDS;
    header->field.kernel_addr = 0x1000;
    header->field.ramdisk_addr = 0x2000;
    header->field.second_addr = 0x3000;
    header->field.tags_addr = 0x4000;
    header->field.ipl_addr = 0x5000;
    header->field.rpm_addr = 0x6000;
    header->field.appsbl_addr = 0x7000;
    header->field.page_size = 2048;
    header->field.board_name = strdup("test");
    ASSERT_NE(header->field.board_name, nullptr);
    header->field.cmdline = strdup("test2");
    ASSERT_NE(header->field.cmdline, nullptr);
    header->field.hdr_kernel_size = 1024;
    header->field.hdr_ramdisk_size = 2048;
    header->field.hdr_second_size = 4096;
    header->field.hdr_dt_size = 8192;
    header->field.hdr_unused = 0x12345678;
    header->field.hdr_id[0] = 0x11111111;
    header->field.hdr_id[1] = 0x22222222;
    header->field.hdr_id[2] = 0x33333333;
    header->field.hdr_id[3] = 0x44444444;
    header->field.hdr_id[4] = 0x55555555;
    header->field.hdr_id[5] = 0x66666666;
    header->field.hdr_id[6] = 0x77777777;
    header->field.hdr_id[7] = 0x88888888;
    header->field.hdr_entrypoint = 0x8000;

    ScopedHeader dup(mb_bi_header_clone(header.get()), mb_bi_header_free);
    ASSERT_TRUE(!!dup);

    // Compare values
    ASSERT_EQ(header->fields_supported, dup->fields_supported);
    ASSERT_EQ(header->fields_set, dup->fields_set);
    ASSERT_EQ(header->field.kernel_addr, dup->field.kernel_addr);
    ASSERT_EQ(header->field.ramdisk_addr, dup->field.ramdisk_addr);
    ASSERT_EQ(header->field.second_addr, dup->field.second_addr);
    ASSERT_EQ(header->field.tags_addr, dup->field.tags_addr);
    ASSERT_EQ(header->field.ipl_addr, dup->field.ipl_addr);
    ASSERT_EQ(header->field.rpm_addr, dup->field.rpm_addr);
    ASSERT_EQ(header->field.appsbl_addr, dup->field.appsbl_addr);
    ASSERT_EQ(header->field.page_size, dup->field.page_size);
    ASSERT_NE(header->field.board_name, dup->field.board_name);
    ASSERT_STREQ(header->field.board_name, dup->field.board_name);
    ASSERT_NE(header->field.cmdline, dup->field.cmdline);
    ASSERT_STREQ(header->field.cmdline, dup->field.cmdline);
    ASSERT_EQ(header->field.hdr_kernel_size, dup->field.hdr_kernel_size);
    ASSERT_EQ(header->field.hdr_ramdisk_size, dup->field.hdr_ramdisk_size);
    ASSERT_EQ(header->field.hdr_second_size, dup->field.hdr_second_size);
    ASSERT_EQ(header->field.hdr_dt_size, dup->field.hdr_dt_size);
    ASSERT_EQ(header->field.hdr_unused, dup->field.hdr_unused);
    ASSERT_EQ(header->field.hdr_id[0], dup->field.hdr_id[0]);
    ASSERT_EQ(header->field.hdr_id[1], dup->field.hdr_id[1]);
    ASSERT_EQ(header->field.hdr_id[2], dup->field.hdr_id[2]);
    ASSERT_EQ(header->field.hdr_id[3], dup->field.hdr_id[3]);
    ASSERT_EQ(header->field.hdr_id[4], dup->field.hdr_id[4]);
    ASSERT_EQ(header->field.hdr_id[5], dup->field.hdr_id[5]);
    ASSERT_EQ(header->field.hdr_id[6], dup->field.hdr_id[6]);
    ASSERT_EQ(header->field.hdr_id[7], dup->field.hdr_id[7]);
    ASSERT_EQ(header->field.hdr_entrypoint, dup->field.hdr_entrypoint);
}

TEST(BootImgHeaderTest, CheckClear)
{
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    // Set fields
    header->fields_supported = header->fields_set = MB_BI_HEADER_ALL_FIELDS;
    header->field.kernel_addr = 0x1000;
    header->field.ramdisk_addr = 0x2000;
    header->field.second_addr = 0x3000;
    header->field.tags_addr = 0x4000;
    header->field.ipl_addr = 0x5000;
    header->field.rpm_addr = 0x6000;
    header->field.appsbl_addr = 0x7000;
    header->field.page_size = 2048;
    header->field.board_name = strdup("test");
    ASSERT_NE(header->field.board_name, nullptr);
    header->field.cmdline = strdup("test2");
    ASSERT_NE(header->field.cmdline, nullptr);
    header->field.hdr_kernel_size = 1024;
    header->field.hdr_ramdisk_size = 2048;
    header->field.hdr_second_size = 4096;
    header->field.hdr_dt_size = 8192;
    header->field.hdr_unused = 0x12345678;
    header->field.hdr_id[0] = 0x11111111;
    header->field.hdr_id[1] = 0x22222222;
    header->field.hdr_id[2] = 0x33333333;
    header->field.hdr_id[3] = 0x44444444;
    header->field.hdr_id[4] = 0x55555555;
    header->field.hdr_id[5] = 0x66666666;
    header->field.hdr_id[6] = 0x77777777;
    header->field.hdr_id[7] = 0x88888888;
    header->field.hdr_entrypoint = 0x8000;

    mb_bi_header_clear(header.get());

    // Supported fields will not be cleared
    ASSERT_EQ(header->fields_supported, MB_BI_HEADER_ALL_FIELDS);
    ASSERT_EQ(header->fields_set, 0);
    ASSERT_EQ(header->field.kernel_addr, 0);
    ASSERT_EQ(header->field.ramdisk_addr, 0);
    ASSERT_EQ(header->field.second_addr, 0);
    ASSERT_EQ(header->field.tags_addr, 0);
    ASSERT_EQ(header->field.ipl_addr, 0);
    ASSERT_EQ(header->field.rpm_addr, 0);
    ASSERT_EQ(header->field.appsbl_addr, 0);
    ASSERT_EQ(header->field.page_size, 0);
    ASSERT_EQ(header->field.board_name, nullptr);
    ASSERT_EQ(header->field.cmdline, nullptr);
    ASSERT_EQ(header->field.hdr_kernel_size, 0);
    ASSERT_EQ(header->field.hdr_ramdisk_size, 0);
    ASSERT_EQ(header->field.hdr_second_size, 0);
    ASSERT_EQ(header->field.hdr_dt_size, 0);
    ASSERT_EQ(header->field.hdr_unused, 0);
    ASSERT_EQ(header->field.hdr_id[0], 0);
    ASSERT_EQ(header->field.hdr_id[1], 0);
    ASSERT_EQ(header->field.hdr_id[2], 0);
    ASSERT_EQ(header->field.hdr_id[3], 0);
    ASSERT_EQ(header->field.hdr_id[4], 0);
    ASSERT_EQ(header->field.hdr_id[5], 0);
    ASSERT_EQ(header->field.hdr_id[6], 0);
    ASSERT_EQ(header->field.hdr_id[7], 0);
    ASSERT_EQ(header->field.hdr_entrypoint, 0);
}
