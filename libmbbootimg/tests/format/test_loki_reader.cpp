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

#include "mbcommon/file.h"
#include "mbcommon/file/memory.h"
#include "mbcommon/file_error.h"

#include "mbbootimg/format/loki_error.h"
#include "mbbootimg/format/loki_reader_p.h"

using namespace mb;
using namespace mb::bootimg;
using namespace mb::bootimg::loki;

// Tests for find_loki_header()

TEST(FindLokiHeaderTest, ValidMagicShouldSucceed)
{
    LokiHeader source = {};
    memcpy(source.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE);

    std::vector<unsigned char> data;
    data.resize(LOKI_MAGIC_OFFSET);
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&source),
                reinterpret_cast<unsigned char *>(&source) + sizeof(source));

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(LokiFormatReader::find_loki_header(file));
}

TEST(FindLokiHeaderTest, UndersizedImageShouldFail)
{
    MemoryFile file(static_cast<void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_loki_header(file);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::LokiHeaderTooSmall);
}

TEST(FindLokiHeaderTest, InvalidMagicShouldFail)
{
    LokiHeader source = {};
    memcpy(source.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE);

    std::vector<unsigned char> data;
    data.resize(LOKI_MAGIC_OFFSET);
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&source),
                reinterpret_cast<unsigned char *>(&source) + sizeof(source));
    data[LOKI_MAGIC_OFFSET] = 'x';

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_loki_header(file);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::InvalidLokiMagic);
}

// Tests for find_ramdisk_address()

TEST(LokiFindRamdiskAddressTest, OldImageShouldUseJflteAddress)
{
    android::AndroidHeader ahdr = {};
    ahdr.kernel_addr = 0x80208000;

    LokiHeader lhdr = {};

    std::vector<unsigned char> data;
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&ahdr),
                reinterpret_cast<unsigned char *>(&ahdr) + sizeof(ahdr));
    data.resize(LOKI_MAGIC_OFFSET);
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&lhdr),
                reinterpret_cast<unsigned char *>(&lhdr) + sizeof(lhdr));

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ramdisk_addr = LokiFormatReader::find_ramdisk_address(file, ahdr, lhdr);
    ASSERT_TRUE(ramdisk_addr);

    ASSERT_EQ(ramdisk_addr.value(), ahdr.kernel_addr + 0x01ff8000);
}

TEST(LokiFindRamdiskAddressTest, NewImageValidShouldSucceed)
{
    android::AndroidHeader ahdr = {};

    LokiHeader lhdr = {};
    lhdr.ramdisk_addr = 0x82200000;

    std::vector<unsigned char> data;
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&ahdr),
                reinterpret_cast<unsigned char *>(&ahdr) + sizeof(ahdr));
    data.resize(LOKI_MAGIC_OFFSET);
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&lhdr),
                reinterpret_cast<unsigned char *>(&lhdr) + sizeof(lhdr));
    data.insert(data.end(), LOKI_SHELLCODE,
                LOKI_SHELLCODE + LOKI_SHELLCODE_SIZE - 5);
    data.push_back(0xaa);
    data.push_back(0xbb);
    data.push_back(0xcc);
    data.push_back(0xdd);
    data.push_back(0x00);

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ramdisk_addr = LokiFormatReader::find_ramdisk_address(file, ahdr, lhdr);
    ASSERT_TRUE(ramdisk_addr);

    ASSERT_EQ(ramdisk_addr.value(), 0xddccbbaa);
}

TEST(LokiFindRamdiskAddressTest, NewImageMissingShellcodeShouldFail)
{
    android::AndroidHeader ahdr = {};

    LokiHeader lhdr = {};
    lhdr.ramdisk_addr = 0x82200000;

    MemoryFile file(static_cast<void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_ramdisk_address(file, ahdr, lhdr);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::ShellcodeNotFound);
}

TEST(LokiFindRamdiskAddressTest, NewImageTruncatedShellcodeShouldFail)
{
    android::AndroidHeader ahdr = {};

    LokiHeader lhdr = {};
    lhdr.ramdisk_addr = 0x82200000;

    std::vector<unsigned char> data;
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&ahdr),
                reinterpret_cast<unsigned char *>(&ahdr) + sizeof(ahdr));
    data.resize(LOKI_MAGIC_OFFSET);
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&lhdr),
                reinterpret_cast<unsigned char *>(&lhdr) + sizeof(lhdr));
    data.insert(data.end(), LOKI_SHELLCODE,
                LOKI_SHELLCODE + LOKI_SHELLCODE_SIZE - 5);

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_ramdisk_address(file, ahdr, lhdr);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), FileError::UnexpectedEof);
}

// Tests for find_gzip_offset_old()

TEST(LokiOldFindGzipOffsetTest, ZeroFlagHeaderFoundShouldSucceed)
{
    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x00,
    };

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    auto gzip_offset = LokiFormatReader::find_gzip_offset_old(file, 0);
    ASSERT_TRUE(gzip_offset);

    ASSERT_EQ(gzip_offset.value(), 0u);
}

TEST(LokiOldFindGzipOffsetTest, EightFlagHeaderFoundShouldSucceed)
{
    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x08,
    };

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    auto gzip_offset = LokiFormatReader::find_gzip_offset_old(file, 0);
    ASSERT_TRUE(gzip_offset);

    ASSERT_EQ(gzip_offset.value(), 0u);
}

TEST(LokiOldFindGzipOffsetTest, EightFlagShouldHavePrecedence)
{
    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x00,
        0x1f, 0x8b, 0x08, 0x08,
    };

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    auto gzip_offset = LokiFormatReader::find_gzip_offset_old(file, 0);
    ASSERT_TRUE(gzip_offset);

    ASSERT_EQ(gzip_offset.value(), 4u);
}

TEST(LokiOldFindGzipOffsetTest, StartOffsetShouldBeRespected)
{
    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x00,
        0x1f, 0x8b, 0x08, 0x00,
    };

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    auto gzip_offset = LokiFormatReader::find_gzip_offset_old(file, 4);
    ASSERT_TRUE(gzip_offset);

    ASSERT_EQ(gzip_offset.value(), 4u);
}

TEST(LokiOldFindGzipOffsetTest, MissingMagicShouldFail)
{
    MemoryFile file(static_cast<void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_gzip_offset_old(file, 4);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::NoRamdiskGzipHeaderFound);
}

TEST(LokiOldFindGzipOffsetTest, MissingFlagsShouldFail)
{
    unsigned char data[] = {
        0x1f, 0x8b, 0x08,
    };

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_gzip_offset_old(file, 4);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::NoRamdiskGzipHeaderFound);
}

// Tests for find_ramdisk_size_old()

TEST(LokiOldFindRamdiskSizeTest, ValidSamsungImageShouldSucceed)
{
    android::AndroidHeader ahdr = {};
    ahdr.ramdisk_addr = 0x88e0ff90; // jflteatt
    ahdr.page_size = 2048;

    std::vector<unsigned char> data;
    data.resize(2 * ahdr.page_size + 0x200);

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ramdisk_size = LokiFormatReader::find_ramdisk_size_old(
            file, ahdr, ahdr.page_size);
    ASSERT_TRUE(ramdisk_size);

    ASSERT_EQ(ramdisk_size.value(), ahdr.page_size);
}

TEST(LokiOldFindRamdiskSizeTest, ValidLGImageShouldSucceed)
{
    android::AndroidHeader ahdr = {};
    ahdr.ramdisk_addr = 0xf8132a0; // LG G2 (AT&T)
    ahdr.page_size = 2048;

    std::vector<unsigned char> data;
    data.resize(3 * ahdr.page_size);

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ramdisk_size = LokiFormatReader::find_ramdisk_size_old(
            file, ahdr, ahdr.page_size);
    ASSERT_TRUE(ramdisk_size);

    ASSERT_EQ(ramdisk_size.value(), ahdr.page_size);
}

TEST(LokiOldFindRamdiskSizeTest, OutOfBoundsRamdiskOffsetShouldFail)
{
    android::AndroidHeader ahdr = {};
    ahdr.ramdisk_addr = 0x88e0ff90; // jflteatt
    ahdr.page_size = 2048;

    std::vector<unsigned char> data;
    data.resize(2 * ahdr.page_size + 0x200);

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_ramdisk_size_old(
            file, ahdr, static_cast<uint32_t>(data.size() + 1));
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::RamdiskOffsetGreaterThanAbootOffset);
}

// Tests for find_linux_kernel_size()

TEST(FindLinuxKernelSizeTest, ValidImageShouldSucceed)
{
    std::vector<unsigned char> data;
    data.resize(2048 + 0x2c);
    data.push_back(0xaa);
    data.push_back(0xbb);
    data.push_back(0xcc);
    data.push_back(0xdd);

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto kernel_size = LokiFormatReader::find_linux_kernel_size(file, 2048);
    ASSERT_TRUE(kernel_size);

    ASSERT_EQ(kernel_size.value(), 0xddccbbaa);
}

TEST(FindLinuxKernelSizeTest, TruncatedImageShouldFail)
{
    std::vector<unsigned char> data;
    data.resize(2048 + 0x2c);

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_linux_kernel_size(file, 2048);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), FileError::UnexpectedEof);
}

// Tests for read_header_old()

TEST(LokiReadOldHeaderTest, ValidImageShouldSucceed)
{
    android::AndroidHeader ahdr = {};
    memcpy(ahdr.magic, android::BOOT_MAGIC, android::BOOT_MAGIC_SIZE);
    ahdr.kernel_addr = 0x11223344;
    ahdr.ramdisk_addr = 0x88e0ff90; // jflteatt
    ahdr.page_size = 2048;
    snprintf(reinterpret_cast<char *>(ahdr.name), sizeof(ahdr.name),
             "Test board name");
    snprintf(reinterpret_cast<char *>(ahdr.cmdline), sizeof(ahdr.cmdline),
             "Test cmdline");

    LokiHeader lhdr = {};
    memcpy(lhdr.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE);

    std::vector<unsigned char> data(3 * ahdr.page_size + 0x200);

    // Write headers
    memcpy(data.data(), &ahdr, sizeof(ahdr));
    memcpy(data.data() + LOKI_MAGIC_OFFSET, &lhdr, sizeof(lhdr));

    // Write kernel
    data[ahdr.page_size + 0x2c + 0] = 0x00;
    data[ahdr.page_size + 0x2c + 1] = 0x08;
    data[ahdr.page_size + 0x2c + 0] = 0x00;
    data[ahdr.page_size + 0x2c + 0] = 0x00;

    // Write ramdisk
    data[2 * ahdr.page_size + 0] = 0x1f;
    data[2 * ahdr.page_size + 1] = 0x8b;
    data[2 * ahdr.page_size + 2] = 0x08;
    data[2 * ahdr.page_size + 3] = 0x08;

    // Write shellcode
    memcpy(data.data() + 3 * ahdr.page_size, LOKI_SHELLCODE,
           LOKI_SHELLCODE_SIZE);
    data[3 * ahdr.page_size + LOKI_SHELLCODE_SIZE - 5] = 0xaa;
    data[3 * ahdr.page_size + LOKI_SHELLCODE_SIZE - 4] = 0xbb;
    data[3 * ahdr.page_size + LOKI_SHELLCODE_SIZE - 3] = 0xcc;
    data[3 * ahdr.page_size + LOKI_SHELLCODE_SIZE - 2] = 0xdd;

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto result = LokiFormatReader::read_header_old(file, ahdr, lhdr);
    ASSERT_TRUE(result);

    // Board name
    auto board_name = result.value().header.board_name();
    ASSERT_TRUE(board_name);
    ASSERT_EQ(*board_name, reinterpret_cast<char *>(ahdr.name));

    // Kernel cmdline
    auto cmdline = result.value().header.kernel_cmdline();
    ASSERT_TRUE(cmdline);
    ASSERT_EQ(*cmdline, reinterpret_cast<char *>(ahdr.cmdline));

    // Page size
    auto page_size = result.value().header.page_size();
    ASSERT_TRUE(page_size);
    ASSERT_EQ(*page_size, ahdr.page_size);

    // Kernel address
    auto kernel_address = result.value().header.kernel_address();
    ASSERT_TRUE(kernel_address);
    ASSERT_EQ(*kernel_address, ahdr.kernel_addr);

    // Ramdisk address
    auto ramdisk_address = result.value().header.ramdisk_address();
    ASSERT_TRUE(ramdisk_address);
    ASSERT_EQ(*ramdisk_address, ahdr.kernel_addr + 0x01ff8000);

    // Kernel tags address
    auto kernel_tags_address = result.value().header.kernel_tags_address();
    ASSERT_TRUE(kernel_tags_address);
    ASSERT_EQ(*kernel_tags_address, *result.value().header.kernel_address()
            - android::DEFAULT_KERNEL_OFFSET + android::DEFAULT_TAGS_OFFSET);

    // Kernel image
    ASSERT_EQ(result.value().kernel_offset, ahdr.page_size);
    ASSERT_EQ(result.value().kernel_size, ahdr.page_size);

    // Ramdisk image
    ASSERT_EQ(result.value().ramdisk_offset, 2 * ahdr.page_size);
    ASSERT_EQ(result.value().ramdisk_size, ahdr.page_size);
}

// Tests for read_header_new()

TEST(LokiReadNewHeaderTest, ValidImageShouldSucceed)
{
    android::AndroidHeader ahdr = {};
    memcpy(ahdr.magic, android::BOOT_MAGIC, android::BOOT_MAGIC_SIZE);
    ahdr.kernel_addr = 0x11223344;
    ahdr.ramdisk_addr = 0x88e0ff90; // jflteatt
    ahdr.tags_addr = 0x22334455;
    ahdr.page_size = 2048;
    snprintf(reinterpret_cast<char *>(ahdr.name), sizeof(ahdr.name),
             "Test board name");
    snprintf(reinterpret_cast<char *>(ahdr.cmdline), sizeof(ahdr.cmdline),
             "Test cmdline");

    LokiHeader lhdr = {};
    memcpy(lhdr.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE);
    lhdr.orig_kernel_size = ahdr.page_size;
    lhdr.orig_ramdisk_size = ahdr.page_size;
    lhdr.ramdisk_addr = 0x82200000;

    std::vector<unsigned char> data(3 * ahdr.page_size + 0x200);

    // Write headers
    memcpy(data.data(), &ahdr, sizeof(ahdr));
    memcpy(data.data() + LOKI_MAGIC_OFFSET, &lhdr, sizeof(lhdr));

    // Write kernel
    data[ahdr.page_size + 0x2c + 0] = 0x00;
    data[ahdr.page_size + 0x2c + 1] = 0x08;
    data[ahdr.page_size + 0x2c + 0] = 0x00;
    data[ahdr.page_size + 0x2c + 0] = 0x00;

    // Write ramdisk
    data[2 * ahdr.page_size + 0] = 0x1f;
    data[2 * ahdr.page_size + 1] = 0x8b;
    data[2 * ahdr.page_size + 2] = 0x08;
    data[2 * ahdr.page_size + 3] = 0x08;

    // Write shellcode
    memcpy(data.data() + 3 * ahdr.page_size, LOKI_SHELLCODE,
           LOKI_SHELLCODE_SIZE);
    data[3 * ahdr.page_size + LOKI_SHELLCODE_SIZE - 5] = 0xaa;
    data[3 * ahdr.page_size + LOKI_SHELLCODE_SIZE - 4] = 0xbb;
    data[3 * ahdr.page_size + LOKI_SHELLCODE_SIZE - 3] = 0xcc;
    data[3 * ahdr.page_size + LOKI_SHELLCODE_SIZE - 2] = 0xdd;

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto result = LokiFormatReader::read_header_new(file, ahdr, lhdr);
    ASSERT_TRUE(result);

    // Board name
    auto board_name = result.value().header.board_name();
    ASSERT_TRUE(board_name);
    ASSERT_EQ(*board_name, reinterpret_cast<char *>(ahdr.name));

    // Kernel cmdline
    auto cmdline = result.value().header.kernel_cmdline();
    ASSERT_TRUE(cmdline);
    ASSERT_EQ(*cmdline, reinterpret_cast<char *>(ahdr.cmdline));

    // Page size
    auto page_size = result.value().header.page_size();
    ASSERT_TRUE(page_size);
    ASSERT_EQ(*page_size, ahdr.page_size);

    // Kernel address
    auto kernel_address = result.value().header.kernel_address();
    ASSERT_TRUE(kernel_address);
    ASSERT_EQ(*kernel_address, ahdr.kernel_addr);

    // Ramdisk address
    auto ramdisk_address = result.value().header.ramdisk_address();
    ASSERT_TRUE(ramdisk_address);
    ASSERT_EQ(*ramdisk_address, 0xddccbbaa);

    // Kernel tags address
    auto kernel_tags_address = result.value().header.kernel_tags_address();
    ASSERT_TRUE(kernel_tags_address);
    ASSERT_EQ(*kernel_tags_address, ahdr.tags_addr);

    // Kernel image
    ASSERT_EQ(result.value().kernel_offset, ahdr.page_size);
    ASSERT_EQ(result.value().kernel_size, ahdr.page_size);

    // Ramdisk image
    ASSERT_EQ(result.value().ramdisk_offset, 2 * ahdr.page_size);
    ASSERT_EQ(result.value().ramdisk_size, ahdr.page_size);
}
