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

#include "mbbootimg/format/loki_error.h"
#include "mbbootimg/format/loki_reader_p.h"
#include "mbbootimg/reader.h"

using namespace mb;
using namespace mb::bootimg;
using namespace mb::bootimg::loki;

// Tests for find_loki_header()

TEST(FindLokiHeaderTest, ValidMagicShouldSucceed)
{
    Reader reader;

    LokiHeader source = {};
    memcpy(source.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE);

    LokiHeader header;
    uint64_t offset;

    std::vector<unsigned char> data;
    data.resize(LOKI_MAGIC_OFFSET);
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&source),
                reinterpret_cast<unsigned char *>(&source) + sizeof(source));

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(LokiFormatReader::find_loki_header(reader, file, header,
                                                   offset));
}

TEST(FindLokiHeaderTest, UndersizedImageShouldFail)
{
    Reader reader;

    LokiHeader header;
    uint64_t offset;

    MemoryFile file(static_cast<void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_loki_header(reader, file, header, offset);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::LokiHeaderTooSmall);
}

TEST(FindLokiHeaderTest, InvalidMagicShouldFail)
{
    Reader reader;

    LokiHeader source = {};
    memcpy(source.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE);

    LokiHeader header;
    uint64_t offset;

    std::vector<unsigned char> data;
    data.resize(LOKI_MAGIC_OFFSET);
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&source),
                reinterpret_cast<unsigned char *>(&source) + sizeof(source));
    data[LOKI_MAGIC_OFFSET] = 'x';

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_loki_header(reader, file, header, offset);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::InvalidLokiMagic);
}

// Tests for find_ramdisk_address()

TEST(LokiFindRamdiskAddressTest, OldImageShouldUseJflteAddress)
{
    Reader reader;

    android::AndroidHeader ahdr = {};
    ahdr.kernel_addr = 0x80208000;

    LokiHeader lhdr = {};

    uint32_t ramdisk_addr;

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

    ASSERT_TRUE(LokiFormatReader::find_ramdisk_address(reader, file, ahdr, lhdr,
                                                       ramdisk_addr));

    ASSERT_EQ(ramdisk_addr, ahdr.kernel_addr + 0x01ff8000);
}

TEST(LokiFindRamdiskAddressTest, NewImageValidShouldSucceed)
{
    Reader reader;

    android::AndroidHeader ahdr = {};

    LokiHeader lhdr = {};
    lhdr.ramdisk_addr = 0x82200000;

    uint32_t ramdisk_addr;

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

    ASSERT_TRUE(LokiFormatReader::find_ramdisk_address(reader, file, ahdr, lhdr,
                                                       ramdisk_addr));

    ASSERT_EQ(ramdisk_addr, 0xddccbbaa);
}

TEST(LokiFindRamdiskAddressTest, NewImageMissingShellcodeShouldFail)
{
    Reader reader;

    android::AndroidHeader ahdr = {};

    LokiHeader lhdr = {};
    lhdr.ramdisk_addr = 0x82200000;

    uint32_t ramdisk_addr;

    MemoryFile file(static_cast<void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_ramdisk_address(reader, file, ahdr,
                                                      lhdr, ramdisk_addr);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::ShellcodeNotFound);
}

TEST(LokiFindRamdiskAddressTest, NewImageTruncatedShellcodeShouldFail)
{
    Reader reader;

    android::AndroidHeader ahdr = {};

    LokiHeader lhdr = {};
    lhdr.ramdisk_addr = 0x82200000;

    uint32_t ramdisk_addr;

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

    auto ret = LokiFormatReader::find_ramdisk_address(reader, file, ahdr,
                                                      lhdr, ramdisk_addr);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), FileError::UnexpectedEof);
}

// Tests for find_gzip_offset_old()

TEST(LokiOldFindGzipOffsetTest, ZeroFlagHeaderFoundShouldSucceed)
{
    Reader reader;

    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x00,
    };

    uint64_t gzip_offset;

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(LokiFormatReader::find_gzip_offset_old(reader, file, 0,
                                                       gzip_offset));

    ASSERT_EQ(gzip_offset, 0u);
}

TEST(LokiOldFindGzipOffsetTest, EightFlagHeaderFoundShouldSucceed)
{
    Reader reader;

    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x08,
    };

    uint64_t gzip_offset;

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(LokiFormatReader::find_gzip_offset_old(reader, file, 0,
                                                       gzip_offset));

    ASSERT_EQ(gzip_offset, 0u);
}

TEST(LokiOldFindGzipOffsetTest, EightFlagShouldHavePrecedence)
{
    Reader reader;

    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x00,
        0x1f, 0x8b, 0x08, 0x08,
    };

    uint64_t gzip_offset;

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(LokiFormatReader::find_gzip_offset_old(reader, file, 0,
                                                       gzip_offset));

    ASSERT_EQ(gzip_offset, 4u);
}

TEST(LokiOldFindGzipOffsetTest, StartOffsetShouldBeRespected)
{
    Reader reader;

    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x00,
        0x1f, 0x8b, 0x08, 0x00,
    };

    uint64_t gzip_offset;

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(LokiFormatReader::find_gzip_offset_old(reader, file, 4,
                                                       gzip_offset));

    ASSERT_EQ(gzip_offset, 4u);
}

TEST(LokiOldFindGzipOffsetTest, MissingMagicShouldFail)
{
    Reader reader;

    uint64_t gzip_offset;

    MemoryFile file(static_cast<void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_gzip_offset_old(reader, file, 4,
                                                      gzip_offset);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::NoRamdiskGzipHeaderFound);
}

TEST(LokiOldFindGzipOffsetTest, MissingFlagsShouldFail)
{
    Reader reader;

    unsigned char data[] = {
        0x1f, 0x8b, 0x08,
    };

    uint64_t gzip_offset;

    MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_gzip_offset_old(reader, file, 4,
                                                      gzip_offset);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::NoRamdiskGzipHeaderFound);
}

// Tests for find_ramdisk_size_old()

TEST(LokiOldFindRamdiskSizeTest, ValidSamsungImageShouldSucceed)
{
    Reader reader;

    android::AndroidHeader ahdr = {};
    ahdr.ramdisk_addr = 0x88e0ff90; // jflteatt
    ahdr.page_size = 2048;

    std::vector<unsigned char> data;
    data.resize(2 * ahdr.page_size + 0x200);

    uint32_t ramdisk_size;

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(LokiFormatReader::find_ramdisk_size_old(reader, file, ahdr,
                                                        ahdr.page_size,
                                                        ramdisk_size));

    ASSERT_EQ(ramdisk_size, ahdr.page_size);
}

TEST(LokiOldFindRamdiskSizeTest, ValidLGImageShouldSucceed)
{
    Reader reader;

    android::AndroidHeader ahdr = {};
    ahdr.ramdisk_addr = 0xf8132a0; // LG G2 (AT&T)
    ahdr.page_size = 2048;

    std::vector<unsigned char> data;
    data.resize(3 * ahdr.page_size);

    uint32_t ramdisk_size;

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(LokiFormatReader::find_ramdisk_size_old(reader, file, ahdr,
                                                        ahdr.page_size,
                                                        ramdisk_size));

    ASSERT_EQ(ramdisk_size, ahdr.page_size);
}

TEST(LokiOldFindRamdiskSizeTest, OutOfBoundsRamdiskOffsetShouldFail)
{
    Reader reader;

    android::AndroidHeader ahdr = {};
    ahdr.ramdisk_addr = 0x88e0ff90; // jflteatt
    ahdr.page_size = 2048;

    std::vector<unsigned char> data;
    data.resize(2 * ahdr.page_size + 0x200);

    uint32_t ramdisk_size;

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_ramdisk_size_old(reader, file, ahdr,
                                                       static_cast<uint32_t>(data.size() + 1),
                                                       ramdisk_size);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), LokiError::RamdiskOffsetGreaterThanAbootOffset);
}

// Tests for find_linux_kernel_size()

TEST(FindLinuxKernelSizeTest, ValidImageShouldSucceed)
{
    Reader reader;

    std::vector<unsigned char> data;
    data.resize(2048 + 0x2c);
    data.push_back(0xaa);
    data.push_back(0xbb);
    data.push_back(0xcc);
    data.push_back(0xdd);

    uint32_t kernel_size;

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(LokiFormatReader::find_linux_kernel_size(reader, file, 2048,
                                                         kernel_size));

    ASSERT_EQ(kernel_size, 0xddccbbaa);
}

TEST(FindLinuxKernelSizeTest, TruncatedImageShouldFail)
{
    Reader reader;

    std::vector<unsigned char> data;
    data.resize(2048 + 0x2c);

    uint32_t kernel_size;

    MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    auto ret = LokiFormatReader::find_linux_kernel_size(reader, file, 2048,
                                                        kernel_size);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), FileError::UnexpectedEof);
}

// Tests for read_header_old()

TEST(LokiReadOldHeaderTest, ValidImageShouldSucceed)
{
    Reader reader;
    Header header;

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

    uint64_t kernel_offset;
    uint32_t kernel_size;
    uint64_t ramdisk_offset;
    uint32_t ramdisk_size;

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

    ASSERT_TRUE(LokiFormatReader::read_header_old(reader, file, ahdr, lhdr,
                                                  header, kernel_offset,
                                                  kernel_size, ramdisk_offset,
                                                  ramdisk_size));

    // Board name
    auto board_name = header.board_name();
    ASSERT_TRUE(board_name);
    ASSERT_EQ(*board_name, reinterpret_cast<char *>(ahdr.name));

    // Kernel cmdline
    auto cmdline = header.kernel_cmdline();
    ASSERT_TRUE(cmdline);
    ASSERT_EQ(*cmdline, reinterpret_cast<char *>(ahdr.cmdline));

    // Page size
    auto page_size = header.page_size();
    ASSERT_TRUE(page_size);
    ASSERT_EQ(*page_size, ahdr.page_size);

    // Kernel address
    auto kernel_address = header.kernel_address();
    ASSERT_TRUE(kernel_address);
    ASSERT_EQ(*kernel_address, ahdr.kernel_addr);

    // Ramdisk address
    auto ramdisk_address = header.ramdisk_address();
    ASSERT_TRUE(ramdisk_address);
    ASSERT_EQ(*ramdisk_address, ahdr.kernel_addr + 0x01ff8000);

    // Kernel tags address
    auto kernel_tags_address = header.kernel_tags_address();
    ASSERT_TRUE(kernel_tags_address);
    ASSERT_EQ(*kernel_tags_address, *header.kernel_address()
            - android::DEFAULT_KERNEL_OFFSET + android::DEFAULT_TAGS_OFFSET);

    // Kernel image
    ASSERT_EQ(kernel_offset, ahdr.page_size);
    ASSERT_EQ(kernel_size, ahdr.page_size);

    // Ramdisk image
    ASSERT_EQ(ramdisk_offset, 2 * ahdr.page_size);
    ASSERT_EQ(ramdisk_size, ahdr.page_size);
}

// Tests for read_header_new()

TEST(LokiReadNewHeaderTest, ValidImageShouldSucceed)
{
    Reader reader;
    Header header;

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

    uint64_t kernel_offset;
    uint32_t kernel_size;
    uint64_t ramdisk_offset;
    uint32_t ramdisk_size;
    uint64_t dt_offset;

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

    ASSERT_TRUE(LokiFormatReader::read_header_new(reader, file, ahdr, lhdr,
                                                  header, kernel_offset,
                                                  kernel_size, ramdisk_offset,
                                                  ramdisk_size, dt_offset));

    // Board name
    auto board_name = header.board_name();
    ASSERT_TRUE(board_name);
    ASSERT_EQ(*board_name, reinterpret_cast<char *>(ahdr.name));

    // Kernel cmdline
    auto cmdline = header.kernel_cmdline();
    ASSERT_TRUE(cmdline);
    ASSERT_EQ(*cmdline, reinterpret_cast<char *>(ahdr.cmdline));

    // Page size
    auto page_size = header.page_size();
    ASSERT_TRUE(page_size);
    ASSERT_EQ(*page_size, ahdr.page_size);

    // Kernel address
    auto kernel_address = header.kernel_address();
    ASSERT_TRUE(kernel_address);
    ASSERT_EQ(*kernel_address, ahdr.kernel_addr);

    // Ramdisk address
    auto ramdisk_address = header.ramdisk_address();
    ASSERT_TRUE(ramdisk_address);
    ASSERT_EQ(*ramdisk_address, 0xddccbbaa);

    // Kernel tags address
    auto kernel_tags_address = header.kernel_tags_address();
    ASSERT_TRUE(kernel_tags_address);
    ASSERT_EQ(*kernel_tags_address, ahdr.tags_addr);

    // Kernel image
    ASSERT_EQ(kernel_offset, ahdr.page_size);
    ASSERT_EQ(kernel_size, ahdr.page_size);

    // Ramdisk image
    ASSERT_EQ(ramdisk_offset, 2 * ahdr.page_size);
    ASSERT_EQ(ramdisk_size, ahdr.page_size);
}
