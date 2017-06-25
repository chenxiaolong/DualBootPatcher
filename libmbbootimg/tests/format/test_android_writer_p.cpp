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

#include "mbbootimg/format/android_writer_p.h"

typedef std::unique_ptr<MbBiHeader, decltype(mb_bi_header_free) *> ScopedHeader;
typedef std::unique_ptr<MbBiWriter, decltype(mb_bi_writer_free) *> ScopedWriter;

#if 0
TEST(AndroidWriterInternalsTest, CheckHeaderCleared)
{
    ScopedWriter biw(mb_bi_writer_new(), mb_bi_writer_free);
    ASSERT_TRUE(!!biw);
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    AndroidWriterCtx ctx = {};
    ctx.client_header = header.get();

    MbBiHeader *header2;

    ASSERT_EQ(mb_bi_header_set_page_size(ctx.client_header, 2048), MB_BI_OK);
    ASSERT_EQ(android_writer_get_header(biw.get(), &ctx, &header2), MB_BI_OK);
    ASSERT_FALSE(mb_bi_header_page_size_is_set(header2));
}

TEST(AndroidWriterInternalsTest, CheckHeaderSupportedFields)
{
    ScopedWriter biw(mb_bi_writer_new(), mb_bi_writer_free);
    ASSERT_TRUE(!!biw);
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    AndroidWriterCtx ctx = {};
    ctx.client_header = header.get();

    MbBiHeader *header2;

    // We don't care about the actual value, just that it was changed
    ASSERT_EQ(android_writer_get_header(biw.get(), &ctx, &header2), MB_BI_OK);
    ASSERT_NE(mb_bi_header_supported_fields(header2), MB_BI_HEADER_ALL_FIELDS);
}

TEST(AndroidWriterInternalsTest, CheckHeaderFieldsSet)
{
    ScopedWriter biw(mb_bi_writer_new(), mb_bi_writer_free);
    ASSERT_TRUE(!!biw);
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    AndroidWriterCtx ctx = {};
    ctx.client_header = header.get();

    MbBiHeader *header2;

    // Get header instance
    ASSERT_EQ(android_writer_get_header(biw.get(), &ctx, &header2), MB_BI_OK);

    // Set some dummy values
    ASSERT_EQ(mb_bi_header_set_kernel_address(header2, 0x11223344), MB_BI_OK);
    ASSERT_EQ(mb_bi_header_set_ramdisk_address(header2, 0x22334455), MB_BI_OK);
    ASSERT_EQ(mb_bi_header_set_secondboot_address(header2, 0x33445566),
              MB_BI_OK);
    ASSERT_EQ(mb_bi_header_set_kernel_tags_address(header2, 0x44556677),
              MB_BI_OK);
    ASSERT_EQ(mb_bi_header_set_page_size(header2, 2048), MB_BI_OK);
    ASSERT_EQ(mb_bi_header_set_board_name(header2, "hello"), MB_BI_OK);
    ASSERT_EQ(mb_bi_header_set_kernel_cmdline(header2, "world"), MB_BI_OK);

    // Write header
    ASSERT_EQ(android_writer_write_header(biw.get(), &ctx, header2), MB_BI_OK);

    // Check that the native header matches
    ASSERT_EQ(ctx.hdr.kernel_addr, 0x11223344);
    ASSERT_EQ(ctx.hdr.ramdisk_addr, 0x22334455);
    ASSERT_EQ(ctx.hdr.second_addr, 0x33445566);
    ASSERT_EQ(ctx.hdr.tags_addr, 0x44556677);
    ASSERT_EQ(ctx.hdr.page_size, 2048);
    ASSERT_STREQ(reinterpret_cast<char *>(ctx.hdr.name), "hello");
    ASSERT_STREQ(reinterpret_cast<char *>(ctx.hdr.cmdline), "world");
}

TEST(AndroidWriterInternalsTest, MissingPageSizeShouldFail)
{
    ScopedWriter biw(mb_bi_writer_new(), mb_bi_writer_free);
    ASSERT_TRUE(!!biw);
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    AndroidWriterCtx ctx = {};
    ctx.client_header = header.get();

    MbBiHeader *header2;

    // Get header instance
    ASSERT_EQ(android_writer_get_header(biw.get(), &ctx, &header2), MB_BI_OK);

    // Write header
    ASSERT_EQ(android_writer_write_header(biw.get(), &ctx, header2),
              MB_BI_FAILED);
    ASSERT_TRUE(strstr(mb_bi_writer_error_string(biw.get()), "Page size"));
}

TEST(AndroidWriterInternalsTest, InvalidPageSizeShouldFail)
{
    ScopedWriter biw(mb_bi_writer_new(), mb_bi_writer_free);
    ASSERT_TRUE(!!biw);
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    AndroidWriterCtx ctx = {};
    ctx.client_header = header.get();

    MbBiHeader *header2;

    // Get header instance
    ASSERT_EQ(android_writer_get_header(biw.get(), &ctx, &header2), MB_BI_OK);

    // Set page size
    ASSERT_EQ(mb_bi_header_set_page_size(header2, 1234), MB_BI_OK);

    // Write header
    ASSERT_EQ(android_writer_write_header(biw.get(), &ctx, header2),
              MB_BI_FAILED);
    ASSERT_TRUE(strstr(mb_bi_writer_error_string(biw.get()),
                       "Invalid page size"));
}

TEST(AndroidWriterInternalsTest, OversizedBoardNameShouldFail)
{
    ScopedWriter biw(mb_bi_writer_new(), mb_bi_writer_free);
    ASSERT_TRUE(!!biw);
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    AndroidWriterCtx ctx = {};
    ctx.client_header = header.get();

    MbBiHeader *header2;

    // Get header instance
    ASSERT_EQ(android_writer_get_header(biw.get(), &ctx, &header2), MB_BI_OK);
    ASSERT_EQ(mb_bi_header_set_page_size(header2, 2048), MB_BI_OK);

    // Set board name
    std::string name(ANDROID_BOOT_NAME_SIZE, 'c');
    ASSERT_EQ(mb_bi_header_set_board_name(header2, name.c_str()), MB_BI_OK);

    // Write header
    ASSERT_EQ(android_writer_write_header(biw.get(), &ctx, header2),
              MB_BI_FAILED);
    ASSERT_TRUE(strstr(mb_bi_writer_error_string(biw.get()),
                       "Board name"));
}

TEST(AndroidWriterInternalsTest, OversizedCmdlineShouldFail)
{
    ScopedWriter biw(mb_bi_writer_new(), mb_bi_writer_free);
    ASSERT_TRUE(!!biw);
    ScopedHeader header(mb_bi_header_new(), mb_bi_header_free);
    ASSERT_TRUE(!!header);

    AndroidWriterCtx ctx = {};
    ctx.client_header = header.get();

    MbBiHeader *header2;

    // Get header instance
    ASSERT_EQ(android_writer_get_header(biw.get(), &ctx, &header2), MB_BI_OK);
    ASSERT_EQ(mb_bi_header_set_page_size(header2, 2048), MB_BI_OK);

    // Set board name
    std::string args(ANDROID_BOOT_ARGS_SIZE, 'c');
    ASSERT_EQ(mb_bi_header_set_kernel_cmdline(header2, args.c_str()), MB_BI_OK);

    // Write header
    ASSERT_EQ(android_writer_write_header(biw.get(), &ctx, header2),
              MB_BI_FAILED);
    ASSERT_TRUE(strstr(mb_bi_writer_error_string(biw.get()),
                       "Kernel cmdline"));
}
#endif
