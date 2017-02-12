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

#include "mbcommon/file/memory.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer.h"

typedef std::unique_ptr<MbFile, decltype(mb_file_free) *> ScopedFile;
typedef std::unique_ptr<MbBiWriter, decltype(mb_bi_writer_free) *> ScopedWriter;

struct AndroidWriterSHA1Test : public ::testing::Test
{
protected:
    ScopedFile _file;
    ScopedWriter _biw;
    void *_buf;
    size_t _buf_size;

    AndroidWriterSHA1Test()
        : _file(mb_file_new(), mb_file_free)
        , _biw(mb_bi_writer_new(), mb_bi_writer_free)
        , _buf(nullptr)
        , _buf_size(0)
    {
    }

    virtual ~AndroidWriterSHA1Test()
    {
        free(_buf);
    }

    virtual void SetUp()
    {
        ASSERT_TRUE(!!_file);
        ASSERT_TRUE(!!_biw);

        ASSERT_EQ(mb_file_open_memory_dynamic(_file.get(), &_buf, &_buf_size),
                  MB_FILE_OK);

        ASSERT_EQ(mb_bi_writer_set_format_android(_biw.get()), MB_BI_OK);
        ASSERT_EQ(mb_bi_writer_open(_biw.get(), _file.get(), false), MB_BI_OK);
    }

    virtual void TearDown()
    {
    }

    void TestChecksum(const unsigned char expected[20], int types)
    {
        static const unsigned char padding[] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };

        MbBiHeader *header;
        MbBiEntry *entry;
        int ret;
        size_t n;

        // Write dummy header
        ASSERT_EQ(mb_bi_writer_get_header(_biw.get(), &header), MB_BI_OK);
        ASSERT_EQ(mb_bi_header_set_page_size(header, 2048), MB_BI_OK);
        ASSERT_EQ(mb_bi_writer_write_header(_biw.get(), header), MB_BI_OK);

        // Write specified dummy entries
        while ((ret = mb_bi_writer_get_entry(_biw.get(), &entry)) == MB_BI_OK) {
            ASSERT_EQ(mb_bi_writer_write_entry(_biw.get(), entry), MB_BI_OK);

            if (mb_bi_entry_type(entry) & types) {
                ASSERT_EQ(mb_bi_writer_write_data(_biw.get(), "hello", 5, &n),
                          MB_BI_OK);
                ASSERT_EQ(n, 5);
            }
        }
        ASSERT_EQ(ret, MB_BI_EOF);

        // Close to write header
        ASSERT_EQ(mb_bi_writer_close(_biw.get()), MB_BI_OK);

        // Check SHA1
        ASSERT_EQ(memcmp(static_cast<unsigned char *>(_buf) + 576,
                         expected, 20), 0);
        // Check padding
        ASSERT_EQ(memcmp(static_cast<unsigned char *>(_buf) + 596,
                         padding, sizeof(padding)), 0);
    }
};

TEST_F(AndroidWriterSHA1Test, HandlesNothing)
{
    static const unsigned char expected[] = {
        0x2c, 0x51, 0x3f, 0x14, 0x9e, 0x73, 0x7e, 0xc4, 0x06, 0x3f,
        0xc1, 0xd3, 0x7a, 0xee, 0x9b, 0xea, 0xbc, 0x4b, 0x4b, 0xbf,
    };

    TestChecksum(expected, 0);
}

TEST_F(AndroidWriterSHA1Test, HandlesKernel)
{
    static const unsigned char expected[] = {
        0xb2, 0x59, 0xf5, 0xff, 0xfb, 0x93, 0x64, 0xab, 0x8c, 0x5f,
        0xf7, 0x89, 0x8c, 0x5f, 0xea, 0x7f, 0x47, 0xbc, 0x68, 0x69,
    };

    TestChecksum(expected, MB_BI_ENTRY_KERNEL);
}

TEST_F(AndroidWriterSHA1Test, HandlesKernelRamdisk)
{
    static const unsigned char expected[] = {
        0xed, 0xe0, 0xeb, 0x80, 0x25, 0x76, 0xb8, 0x9d, 0x2e, 0x8c,
        0x1e, 0xfb, 0xec, 0x44, 0x65, 0xf9, 0xc1, 0x62, 0x11, 0xfd,
    };

    TestChecksum(expected, MB_BI_ENTRY_KERNEL | MB_BI_ENTRY_RAMDISK);
}

TEST_F(AndroidWriterSHA1Test, HandlesKernelRamdiskSecondboot)
{
    static const unsigned char expected[] = {
        0xdb, 0xb8, 0x4a, 0x49, 0x68, 0xc2, 0xcb, 0xef, 0xdc, 0x7e,
        0x42, 0xe1, 0xaf, 0xe5, 0x4d, 0xa7, 0xc3, 0x16, 0x8f, 0x5e,
    };

    TestChecksum(expected, MB_BI_ENTRY_KERNEL | MB_BI_ENTRY_RAMDISK
            | MB_BI_ENTRY_SECONDBOOT);
}

TEST_F(AndroidWriterSHA1Test, HandlesKernelRamdiskSecondbootDT)
{
    static const unsigned char expected[] = {
        0xba, 0xf5, 0xf6, 0x39, 0xde, 0xb3, 0x53, 0xeb, 0x29, 0xc2,
        0x09, 0x35, 0x85, 0x26, 0x06, 0x36, 0x17, 0xbb, 0x05, 0x20,
    };

    TestChecksum(expected, MB_BI_ENTRY_KERNEL | MB_BI_ENTRY_RAMDISK
            | MB_BI_ENTRY_SECONDBOOT | MB_BI_ENTRY_DEVICE_TREE);
}
