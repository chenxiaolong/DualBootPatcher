/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file/memory.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer.h"

using namespace mb;
using namespace mb::bootimg;

struct AndroidWriterSHA1Test : public ::testing::Test
{
protected:
    void *_buf;
    size_t _buf_size;
    MemoryFile _file;
    Writer _writer;

    AndroidWriterSHA1Test()
        : _buf(nullptr)
        , _buf_size(0)
        , _file(&_buf, &_buf_size)
        , _writer()
    {
    }

    virtual ~AndroidWriterSHA1Test()
    {
        free(_buf);
    }

    virtual void SetUp()
    {
        ASSERT_TRUE(_file.is_open());

        ASSERT_TRUE(_writer.set_format(Format::Android));
        ASSERT_TRUE(_writer.open(&_file));
    }

    virtual void TearDown()
    {
    }

    void TestChecksum(const unsigned char expected[20], EntryTypes types)
    {
        static const unsigned char padding[] = {
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        };

        // Write dummy header
        auto header = _writer.get_header();
        ASSERT_TRUE(header);
        ASSERT_TRUE(header.value().set_page_size(2048));
        ASSERT_TRUE(_writer.write_header(header.value()));

        // Write specified dummy entries
        while (true) {
            auto entry = _writer.get_entry();
            if (!entry) {
                ASSERT_EQ(entry.error(), WriterError::EndOfEntries);
                break;
            }

            ASSERT_TRUE(_writer.write_entry(entry.value()));

            if (entry.value().type() & types) {
                auto n = _writer.write_data("hello", 5);
                ASSERT_TRUE(n);
                ASSERT_EQ(n.value(), 5u);
            }
        }

        // Close to write header
        ASSERT_TRUE(_writer.close());

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

    TestChecksum(expected, {});
}

TEST_F(AndroidWriterSHA1Test, HandlesKernel)
{
    static const unsigned char expected[] = {
        0xb2, 0x59, 0xf5, 0xff, 0xfb, 0x93, 0x64, 0xab, 0x8c, 0x5f,
        0xf7, 0x89, 0x8c, 0x5f, 0xea, 0x7f, 0x47, 0xbc, 0x68, 0x69,
    };

    TestChecksum(expected, EntryType::Kernel);
}

TEST_F(AndroidWriterSHA1Test, HandlesKernelRamdisk)
{
    static const unsigned char expected[] = {
        0xed, 0xe0, 0xeb, 0x80, 0x25, 0x76, 0xb8, 0x9d, 0x2e, 0x8c,
        0x1e, 0xfb, 0xec, 0x44, 0x65, 0xf9, 0xc1, 0x62, 0x11, 0xfd,
    };

    TestChecksum(expected, EntryType::Kernel | EntryType::Ramdisk);
}

TEST_F(AndroidWriterSHA1Test, HandlesKernelRamdiskSecondboot)
{
    static const unsigned char expected[] = {
        0xdb, 0xb8, 0x4a, 0x49, 0x68, 0xc2, 0xcb, 0xef, 0xdc, 0x7e,
        0x42, 0xe1, 0xaf, 0xe5, 0x4d, 0xa7, 0xc3, 0x16, 0x8f, 0x5e,
    };

    TestChecksum(expected, EntryType::Kernel | EntryType::Ramdisk
            | EntryType::SecondBoot);
}

TEST_F(AndroidWriterSHA1Test, HandlesKernelRamdiskSecondbootDT)
{
    static const unsigned char expected[] = {
        0xba, 0xf5, 0xf6, 0x39, 0xde, 0xb3, 0x53, 0xeb, 0x29, 0xc2,
        0x09, 0x35, 0x85, 0x26, 0x06, 0x36, 0x17, 0xbb, 0x05, 0x20,
    };

    TestChecksum(expected, EntryType::Kernel | EntryType::Ramdisk
            | EntryType::SecondBoot | EntryType::DeviceTree);
}
