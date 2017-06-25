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

#include "mbcommon/file.h"
#include "mbcommon/file/memory.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/android_reader_p.h"
#include "mbbootimg/reader.h"

typedef std::unique_ptr<MbFile, decltype(mb_file_free) *> ScopedFile;
typedef std::unique_ptr<MbBiHeader, decltype(mb_bi_header_free) *> ScopedHeader;
typedef std::unique_ptr<MbBiReader, decltype(mb_bi_reader_free) *> ScopedReader;

// Tests for find_android_header()

TEST(FindAndroidHeaderTest, ValidMagicShouldSucceed)
{
    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    AndroidHeader source = {};
    memcpy(source.magic, ANDROID_BOOT_MAGIC, ANDROID_BOOT_MAGIC_SIZE);

    AndroidHeader header;
    uint64_t offset;

    ASSERT_EQ(mb_file_open_memory_static(file.get(), &source, sizeof(source)),
              MB_FILE_OK);

    ASSERT_EQ(find_android_header(bir.get(), file.get(),
                                  ANDROID_MAX_HEADER_OFFSET,
                                  &header, &offset), MB_BI_OK);
}

TEST(FindAndroidHeaderTest, BadInitialFileOffsetShouldSucceed)
{
    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    AndroidHeader source = {};
    memcpy(source.magic, ANDROID_BOOT_MAGIC, ANDROID_BOOT_MAGIC_SIZE);

    AndroidHeader header;
    uint64_t offset;

    ASSERT_EQ(mb_file_open_memory_static(file.get(), &source, sizeof(source)),
              MB_FILE_OK);

    // Seek to bad location initially
    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_SET, nullptr), MB_FILE_OK);

    ASSERT_EQ(find_android_header(bir.get(), file.get(),
                                  ANDROID_MAX_HEADER_OFFSET,
                                  &header, &offset), MB_BI_OK);
}

TEST(FindAndroidHeaderTest, OutOfBoundsMaximumOffsetShouldWarn)
{
    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    AndroidHeader header;
    uint64_t offset;

    ASSERT_EQ(mb_file_open_memory_static(file.get(), nullptr, 0), MB_FILE_OK);

    ASSERT_EQ(find_android_header(bir.get(), file.get(),
                                  ANDROID_MAX_HEADER_OFFSET + 1,
                                  &header, &offset), MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "Max header offset"));
}

TEST(FindAndroidHeaderTest, MissingMagicShouldWarn)
{
    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    AndroidHeader header;
    uint64_t offset;

    ASSERT_EQ(mb_file_open_memory_static(file.get(), nullptr, 0), MB_FILE_OK);

    ASSERT_EQ(find_android_header(bir.get(), file.get(),
                                  ANDROID_MAX_HEADER_OFFSET,
                                  &header, &offset), MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "Android magic not found"));
}

TEST(FindAndroidHeaderTest, TruncatedHeaderShouldWarn)
{
    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    AndroidHeader header;
    uint64_t offset;

    ASSERT_EQ(mb_file_open_memory_static(file.get(), ANDROID_BOOT_MAGIC,
                                         ANDROID_BOOT_MAGIC_SIZE), MB_FILE_OK);

    ASSERT_EQ(find_android_header(bir.get(), file.get(),
                                  ANDROID_MAX_HEADER_OFFSET, &header, &offset),
              MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "exceeds file size"));
}

// Tests for find_samsung_seandroid_magic()

TEST(FindSEAndroidMagicTest, ValidMagicShouldSucceed)
{
    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    AndroidHeader source = {};
    memcpy(source.magic, ANDROID_BOOT_MAGIC, ANDROID_BOOT_MAGIC_SIZE);
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

    ASSERT_EQ(mb_file_open_memory_static(file.get(), data.data(), data.size()),
              MB_FILE_OK);

    ASSERT_EQ(find_samsung_seandroid_magic(bir.get(), file.get(),
                                           &source, &offset), MB_BI_OK);
}

TEST(FindSEAndroidMagicTest, UndersizedImageShouldWarn)
{
    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    AndroidHeader source = {};
    memcpy(source.magic, ANDROID_BOOT_MAGIC, ANDROID_BOOT_MAGIC_SIZE);
    source.kernel_size = 0;
    source.ramdisk_size = 0;
    source.second_size = 0;
    source.dt_size = 0;
    source.page_size = 2048;

    uint64_t offset;

    ASSERT_EQ(mb_file_open_memory_static(file.get(), nullptr, 0), MB_FILE_OK);

    ASSERT_EQ(find_samsung_seandroid_magic(bir.get(), file.get(),
                                           &source, &offset), MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "SEAndroid magic not found"));
}

TEST(FindSEAndroidMagicTest, InvalidMagicShouldWarn)
{
    ScopedFile file(mb_file_new(), &mb_file_free);
    ASSERT_TRUE(!!file);
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    AndroidHeader source = {};
    memcpy(source.magic, ANDROID_BOOT_MAGIC, ANDROID_BOOT_MAGIC_SIZE);
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

    ASSERT_EQ(mb_file_open_memory_static(file.get(), data.data(), data.size()),
              MB_FILE_OK);

    ASSERT_EQ(find_samsung_seandroid_magic(bir.get(), file.get(),
                                           &source, &offset), MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "SEAndroid magic not found"));
}

// Tests for android_set_header()

TEST(AndroidSetHeaderTest, ValuesShouldMatch)
{
    ScopedHeader header(mb_bi_header_new(), &mb_bi_header_free);
    ASSERT_TRUE(!!header);

    AndroidHeader ahdr = {};
    memcpy(ahdr.magic, ANDROID_BOOT_MAGIC, ANDROID_BOOT_MAGIC_SIZE);
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

    ASSERT_EQ(android_set_header(&ahdr, header.get()), MB_BI_OK);

    ASSERT_EQ(mb_bi_header_supported_fields(header.get()),
              ANDROID_SUPPORTED_FIELDS);

    const char *board_name = mb_bi_header_board_name(header.get());
    ASSERT_TRUE(board_name);
    ASSERT_STREQ(board_name, reinterpret_cast<char *>(ahdr.name));

    const char *cmdline = mb_bi_header_kernel_cmdline(header.get());
    ASSERT_TRUE(cmdline);
    ASSERT_STREQ(cmdline, reinterpret_cast<char *>(ahdr.cmdline));

    ASSERT_TRUE(mb_bi_header_page_size_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_page_size(header.get()), ahdr.page_size);

    ASSERT_TRUE(mb_bi_header_kernel_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_kernel_address(header.get()), ahdr.kernel_addr);

    ASSERT_TRUE(mb_bi_header_ramdisk_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_ramdisk_address(header.get()), ahdr.ramdisk_addr);

    ASSERT_TRUE(mb_bi_header_secondboot_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_secondboot_address(header.get()), ahdr.second_addr);

    ASSERT_TRUE(mb_bi_header_kernel_tags_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_kernel_tags_address(header.get()), ahdr.tags_addr);
}

struct AndroidReaderGoToEntryTest : testing::Test
{
    ScopedFile _file;
    ScopedReader _bir;
    std::vector<unsigned char> _data;
    MbBiHeader *_header;

    AndroidReaderGoToEntryTest()
        : _file(mb_file_new(), &mb_file_free)
        , _bir(mb_bi_reader_new(), &mb_bi_reader_free)
    {
    }

    virtual ~AndroidReaderGoToEntryTest()
    {
    }

    virtual void SetUp() override
    {
        ASSERT_TRUE(!!_file);
        ASSERT_TRUE(!!_bir);

        AndroidHeader ahdr = {};
        memcpy(ahdr.magic, ANDROID_BOOT_MAGIC, ANDROID_BOOT_MAGIC_SIZE);
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

        ASSERT_EQ(mb_file_open_memory_static(_file.get(), _data.data(),
                                             _data.size()), MB_FILE_OK);

        ASSERT_EQ(mb_bi_reader_enable_format_android(_bir.get()), MB_BI_OK);
        ASSERT_EQ(mb_bi_reader_open(_bir.get(), _file.get(), false), MB_BI_OK);

        ASSERT_EQ(mb_bi_reader_read_header(_bir.get(), &_header), MB_BI_OK);
    }
};

TEST_F(AndroidReaderGoToEntryTest, GoToShouldSucceed)
{
    MbBiEntry *entry;
    char buf[50];
    size_t n;

    ASSERT_EQ(mb_bi_reader_go_to_entry(_bir.get(), &entry, MB_BI_ENTRY_RAMDISK),
              MB_BI_OK);
    ASSERT_EQ(mb_bi_entry_type(entry), MB_BI_ENTRY_RAMDISK);
    ASSERT_EQ(mb_bi_reader_read_data(_bir.get(), buf, sizeof(buf), &n),
              MB_BI_OK);
    ASSERT_EQ(n, 7);
    ASSERT_EQ(memcmp(buf, "ramdisk", n), 0);

    // We should continue at the next entry
    ASSERT_EQ(mb_bi_reader_read_entry(_bir.get(), &entry), MB_BI_OK);
    ASSERT_EQ(mb_bi_entry_type(entry), MB_BI_ENTRY_SECONDBOOT);
    ASSERT_EQ(mb_bi_reader_read_data(_bir.get(), buf, sizeof(buf), &n),
              MB_BI_OK);
    ASSERT_EQ(n, 10);
    ASSERT_EQ(memcmp(buf, "secondboot", n), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToFirstEntryShouldSucceed)
{
    MbBiEntry *entry;
    char buf[50];
    size_t n;

    ASSERT_EQ(mb_bi_reader_go_to_entry(_bir.get(), &entry, 0), MB_BI_OK);
    ASSERT_EQ(mb_bi_entry_type(entry), MB_BI_ENTRY_KERNEL);
    ASSERT_EQ(mb_bi_reader_read_data(_bir.get(), buf, sizeof(buf), &n),
              MB_BI_OK);
    ASSERT_EQ(n, 6);
    ASSERT_EQ(memcmp(buf, "kernel", n), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToPreviousEntryShouldSucceed)
{
    MbBiEntry *entry;
    char buf[50];
    size_t n;

    ASSERT_EQ(mb_bi_reader_go_to_entry(_bir.get(), &entry,
                                       MB_BI_ENTRY_SECONDBOOT),
              MB_BI_OK);
    ASSERT_EQ(mb_bi_entry_type(entry), MB_BI_ENTRY_SECONDBOOT);
    ASSERT_EQ(mb_bi_reader_read_data(_bir.get(), buf, sizeof(buf), &n),
              MB_BI_OK);
    ASSERT_EQ(n, 10);
    ASSERT_EQ(memcmp(buf, "secondboot", n), 0);

    // Go back to kernel
    ASSERT_EQ(mb_bi_reader_go_to_entry(_bir.get(), &entry, MB_BI_ENTRY_KERNEL),
              MB_BI_OK);
    ASSERT_EQ(mb_bi_entry_type(entry), MB_BI_ENTRY_KERNEL);
    ASSERT_EQ(mb_bi_reader_read_data(_bir.get(), buf, sizeof(buf), &n),
              MB_BI_OK);
    ASSERT_EQ(n, 6);
    ASSERT_EQ(memcmp(buf, "kernel", n), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToAfterEOFShouldSucceed)
{
    MbBiEntry *entry;
    char buf[50];
    size_t n;
    int ret;

    while ((ret = mb_bi_reader_read_entry(_bir.get(), &entry)) == MB_BI_OK);
    ASSERT_EQ(ret, MB_BI_EOF);

    ASSERT_EQ(mb_bi_reader_go_to_entry(_bir.get(), &entry, MB_BI_ENTRY_KERNEL),
              MB_BI_OK);
    ASSERT_EQ(mb_bi_entry_type(entry), MB_BI_ENTRY_KERNEL);
    ASSERT_EQ(mb_bi_reader_read_data(_bir.get(), buf, sizeof(buf), &n),
              MB_BI_OK);
    ASSERT_EQ(n, 6);
    ASSERT_EQ(memcmp(buf, "kernel", n), 0);
}

TEST_F(AndroidReaderGoToEntryTest, GoToMissingEntryShouldFail)
{
    MbBiEntry *entry;

    ASSERT_EQ(mb_bi_reader_go_to_entry(_bir.get(), &entry,
                                       MB_BI_ENTRY_DEVICE_TREE),
              MB_BI_EOF);

    // In EOF state now, so next read should return MB_BI_EOF
    ASSERT_EQ(mb_bi_reader_read_entry(_bir.get(), &entry), MB_BI_EOF);
}
