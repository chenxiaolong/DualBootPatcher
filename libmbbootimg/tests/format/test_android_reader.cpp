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

#include "mbcommon/file.h"
#include "mbcommon/file/memory.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/android_error.h"
#include "mbbootimg/format/android_reader_p.h"
#include "mbbootimg/reader.h"

using namespace mb;
using namespace mb::bootimg;
using namespace mb::bootimg::android;

// Tests for find_header()

TEST(FindAndroidHeaderTest, ValidMagicShouldSucceed)
{
    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

    MemoryFile file(&source, sizeof(source));
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(AndroidFormatReader::find_header(file, MAX_HEADER_OFFSET));
}

TEST(FindAndroidHeaderTest, BadInitialFileOffsetShouldSucceed)
{
    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);

    MemoryFile file(&source, sizeof(source));
    ASSERT_TRUE(file.is_open());

    // Seek to bad location initially
    ASSERT_TRUE(file.seek(10, SEEK_SET));

    ASSERT_TRUE(AndroidFormatReader::find_header(file, MAX_HEADER_OFFSET));
}

TEST(FindAndroidHeaderTest, OutOfBoundsMaximumOffsetShouldFail)
{
    MemoryFile file(static_cast<void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    auto ret = AndroidFormatReader::find_header(file, MAX_HEADER_OFFSET + 1);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), AndroidError::InvalidArgument);
}

TEST(FindAndroidHeaderTest, MissingMagicShouldFail)
{
    MemoryFile file(static_cast<void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    auto ret = AndroidFormatReader::find_header(file, MAX_HEADER_OFFSET);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), AndroidError::HeaderNotFound);
}

TEST(FindAndroidHeaderTest, TruncatedHeaderShouldFail)
{
    MemoryFile file(const_cast<unsigned char *>(BOOT_MAGIC), BOOT_MAGIC_SIZE);
    ASSERT_TRUE(file.is_open());

    auto ret = AndroidFormatReader::find_header(file, MAX_HEADER_OFFSET);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), AndroidError::HeaderOutOfBounds);
}

// Tests for find_samsung_seandroid_magic()

TEST(FindSEAndroidMagicTest, ValidMagicShouldSucceed)
{
    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    source.kernel_size = 0;
    source.ramdisk_size = 0;
    source.second_size = 0;
    source.dt_size = 0;
    source.page_size = 2048;

    std::vector<unsigned char> data;
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&source),
                reinterpret_cast<unsigned char *>(&source) + sizeof(source));
    data.resize(source.page_size);
    data.insert(data.end(), SAMSUNG_SEANDROID_MAGIC,
                SAMSUNG_SEANDROID_MAGIC + SAMSUNG_SEANDROID_MAGIC_SIZE);

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(AndroidFormatReader::find_samsung_seandroid_magic(
            file, source));
}

TEST(FindSEAndroidMagicTest, UndersizedImageShouldFail)
{
    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    source.kernel_size = 0;
    source.ramdisk_size = 0;
    source.second_size = 0;
    source.dt_size = 0;
    source.page_size = 2048;

    MemoryFile file(static_cast<void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    auto ret = AndroidFormatReader::find_samsung_seandroid_magic(
            file, source);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), AndroidError::SamsungMagicNotFound);
}

TEST(FindSEAndroidMagicTest, InvalidMagicShouldFail)
{
    AndroidHeader source = {};
    memcpy(source.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    source.kernel_size = 0;
    source.ramdisk_size = 0;
    source.second_size = 0;
    source.dt_size = 0;
    source.page_size = 2048;

    std::vector<unsigned char> data;
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&source),
                reinterpret_cast<unsigned char *>(&source) + sizeof(source));
    data.resize(source.page_size);
    data.insert(data.end(), SAMSUNG_SEANDROID_MAGIC,
                SAMSUNG_SEANDROID_MAGIC + SAMSUNG_SEANDROID_MAGIC_SIZE);
    data.back() = 'x';

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ret = AndroidFormatReader::find_samsung_seandroid_magic(
            file, source);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), AndroidError::SamsungMagicNotFound);
}

// Tests for convert_header()

TEST(AndroidSetHeaderTest, ValuesShouldMatch)
{
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
    std::fill(std::begin(ahdr.id), std::end(ahdr.id), 0xff);

    auto header = AndroidFormatReader::convert_header(ahdr);

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
    MemoryFile _file;
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

        ASSERT_TRUE(_reader.enable_formats(Format::Android));

        (void) _file.open(_data.data(), _data.size());
        ASSERT_TRUE(_file.is_open());

        ASSERT_TRUE(_reader.open(&_file));

        auto header = _reader.read_header();
        ASSERT_TRUE(header);
        _header = std::move(header.value());
    }
};

TEST_F(AndroidReaderGoToEntryTest, GoToShouldSucceed)
{
    char buf[50];

    auto entry = _reader.go_to_entry(EntryType::Ramdisk);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.value().type(), EntryType::Ramdisk);
    auto n = _reader.read_data(buf, sizeof(buf));
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 7u);
    ASSERT_EQ(memcmp(buf, "ramdisk", n.value()), 0);

    // We should continue at the next entry
    entry = _reader.read_entry();
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.value().type(), EntryType::SecondBoot);
    n = _reader.read_data(buf, sizeof(buf));
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 10u);
    ASSERT_EQ(memcmp(buf, "secondboot", n.value()), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToFirstEntryShouldSucceed)
{
    char buf[50];

    auto entry = _reader.go_to_entry({});
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.value().type(), EntryType::Kernel);
    auto n = _reader.read_data(buf, sizeof(buf));
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 6u);
    ASSERT_EQ(memcmp(buf, "kernel", n.value()), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToPreviousEntryShouldSucceed)
{
    char buf[50];

    auto entry = _reader.go_to_entry(EntryType::SecondBoot);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.value().type(), EntryType::SecondBoot);
    auto n = _reader.read_data(buf, sizeof(buf));
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 10u);
    ASSERT_EQ(memcmp(buf, "secondboot", n.value()), 0);

    // Go back to kernel
    entry = _reader.go_to_entry(EntryType::Kernel);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.value().type(), EntryType::Kernel);
    n = _reader.read_data(buf, sizeof(buf));
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 6u);
    ASSERT_EQ(memcmp(buf, "kernel", n.value()), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToAfterEOFShouldSucceed)
{
    char buf[50];
    oc::result<Entry> entry = std::error_code{};

    while ((entry = _reader.read_entry()));
    ASSERT_EQ(entry.error(), ReaderError::EndOfEntries);

    entry = _reader.go_to_entry(EntryType::Kernel);
    ASSERT_TRUE(entry);
    ASSERT_EQ(entry.value().type(), EntryType::Kernel);
    auto n = _reader.read_data(buf, sizeof(buf));
    ASSERT_TRUE(n);
    ASSERT_EQ(n.value(), 6u);
    ASSERT_EQ(memcmp(buf, "kernel", n.value()), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToMissingEntryShouldFail)
{
    auto entry = _reader.go_to_entry(EntryType::DeviceTree);
    ASSERT_FALSE(entry);
    ASSERT_EQ(entry.error(), ReaderError::EndOfEntries);

    // In EOF state now, so next read should fail
    entry = _reader.read_entry();
    ASSERT_FALSE(entry);
    ASSERT_EQ(entry.error(), ReaderError::EndOfEntries);
}
