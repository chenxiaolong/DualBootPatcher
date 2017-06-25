/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/endian.h"

TEST(EndianTest, Check16BitConversion)
{
    uint16_t orig = 0x0123u;
    uint16_t swap = 0x2301u;

#if MB_BYTE_ORDER == MB_LITTLE_ENDIAN
    ASSERT_EQ(swap, mb_htobe16(orig));
    ASSERT_EQ(orig, mb_htole16(orig));
    ASSERT_EQ(swap, mb_be16toh(orig));
    ASSERT_EQ(orig, mb_le16toh(orig));
#elif MB_BYTE_ORDER == MB_BIG_ENDIAN
    ASSERT_EQ(orig, mb_htobe16(orig));
    ASSERT_EQ(swap, mb_htole16(orig));
    ASSERT_EQ(orig, mb_be16toh(orig));
    ASSERT_EQ(swap, mb_le16toh(orig));
#else
#  error Unsupported endianness
#endif
}

TEST(EndianTest, Check32BitConversion)
{
    uint32_t orig = 0x01234567u;
    uint32_t swap = 0x67452301u;

#if MB_BYTE_ORDER == MB_LITTLE_ENDIAN
    ASSERT_EQ(swap, mb_htobe32(orig));
    ASSERT_EQ(orig, mb_htole32(orig));
    ASSERT_EQ(swap, mb_be32toh(orig));
    ASSERT_EQ(orig, mb_le32toh(orig));
#elif MB_BYTE_ORDER == MB_BIG_ENDIAN
    ASSERT_EQ(orig, mb_htobe32(orig));
    ASSERT_EQ(swap, mb_htole32(orig));
    ASSERT_EQ(orig, mb_be32toh(orig));
    ASSERT_EQ(swap, mb_le32toh(orig));
#else
#  error Unsupported endianness
#endif
}

TEST(EndianTest, Check64BitConversion)
{
    uint64_t orig = 0x0123456789ABCDEFull;
    uint64_t swap = 0xEFCDAB8967452301ull;

#if MB_BYTE_ORDER == MB_LITTLE_ENDIAN
    ASSERT_EQ(swap, mb_htobe64(orig));
    ASSERT_EQ(orig, mb_htole64(orig));
    ASSERT_EQ(swap, mb_be64toh(orig));
    ASSERT_EQ(orig, mb_le64toh(orig));
#elif MB_BYTE_ORDER == MB_BIG_ENDIAN
    ASSERT_EQ(orig, mb_htobe64(orig));
    ASSERT_EQ(swap, mb_htole64(orig));
    ASSERT_EQ(orig, mb_be64toh(orig));
    ASSERT_EQ(swap, mb_le64toh(orig));
#else
#  error Unsupported endianness
#endif
}
