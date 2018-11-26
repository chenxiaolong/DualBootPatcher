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

#include <gtest/gtest.h>

#include <memory>

#include "mbbootimg/header.h"

using namespace mb::bootimg;


TEST(BootImgHeaderTest, CheckDefaultValues)
{
    Header header;

    ASSERT_EQ(header.supported_fields(), ALL_FIELDS);

    ASSERT_FALSE(header.board_name());
    ASSERT_FALSE(header.kernel_cmdline());
    ASSERT_FALSE(header.page_size());
    ASSERT_FALSE(header.kernel_address());
    ASSERT_FALSE(header.ramdisk_address());
    ASSERT_FALSE(header.secondboot_address());
    ASSERT_FALSE(header.kernel_tags_address());
    ASSERT_FALSE(header.sony_ipl_address());
    ASSERT_FALSE(header.sony_rpm_address());
    ASSERT_FALSE(header.sony_appsbl_address());
    ASSERT_FALSE(header.entrypoint_address());
}

TEST(BootImgHeaderTest, CheckGettersSetters)
{
    Header header;

    // Board name field

    header.set_board_name({"test"});
    auto board_name = header.board_name();
    ASSERT_TRUE(board_name);
    ASSERT_EQ(*board_name, "test");

    header.set_board_name({});
    ASSERT_FALSE(header.board_name());

    // Kernel cmdline field

    header.set_kernel_cmdline({"test"});
    auto kernel_cmdline = header.kernel_cmdline();
    ASSERT_TRUE(kernel_cmdline);
    ASSERT_EQ(*kernel_cmdline, "test");

    header.set_kernel_cmdline({});
    ASSERT_FALSE(header.kernel_cmdline());

    // Page size field

    header.set_page_size(1234);
    auto page_size = header.page_size();
    ASSERT_TRUE(page_size);
    ASSERT_EQ(*page_size, 1234u);

    header.set_page_size({});
    ASSERT_FALSE(header.page_size());

    // Kernel address field

    header.set_kernel_address(1234);
    auto kernel_address = header.kernel_address();
    ASSERT_TRUE(kernel_address);
    ASSERT_EQ(*kernel_address, 1234u);

    header.set_kernel_address({});
    ASSERT_FALSE(header.kernel_address());

    // Ramdisk address field

    header.set_ramdisk_address(1234);
    auto ramdisk_address = header.ramdisk_address();
    ASSERT_TRUE(ramdisk_address);
    ASSERT_EQ(*ramdisk_address, 1234u);

    header.set_ramdisk_address({});
    ASSERT_FALSE(header.ramdisk_address());

    // Second bootloader address field

    header.set_secondboot_address(1234);
    auto secondboot_address = header.secondboot_address();
    ASSERT_TRUE(secondboot_address);
    ASSERT_EQ(*secondboot_address, 1234u);

    header.set_secondboot_address({});
    ASSERT_FALSE(header.secondboot_address());

    // Kernel tags address field

    header.set_kernel_tags_address(1234);
    auto kernel_tags_address = header.kernel_tags_address();
    ASSERT_TRUE(kernel_tags_address);
    ASSERT_EQ(*kernel_tags_address, 1234u);

    header.set_kernel_tags_address({});
    ASSERT_FALSE(header.kernel_tags_address());

    // Sony IPL address field

    header.set_sony_ipl_address(1234);
    auto sony_ipl_address = header.sony_ipl_address();
    ASSERT_TRUE(sony_ipl_address);
    ASSERT_EQ(*sony_ipl_address, 1234u);

    header.set_sony_ipl_address({});
    ASSERT_FALSE(header.sony_ipl_address());

    // Sony RPM address field

    header.set_sony_rpm_address(1234);
    auto sony_rpm_address = header.sony_rpm_address();
    ASSERT_TRUE(sony_rpm_address);
    ASSERT_EQ(*sony_rpm_address, 1234u);

    header.set_sony_rpm_address({});
    ASSERT_FALSE(header.sony_rpm_address());

    // Sony APPSBL address field

    header.set_sony_appsbl_address(1234);
    auto sony_appsbl_address = header.sony_appsbl_address();
    ASSERT_TRUE(sony_appsbl_address);
    ASSERT_EQ(*sony_appsbl_address, 1234u);

    header.set_sony_appsbl_address({});
    ASSERT_FALSE(header.sony_appsbl_address());

    // Entrypoint address field

    header.set_entrypoint_address(1234);
    auto entrypoint_address = header.entrypoint_address();
    ASSERT_TRUE(entrypoint_address);
    ASSERT_EQ(*entrypoint_address, 1234u);

    header.set_entrypoint_address({});
    ASSERT_FALSE(header.entrypoint_address());
}

TEST(BootImgHeaderTest, CheckSettingUnsupported)
{
    Header header;

    // Board name field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::BoardName, false));

    ASSERT_FALSE(header.set_board_name({"test"}));
    ASSERT_FALSE(header.board_name());
    ASSERT_FALSE(header.set_board_name({}));

    // Kernel cmdline field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::KernelCmdline, false));

    ASSERT_FALSE(header.set_kernel_cmdline({"test"}));
    ASSERT_FALSE(header.kernel_cmdline());
    ASSERT_FALSE(header.set_kernel_cmdline({}));

    // Page size field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::PageSize, false));

    ASSERT_FALSE(header.set_page_size(1234));
    ASSERT_FALSE(header.page_size());
    ASSERT_FALSE(header.set_page_size({}));

    // Kernel address field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::KernelAddress, false));

    ASSERT_FALSE(header.set_kernel_address(1234));
    ASSERT_FALSE(header.kernel_address());
    ASSERT_FALSE(header.set_kernel_address({}));

    // Ramdisk address field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::RamdiskAddress, false));

    ASSERT_FALSE(header.set_ramdisk_address(1234));
    ASSERT_FALSE(header.ramdisk_address());
    ASSERT_FALSE(header.set_ramdisk_address({}));

    // Second bootloader address field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::SecondbootAddress, false));

    ASSERT_FALSE(header.set_secondboot_address(1234));
    ASSERT_FALSE(header.secondboot_address());
    ASSERT_FALSE(header.set_secondboot_address({}));

    // Kernel tags address field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::KernelTagsAddress, false));

    ASSERT_FALSE(header.set_kernel_tags_address(1234));
    ASSERT_FALSE(header.kernel_tags_address());
    ASSERT_FALSE(header.set_kernel_tags_address({}));

    // Sony IPL address field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::SonyIplAddress, false));

    ASSERT_FALSE(header.set_sony_ipl_address(1234));
    ASSERT_FALSE(header.sony_ipl_address());
    ASSERT_FALSE(header.set_sony_ipl_address({}));

    // Sony RPM address field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::SonyRpmAddress, false));

    ASSERT_FALSE(header.set_sony_rpm_address(1234));
    ASSERT_FALSE(header.sony_rpm_address());
    ASSERT_FALSE(header.set_sony_rpm_address({}));

    // Sony APPSBL address field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::SonyAppsblAddress, false));

    ASSERT_FALSE(header.set_sony_appsbl_address(1234));
    ASSERT_FALSE(header.sony_appsbl_address());
    ASSERT_FALSE(header.set_sony_appsbl_address({}));

    // Entrypoint address field

    header.set_supported_fields(header.supported_fields()
            .set_flag(HeaderField::Entrypoint, false));

    ASSERT_FALSE(header.set_entrypoint_address(1234));
    ASSERT_FALSE(header.entrypoint_address());
    ASSERT_FALSE(header.set_entrypoint_address({}));
}

TEST(BootImgHeaderTest, CheckCopyConstructAndAssign)
{
    Header header;

    header.set_board_name({"test"});
    header.set_kernel_cmdline({"test2"});
    header.set_page_size(2048);
    header.set_kernel_address(0x1000);
    header.set_ramdisk_address(0x2000);
    header.set_secondboot_address(0x3000);
    header.set_kernel_tags_address(0x4000);
    header.set_sony_ipl_address(0x5000);
    header.set_sony_rpm_address(0x6000);
    header.set_sony_appsbl_address(0x7000);
    header.set_entrypoint_address(0x8000);

    {
        Header header2{header};

        ASSERT_EQ(header2.supported_fields(), ALL_FIELDS);

        auto board_name = header2.board_name();
        ASSERT_TRUE(board_name);
        ASSERT_EQ(*board_name, "test");
        auto kernel_cmdline = header2.kernel_cmdline();
        ASSERT_TRUE(kernel_cmdline);
        ASSERT_EQ(*kernel_cmdline, "test2");
        auto page_size = header2.page_size();
        ASSERT_TRUE(page_size);
        ASSERT_EQ(*page_size, 2048u);
        auto kernel_address = header2.kernel_address();
        ASSERT_TRUE(kernel_address);
        ASSERT_EQ(*kernel_address, 0x1000u);
        auto ramdisk_address = header2.ramdisk_address();
        ASSERT_TRUE(ramdisk_address);
        ASSERT_EQ(*ramdisk_address, 0x2000u);
        auto secondboot_address = header2.secondboot_address();
        ASSERT_TRUE(secondboot_address);
        ASSERT_EQ(*secondboot_address, 0x3000u);
        auto kernel_tags_address = header2.kernel_tags_address();
        ASSERT_TRUE(kernel_tags_address);
        ASSERT_EQ(*kernel_tags_address, 0x4000u);
        auto sony_ipl_address = header2.sony_ipl_address();
        ASSERT_TRUE(sony_ipl_address);
        ASSERT_EQ(*sony_ipl_address, 0x5000u);
        auto sony_rpm_address = header2.sony_rpm_address();
        ASSERT_TRUE(sony_rpm_address);
        ASSERT_EQ(*sony_rpm_address, 0x6000u);
        auto sony_appsbl_address = header2.sony_appsbl_address();
        ASSERT_TRUE(sony_appsbl_address);
        ASSERT_EQ(*sony_appsbl_address, 0x7000u);
        auto entrypoint_address = header2.entrypoint_address();
        ASSERT_TRUE(entrypoint_address);
        ASSERT_EQ(*entrypoint_address, 0x8000u);
    }

    {
        Header header2;
        header2 = header;

        ASSERT_EQ(header2.supported_fields(), ALL_FIELDS);

        auto board_name = header2.board_name();
        ASSERT_TRUE(board_name);
        ASSERT_EQ(*board_name, "test");
        auto kernel_cmdline = header2.kernel_cmdline();
        ASSERT_TRUE(kernel_cmdline);
        ASSERT_EQ(*kernel_cmdline, "test2");
        auto page_size = header2.page_size();
        ASSERT_TRUE(page_size);
        ASSERT_EQ(*page_size, 2048u);
        auto kernel_address = header2.kernel_address();
        ASSERT_TRUE(kernel_address);
        ASSERT_EQ(*kernel_address, 0x1000u);
        auto ramdisk_address = header2.ramdisk_address();
        ASSERT_TRUE(ramdisk_address);
        ASSERT_EQ(*ramdisk_address, 0x2000u);
        auto secondboot_address = header2.secondboot_address();
        ASSERT_TRUE(secondboot_address);
        ASSERT_EQ(*secondboot_address, 0x3000u);
        auto kernel_tags_address = header2.kernel_tags_address();
        ASSERT_TRUE(kernel_tags_address);
        ASSERT_EQ(*kernel_tags_address, 0x4000u);
        auto sony_ipl_address = header2.sony_ipl_address();
        ASSERT_TRUE(sony_ipl_address);
        ASSERT_EQ(*sony_ipl_address, 0x5000u);
        auto sony_rpm_address = header2.sony_rpm_address();
        ASSERT_TRUE(sony_rpm_address);
        ASSERT_EQ(*sony_rpm_address, 0x6000u);
        auto sony_appsbl_address = header2.sony_appsbl_address();
        ASSERT_TRUE(sony_appsbl_address);
        ASSERT_EQ(*sony_appsbl_address, 0x7000u);
        auto entrypoint_address = header2.entrypoint_address();
        ASSERT_TRUE(entrypoint_address);
        ASSERT_EQ(*entrypoint_address, 0x8000u);
    }
}

TEST(BootImgHeaderTest, CheckMoveConstructAndAssign)
{
    Header header;

    {
        header.set_board_name({"test"});
        header.set_kernel_cmdline({"test2"});
        header.set_page_size(2048);
        header.set_kernel_address(0x1000);
        header.set_ramdisk_address(0x2000);
        header.set_secondboot_address(0x3000);
        header.set_kernel_tags_address(0x4000);
        header.set_sony_ipl_address(0x5000);
        header.set_sony_rpm_address(0x6000);
        header.set_sony_appsbl_address(0x7000);
        header.set_entrypoint_address(0x8000);

        Header header2{std::move(header)};

        ASSERT_EQ(header2.supported_fields(), ALL_FIELDS);

        auto board_name = header2.board_name();
        ASSERT_TRUE(board_name);
        ASSERT_EQ(*board_name, "test");
        auto kernel_cmdline = header2.kernel_cmdline();
        ASSERT_TRUE(kernel_cmdline);
        ASSERT_EQ(*kernel_cmdline, "test2");
        auto page_size = header2.page_size();
        ASSERT_TRUE(page_size);
        ASSERT_EQ(*page_size, 2048u);
        auto kernel_address = header2.kernel_address();
        ASSERT_TRUE(kernel_address);
        ASSERT_EQ(*kernel_address, 0x1000u);
        auto ramdisk_address = header2.ramdisk_address();
        ASSERT_TRUE(ramdisk_address);
        ASSERT_EQ(*ramdisk_address, 0x2000u);
        auto secondboot_address = header2.secondboot_address();
        ASSERT_TRUE(secondboot_address);
        ASSERT_EQ(*secondboot_address, 0x3000u);
        auto kernel_tags_address = header2.kernel_tags_address();
        ASSERT_TRUE(kernel_tags_address);
        ASSERT_EQ(*kernel_tags_address, 0x4000u);
        auto sony_ipl_address = header2.sony_ipl_address();
        ASSERT_TRUE(sony_ipl_address);
        ASSERT_EQ(*sony_ipl_address, 0x5000u);
        auto sony_rpm_address = header2.sony_rpm_address();
        ASSERT_TRUE(sony_rpm_address);
        ASSERT_EQ(*sony_rpm_address, 0x6000u);
        auto sony_appsbl_address = header2.sony_appsbl_address();
        ASSERT_TRUE(sony_appsbl_address);
        ASSERT_EQ(*sony_appsbl_address, 0x7000u);
        auto entrypoint_address = header2.entrypoint_address();
        ASSERT_TRUE(entrypoint_address);
        ASSERT_EQ(*entrypoint_address, 0x8000u);
    }

    {
        header.set_board_name({"test"});
        header.set_kernel_cmdline({"test2"});
        header.set_page_size(2048);
        header.set_kernel_address(0x1000);
        header.set_ramdisk_address(0x2000);
        header.set_secondboot_address(0x3000);
        header.set_kernel_tags_address(0x4000);
        header.set_sony_ipl_address(0x5000);
        header.set_sony_rpm_address(0x6000);
        header.set_sony_appsbl_address(0x7000);
        header.set_entrypoint_address(0x8000);

        Header header2;
        header2 = std::move(header);

        ASSERT_EQ(header2.supported_fields(), ALL_FIELDS);

        auto board_name = header2.board_name();
        ASSERT_TRUE(board_name);
        ASSERT_EQ(*board_name, "test");
        auto kernel_cmdline = header2.kernel_cmdline();
        ASSERT_TRUE(kernel_cmdline);
        ASSERT_EQ(*kernel_cmdline, "test2");
        auto page_size = header2.page_size();
        ASSERT_TRUE(page_size);
        ASSERT_EQ(*page_size, 2048u);
        auto kernel_address = header2.kernel_address();
        ASSERT_TRUE(kernel_address);
        ASSERT_EQ(*kernel_address, 0x1000u);
        auto ramdisk_address = header2.ramdisk_address();
        ASSERT_TRUE(ramdisk_address);
        ASSERT_EQ(*ramdisk_address, 0x2000u);
        auto secondboot_address = header2.secondboot_address();
        ASSERT_TRUE(secondboot_address);
        ASSERT_EQ(*secondboot_address, 0x3000u);
        auto kernel_tags_address = header2.kernel_tags_address();
        ASSERT_TRUE(kernel_tags_address);
        ASSERT_EQ(*kernel_tags_address, 0x4000u);
        auto sony_ipl_address = header2.sony_ipl_address();
        ASSERT_TRUE(sony_ipl_address);
        ASSERT_EQ(*sony_ipl_address, 0x5000u);
        auto sony_rpm_address = header2.sony_rpm_address();
        ASSERT_TRUE(sony_rpm_address);
        ASSERT_EQ(*sony_rpm_address, 0x6000u);
        auto sony_appsbl_address = header2.sony_appsbl_address();
        ASSERT_TRUE(sony_appsbl_address);
        ASSERT_EQ(*sony_appsbl_address, 0x7000u);
        auto entrypoint_address = header2.entrypoint_address();
        ASSERT_TRUE(entrypoint_address);
        ASSERT_EQ(*entrypoint_address, 0x8000u);
    }
}

TEST(BootImgHeaderTest, CheckEquality)
{
    Header header;

    header.set_board_name({"test"});
    header.set_kernel_cmdline({"test2"});
    header.set_page_size(2048);
    header.set_kernel_address(0x1000);
    header.set_ramdisk_address(0x2000);
    header.set_secondboot_address(0x3000);
    header.set_kernel_tags_address(0x4000);
    header.set_sony_ipl_address(0x5000);
    header.set_sony_rpm_address(0x6000);
    header.set_sony_appsbl_address(0x7000);
    header.set_entrypoint_address(0x8000);

    Header header2;

    header2.set_board_name({"test"});
    header2.set_kernel_cmdline({"test2"});
    header2.set_page_size(2048);
    header2.set_kernel_address(0x1000);
    header2.set_ramdisk_address(0x2000);
    header2.set_secondboot_address(0x3000);
    header2.set_kernel_tags_address(0x4000);
    header2.set_sony_ipl_address(0x5000);
    header2.set_sony_rpm_address(0x6000);
    header2.set_sony_appsbl_address(0x7000);
    header2.set_entrypoint_address(0x8000);

    ASSERT_EQ(header, header2);
    header2.set_board_name({"foobar"});
    ASSERT_NE(header, header2);
}
