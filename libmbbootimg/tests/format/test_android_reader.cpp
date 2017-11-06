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

#include "mbcommon/file.h"
#include "mbcommon/file/memory.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/android_reader_p.h"
#include "mbbootimg/reader.h"

using namespace mb::bootimg;
using namespace mb::bootimg::android;

// Tests for find_header()

TEST(FindAndroidHeaderTest, ValidMagicShouldSucceed)
{
    Reader reader;

    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

    AndroidHeader header;
    uint64_t offset;

    mb::MemoryFile file(&source, sizeof(source));
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(AndroidFormatReader::find_header(reader, file, MAX_HEADER_OFFSET,
                                               header, offset), RET_OK);
}

TEST(FindAndroidHeaderTest, BadInitialFileOffsetShouldSucceed)
{
    Reader reader;

    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

    AndroidHeader header;
    uint64_t offset;

    mb::MemoryFile file(&source, sizeof(source));
    ASSERT_TRUE(file.is_open());

    // Seek to bad location initially
    ASSERT_TRUE(file.seek(10, SEEK_SET));

    ASSERT_EQ(AndroidFormatReader::find_header(reader, file, MAX_HEADER_OFFSET,
                                               header, offset), RET_OK);
}

TEST(FindAndroidHeaderTest, OutOfBoundsMaximumOffsetShouldWarn)
{
    Reader reader;

    AndroidHeader header;
    uint64_t offset;

    mb::MemoryFile file(static_cast<const void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(AndroidFormatReader::find_header(reader, file,
                                               MAX_HEADER_OFFSET + 1, header,
                                               offset), RET_WARN);
    ASSERT_NE(reader.error_string().find("Max header offset"),
              std::string::npos);
}

TEST(FindAndroidHeaderTest, MissingMagicShouldWarn)
{
    Reader reader;

    AndroidHeader header;
    uint64_t offset;

    mb::MemoryFile file(static_cast<const void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(AndroidFormatReader::find_header(reader, file, MAX_HEADER_OFFSET,
                                               header, offset), RET_WARN);
    ASSERT_NE(reader.error_string().find("Android magic not found"),
              std::string::npos);
}

TEST(FindAndroidHeaderTest, TruncatedHeaderShouldWarn)
{
    Reader reader;

    AndroidHeader header;
    uint64_t offset;

    mb::MemoryFile file(BOOT_MAGIC, BOOT_MAGIC_SIZE);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(AndroidFormatReader::find_header(reader, file, MAX_HEADER_OFFSET,
                                               header, offset), RET_WARN);
    ASSERT_NE(reader.error_string().find("exceeds file size"),
              std::string::npos);
}

// Tests for find_samsung_seandroid_magic()

TEST(FindSEAndroidMagicTest, ValidMagicShouldSucceed)
{
    Reader reader;

    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    source.kernel_size = 0;
    source.ramdisk_size = 0;
    source.second_size = 0;
    source.dt_size = 0;
    source.page_size = 2048;

    uint64_t offset;

    std::vector<unsigned char> data;
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&source),
                reinterpret_cast<unsigned char *>(&source) + sizeof(source));
    data.resize(source.page_size);
    data.insert(data.end(), SAMSUNG_SEANDROID_MAGIC,
                SAMSUNG_SEANDROID_MAGIC + SAMSUNG_SEANDROID_MAGIC_SIZE);

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(AndroidFormatReader::find_samsung_seandroid_magic(reader, file,
                                                                source, offset),
              RET_OK);
}

TEST(FindSEAndroidMagicTest, UndersizedImageShouldWarn)
{
    Reader reader;

    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    source.kernel_size = 0;
    source.ramdisk_size = 0;
    source.second_size = 0;
    source.dt_size = 0;
    source.page_size = 2048;

    uint64_t offset;

    mb::MemoryFile file(static_cast<const void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(AndroidFormatReader::find_samsung_seandroid_magic(reader, file,
                                                                source, offset),
              RET_WARN);
    ASSERT_NE(reader.error_string().find("SEAndroid magic not found"),
              std::string::npos);
}

TEST(FindSEAndroidMagicTest, InvalidMagicShouldWarn)
{
    Reader reader;

    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    source.kernel_size = 0;
    source.ramdisk_size = 0;
    source.second_size = 0;
    source.dt_size = 0;
    source.page_size = 2048;

    uint64_t offset;

    std::vector<unsigned char> data;
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&source),
                reinterpret_cast<unsigned char *>(&source) + sizeof(source));
    data.resize(source.page_size);
    data.insert(data.end(), SAMSUNG_SEANDROID_MAGIC,
                SAMSUNG_SEANDROID_MAGIC + SAMSUNG_SEANDROID_MAGIC_SIZE);
    data.back() = 'x';

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(AndroidFormatReader::find_samsung_seandroid_magic(reader, file,
                                                                source, offset),
              RET_WARN);
    ASSERT_NE(reader.error_string().find("SEAndroid magic not found"),
              std::string::npos);
}

// Tests for convert_header()

TEST(AndroidSetHeaderTest, ValuesShouldMatch)
{
    Header header;

    AndroidHeader ahdr = {};
    memcpy(ahdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    ahdr.kernel_size = 1234;
    ahdr.kernel_addr = 0x11223344;
    ahdr.ramdisk_size = 2345;
    ahdr.ramdisk_addr = 0x22334455;
    ahdr.second_size = 3456;
    ahdr.second_addr = 0x33445566;
    ahdr.tags_addr = 0x44556677;
    ahdr.page_size = 2048;
    ahdr.dt_size = 4567;
    ahdr.unused = 5678;
    snprintf(reinterpret_cast<char *>(ahdr.name), sizeof(ahdr.name),
             "Test board name");
    snprintf(reinterpret_cast<char *>(ahdr.cmdline), sizeof(ahdr.cmdline),
             "Test cmdline");
    memset(ahdr.id, 0xff, sizeof(ahdr.id));

    ASSERT_EQ(AndroidFormatReader::convert_header(ahdr, header), RET_OK);

    ASSERT_EQ(header.supported_fields(), SUPPORTED_FIELDS);

    auto board_name = header.board_name();
    ASSERT_TRUE(board_name);
    ASSERT_EQ(*board_name, reinterpret_cast<char *>(ahdr.name));

    auto cmdline = header.kernel_cmdline();
    ASSERT_TRUE(cmdline);
    ASSERT_EQ(*cmdline, reinterpret_cast<char *>(ahdr.cmdline));

    auto page_size = header.page_size();
    ASSERT_TRUE(page_size);
    ASSERT_EQ(*page_size, ahdr.page_size);

    auto kernel_address = header.kernel_address();
    ASSERT_TRUE(kernel_address);
    ASSERT_EQ(*kernel_address, ahdr.kernel_addr);

    auto ramdisk_address = header.ramdisk_address();
    ASSERT_TRUE(ramdisk_address);
    ASSERT_EQ(*ramdisk_address, ahdr.ramdisk_addr);

    auto secondboot_address = header.secondboot_address();
    ASSERT_TRUE(secondboot_address);
    ASSERT_EQ(*secondboot_address, ahdr.second_addr);

    auto kernel_tags_address = header.kernel_tags_address();
    ASSERT_TRUE(kernel_tags_address);
    ASSERT_EQ(*kernel_tags_address, ahdr.tags_addr);
}

struct AndroidReaderGoToEntryTest : testing::Test
{
    mb::MemoryFile _file;
    Reader _reader;
    std::vector<unsigned char> _data;
    Header _header;

    AndroidReaderGoToEntryTest()
    {
    }

    virtual ~AndroidReaderGoToEntryTest()
    {
    }

    virtual void SetUp() override
    {
        AndroidHeader ahdr = {};
        memcpy(ahdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
        ahdr.kernel_size = 6;
        ahdr.ramdisk_size = 7;
        ahdr.second_size = 10;
        ahdr.page_size = 2048;

        _data.resize(4 * ahdr.page_size);

        // Write headers
        memcpy(_data.data(), &ahdr, sizeof(ahdr));
        // Write kernel
        memcpy(_data.data() + ahdr.page_size, "kernel", 6);
        // Write ramdisk
        memcpy(_data.data() + 2 * ahdr.page_size, "ramdisk", 7);
        // Write secondboot
        memcpy(_data.data() + 3 * ahdr.page_size, "secondboot", 10);

        ASSERT_EQ(_reader.enable_format_android(), RET_OK);

        (void) _file.open(_data.data(), _data.size());
        ASSERT_TRUE(_file.is_open());

        ASSERT_EQ(_reader.open(&_file), RET_OK);

        ASSERT_EQ(_reader.read_header(_header), RET_OK);
    }
};

TEST_F(AndroidReaderGoToEntryTest, GoToShouldSucceed)
{
    Entry entry;
    char buf[50];
    size_t n;

    ASSERT_EQ(_reader.go_to_entry(entry, ENTRY_TYPE_RAMDISK), RET_OK);
    ASSERT_EQ(*entry.type(), ENTRY_TYPE_RAMDISK);
    ASSERT_EQ(_reader.read_data(buf, sizeof(buf), n), RET_OK);
    ASSERT_EQ(n, 7u);
    ASSERT_EQ(memcmp(buf, "ramdisk", n), 0);

    // We should continue at the next entry
    ASSERT_EQ(_reader.read_entry(entry), RET_OK);
    ASSERT_EQ(*entry.type(), ENTRY_TYPE_SECONDBOOT);
    ASSERT_EQ(_reader.read_data(buf, sizeof(buf), n), RET_OK);
    ASSERT_EQ(n, 10u);
    ASSERT_EQ(memcmp(buf, "secondboot", n), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToFirstEntryShouldSucceed)
{
    Entry entry;
    char buf[50];
    size_t n;

    ASSERT_EQ(_reader.go_to_entry(entry, 0), RET_OK);
    ASSERT_EQ(*entry.type(), ENTRY_TYPE_KERNEL);
    ASSERT_EQ(_reader.read_data(buf, sizeof(buf), n), RET_OK);
    ASSERT_EQ(n, 6u);
    ASSERT_EQ(memcmp(buf, "kernel", n), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToPreviousEntryShouldSucceed)
{
    Entry entry;
    char buf[50];
    size_t n;

    ASSERT_EQ(_reader.go_to_entry(entry, ENTRY_TYPE_SECONDBOOT), RET_OK);
    ASSERT_EQ(*entry.type(), ENTRY_TYPE_SECONDBOOT);
    ASSERT_EQ(_reader.read_data(buf, sizeof(buf), n), RET_OK);
    ASSERT_EQ(n, 10u);
    ASSERT_EQ(memcmp(buf, "secondboot", n), 0);

    // Go back to kernel
    ASSERT_EQ(_reader.go_to_entry(entry, ENTRY_TYPE_KERNEL), RET_OK);
    ASSERT_EQ(*entry.type(), ENTRY_TYPE_KERNEL);
    ASSERT_EQ(_reader.read_data(buf, sizeof(buf), n), RET_OK);
    ASSERT_EQ(n, 6u);
    ASSERT_EQ(memcmp(buf, "kernel", n), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToAfterEOFShouldSucceed)
{
    Entry entry;
    char buf[50];
    size_t n;
    int ret;

    while ((ret = _reader.read_entry(entry)) == RET_OK);
    ASSERT_EQ(ret, RET_EOF);

    ASSERT_EQ(_reader.go_to_entry(entry, ENTRY_TYPE_KERNEL), RET_OK);
    ASSERT_EQ(*entry.type(), ENTRY_TYPE_KERNEL);
    ASSERT_EQ(_reader.read_data(buf, sizeof(buf), n), RET_OK);
    ASSERT_EQ(n, 6u);
    ASSERT_EQ(memcmp(buf, "kernel", n), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToMissingEntryShouldFail)
{
    Entry entry;

    ASSERT_EQ(_reader.go_to_entry(entry, ENTRY_TYPE_DEVICE_TREE), RET_EOF);

    // In EOF state now, so next read should return RET_EOF
    ASSERT_EQ(_reader.read_entry(entry), RET_EOF);
}
