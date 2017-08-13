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

#include "mbbootimg/format/loki_reader_p.h"
#include "mbbootimg/reader.h"

typedef std::unique_ptr<MbBiHeader, decltype(mb_bi_header_free) *> ScopedHeader;
typedef std::unique_ptr<MbBiReader, decltype(mb_bi_reader_free) *> ScopedReader;

using namespace mb::bootimg;
using namespace mb::bootimg::loki;

// Tests for find_loki_header()

TEST(FindLokiHeaderTest, ValidMagicShouldSucceed)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    LokiHeader source = {};
    memcpy(source.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE);

    LokiHeader header;
    uint64_t offset;

    std::vector<unsigned char> data;
    data.resize(LOKI_MAGIC_OFFSET);
    data.insert(data.end(),
                reinterpret_cast<unsigned char *>(&source),
                reinterpret_cast<unsigned char *>(&source) + sizeof(source));

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(find_loki_header(bir.get(), file, header, offset), MB_BI_OK);
}

TEST(FindLokiHeaderTest, UndersizedImageShouldWarn)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    LokiHeader header;
    uint64_t offset;

    mb::MemoryFile file(static_cast<const void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(find_loki_header(bir.get(), file, header, offset), MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()), "Too small"));
}

TEST(FindLokiHeaderTest, InvalidMagicShouldWarn)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

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

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(find_loki_header(bir.get(), file, header, offset), MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "Invalid loki magic"));
}

// Tests for loki_find_ramdisk_address()

TEST(LokiFindRamdiskAddressTest, OldImageShouldUseJflteAddress)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

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

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_find_ramdisk_address(bir.get(), file, ahdr, lhdr,
                                        ramdisk_addr), MB_BI_OK);

    ASSERT_EQ(ramdisk_addr, ahdr.kernel_addr + 0x01ff8000);
}

TEST(LokiFindRamdiskAddressTest, NewImageValidShouldSucceed)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

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

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_find_ramdisk_address(bir.get(), file, ahdr, lhdr,
                                        ramdisk_addr), MB_BI_OK);

    ASSERT_EQ(ramdisk_addr, 0xddccbbaa);
}

TEST(LokiFindRamdiskAddressTest, NewImageMissingShellcodeShouldWarn)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    android::AndroidHeader ahdr = {};

    LokiHeader lhdr = {};
    lhdr.ramdisk_addr = 0x82200000;

    uint32_t ramdisk_addr;

    mb::MemoryFile file(static_cast<const void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_find_ramdisk_address(bir.get(), file, ahdr, lhdr,
                                        ramdisk_addr), MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "Loki shellcode not found"));
}

TEST(LokiFindRamdiskAddressTest, NewImageTruncatedShellcodeShouldWarn)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

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

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_find_ramdisk_address(bir.get(), file, ahdr, lhdr,
                                        ramdisk_addr), MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "Unexpected EOF"));
}

// Tests for loki_old_find_gzip_offset()

TEST(LokiOldFindGzipOffsetTest, ZeroFlagHeaderFoundShouldSucceed)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x00,
    };

    uint64_t gzip_offset;

    mb::MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_old_find_gzip_offset(bir.get(), file, 0, gzip_offset),
              MB_BI_OK);

    ASSERT_EQ(gzip_offset, 0u);
}

TEST(LokiOldFindGzipOffsetTest, EightFlagHeaderFoundShouldSucceed)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x08,
    };

    uint64_t gzip_offset;

    mb::MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_old_find_gzip_offset(bir.get(), file, 0, gzip_offset),
              MB_BI_OK);

    ASSERT_EQ(gzip_offset, 0u);
}

TEST(LokiOldFindGzipOffsetTest, EightFlagShouldHavePrecedence)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x00,
        0x1f, 0x8b, 0x08, 0x08,
    };

    uint64_t gzip_offset;

    mb::MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_old_find_gzip_offset(bir.get(), file, 0, gzip_offset),
              MB_BI_OK);

    ASSERT_EQ(gzip_offset, 4u);
}

TEST(LokiOldFindGzipOffsetTest, StartOffsetShouldBeRespected)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    unsigned char data[] = {
        0x1f, 0x8b, 0x08, 0x00,
        0x1f, 0x8b, 0x08, 0x00,
    };

    uint64_t gzip_offset;

    mb::MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_old_find_gzip_offset(bir.get(), file, 4, gzip_offset),
              MB_BI_OK);

    ASSERT_EQ(gzip_offset, 4u);
}

TEST(LokiOldFindGzipOffsetTest, MissingMagicShouldWarn)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    uint64_t gzip_offset;

    mb::MemoryFile file(static_cast<const void *>(nullptr), 0);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_old_find_gzip_offset(bir.get(), file, 4, gzip_offset),
              MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "No gzip headers found"));
}

TEST(LokiOldFindGzipOffsetTest, MissingFlagsShouldWarn)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    unsigned char data[] = {
        0x1f, 0x8b, 0x08,
    };

    uint64_t gzip_offset;

    mb::MemoryFile file(data, sizeof(data));
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_old_find_gzip_offset(bir.get(), file, 4, gzip_offset),
              MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()),
                       "No gzip headers found"));
}

// Tests for loki_old_find_ramdisk_size()

TEST(LokiOldFindRamdiskSizeTest, ValidSamsungImageShouldSucceed)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    android::AndroidHeader ahdr = {};
    ahdr.ramdisk_addr = 0x88e0ff90; // jflteatt
    ahdr.page_size = 2048;

    std::vector<unsigned char> data;
    data.resize(2 * ahdr.page_size + 0x200);

    uint32_t ramdisk_size;

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_old_find_ramdisk_size(bir.get(), file, ahdr,
                                         ahdr.page_size, ramdisk_size),
              MB_BI_OK);

    ASSERT_EQ(ramdisk_size, ahdr.page_size);
}

TEST(LokiOldFindRamdiskSizeTest, ValidLGImageShouldSucceed)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    android::AndroidHeader ahdr = {};
    ahdr.ramdisk_addr = 0xf8132a0; // LG G2 (AT&T)
    ahdr.page_size = 2048;

    std::vector<unsigned char> data;
    data.resize(3 * ahdr.page_size);

    uint32_t ramdisk_size;

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_old_find_ramdisk_size(bir.get(), file, ahdr,
                                         ahdr.page_size, ramdisk_size),
              MB_BI_OK);

    ASSERT_EQ(ramdisk_size, ahdr.page_size);
}

TEST(LokiOldFindRamdiskSizeTest, OutOfBoundsRamdiskOffsetShouldFail)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    android::AndroidHeader ahdr = {};
    ahdr.ramdisk_addr = 0x88e0ff90; // jflteatt
    ahdr.page_size = 2048;

    std::vector<unsigned char> data;
    data.resize(2 * ahdr.page_size + 0x200);

    uint32_t ramdisk_size;

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_old_find_ramdisk_size(bir.get(), file, ahdr,
                                         data.size() + 1, ramdisk_size),
              MB_BI_FAILED);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()), "greater than"));
}

// Tests for find_linux_kernel_size()

TEST(FindLinuxKernelSizeTest, ValidImageShouldSucceed)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    std::vector<unsigned char> data;
    data.resize(2048 + 0x2c);
    data.push_back(0xaa);
    data.push_back(0xbb);
    data.push_back(0xcc);
    data.push_back(0xdd);

    uint32_t kernel_size;

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(find_linux_kernel_size(bir.get(), file, 2048, kernel_size),
              MB_BI_OK);

    ASSERT_EQ(kernel_size, 0xddccbbaa);
}

TEST(FindLinuxKernelSizeTest, TruncatedImageShouldWarn)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);

    std::vector<unsigned char> data;
    data.resize(2048 + 0x2c);

    uint32_t kernel_size;

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(find_linux_kernel_size(bir.get(), file, 2048, kernel_size),
              MB_BI_WARN);
    ASSERT_TRUE(strstr(mb_bi_reader_error_string(bir.get()), "Unexpected EOF"));
}

// Tests for loki_read_old_header()

TEST(LokiReadOldHeaderTest, ValidImageShouldSucceed)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);
    ScopedHeader header(mb_bi_header_new(), &mb_bi_header_free);
    ASSERT_TRUE(!!header);

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

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_read_old_header(bir.get(), file, ahdr, lhdr,
                                   header.get(), kernel_offset, kernel_size,
                                   ramdisk_offset, ramdisk_size),
              MB_BI_OK);

    // Board name
    const char *board_name = mb_bi_header_board_name(header.get());
    ASSERT_TRUE(board_name);
    ASSERT_STREQ(board_name, reinterpret_cast<char *>(ahdr.name));

    // Kernel cmdline
    const char *cmdline = mb_bi_header_kernel_cmdline(header.get());
    ASSERT_TRUE(cmdline);
    ASSERT_STREQ(cmdline, reinterpret_cast<char *>(ahdr.cmdline));

    // Page size
    ASSERT_TRUE(mb_bi_header_page_size_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_page_size(header.get()), ahdr.page_size);

    // Kernel address
    ASSERT_TRUE(mb_bi_header_kernel_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_kernel_address(header.get()), ahdr.kernel_addr);

    // Ramdisk address
    ASSERT_TRUE(mb_bi_header_ramdisk_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_ramdisk_address(header.get()),
              ahdr.kernel_addr + 0x01ff8000);

    // Kernel tags address
    ASSERT_TRUE(mb_bi_header_kernel_tags_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_kernel_tags_address(header.get()),
              mb_bi_header_kernel_address(header.get())
            - android::DEFAULT_KERNEL_OFFSET + android::DEFAULT_TAGS_OFFSET);

    // Kernel image
    ASSERT_EQ(kernel_offset, ahdr.page_size);
    ASSERT_EQ(kernel_size, ahdr.page_size);

    // Ramdisk image
    ASSERT_EQ(ramdisk_offset, 2 * ahdr.page_size);
    ASSERT_EQ(ramdisk_size, ahdr.page_size);
}

// Tests for loki_read_new_header()

TEST(LokiReadNewHeaderTest, ValidImageShouldSucceed)
{
    ScopedReader bir(mb_bi_reader_new(), &mb_bi_reader_free);
    ASSERT_TRUE(!!bir);
    ScopedHeader header(mb_bi_header_new(), &mb_bi_header_free);
    ASSERT_TRUE(!!header);

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

    mb::MemoryFile file(data.data(), data.size());
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(loki_read_new_header(bir.get(), file, ahdr, lhdr,
                                   header.get(), kernel_offset, kernel_size,
                                   ramdisk_offset, ramdisk_size, dt_offset),
              MB_BI_OK);

    // Board name
    const char *board_name = mb_bi_header_board_name(header.get());
    ASSERT_TRUE(board_name);
    ASSERT_STREQ(board_name, reinterpret_cast<char *>(ahdr.name));

    // Kernel cmdline
    const char *cmdline = mb_bi_header_kernel_cmdline(header.get());
    ASSERT_TRUE(cmdline);
    ASSERT_STREQ(cmdline, reinterpret_cast<char *>(ahdr.cmdline));

    // Page size
    ASSERT_TRUE(mb_bi_header_page_size_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_page_size(header.get()), ahdr.page_size);

    // Kernel address
    ASSERT_TRUE(mb_bi_header_kernel_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_kernel_address(header.get()), ahdr.kernel_addr);

    // Ramdisk address
    ASSERT_TRUE(mb_bi_header_ramdisk_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_ramdisk_address(header.get()), 0xddccbbaa);

    // Kernel tags address
    ASSERT_TRUE(mb_bi_header_kernel_tags_address_is_set(header.get()));
    ASSERT_EQ(mb_bi_header_kernel_tags_address(header.get()), ahdr.tags_addr);

    // Kernel image
    ASSERT_EQ(kernel_offset, ahdr.page_size);
    ASSERT_EQ(kernel_size, ahdr.page_size);

    // Ramdisk image
    ASSERT_EQ(ramdisk_offset, 2 * ahdr.page_size);
    ASSERT_EQ(ramdisk_size, ahdr.page_size);
}
