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

#include <limits>

#include <cstdint>

#include "mbcommon/integer.h"

using namespace mb;

TEST(IntegerTest, ConvertEmptyString)
{
    int ri;
    ASSERT_FALSE(str_to_num("", 0, ri));
    ASSERT_EQ(errno, EINVAL);

    unsigned int ru;
    ASSERT_FALSE(str_to_num("", 0, ru));
    ASSERT_EQ(errno, EINVAL);
}

TEST(IntegerTest, ConvertInvalidString)
{
    int ri;
    ASSERT_FALSE(str_to_num("abcd", 0, ri));
    ASSERT_EQ(errno, EINVAL);

    unsigned int ru;
    ASSERT_FALSE(str_to_num("abcd", 0, ru));
    ASSERT_EQ(errno, EINVAL);
}

TEST(IntegerTest, ConvertStringWithTrailingNonDigits)
{
    int ri;
    ASSERT_FALSE(str_to_num("1234 ", 0, ri));
    ASSERT_EQ(errno, EINVAL);

    unsigned int ru;
    ASSERT_FALSE(str_to_num("1234 ", 0, ru));
    ASSERT_EQ(errno, EINVAL);
}

TEST(IntegerTest, ConvertSignedIntegerNormal)
{
    int r;

    ASSERT_TRUE(str_to_num("1234", 8, r));
    ASSERT_EQ(r, 01234);

    ASSERT_TRUE(str_to_num("1234", 10, r));
    ASSERT_EQ(r, 1234);

    ASSERT_TRUE(str_to_num("-1234", 16, r));
    ASSERT_EQ(r, -0x1234);
}

TEST(IntegerTest, ConvertUnsignedIntegerNormal)
{
    unsigned int r;

    ASSERT_TRUE(str_to_num("1234", 8, r));
    ASSERT_EQ(r, 01234u);

    ASSERT_TRUE(str_to_num("1234", 10, r));
    ASSERT_EQ(r, 1234u);

    ASSERT_TRUE(str_to_num("1234", 16, r));
    ASSERT_EQ(r, 0x1234u);
}

TEST(IntegerTest, ConvertSignedIntegerLimits)
{
    // 8 bit
    int8_t r8;
    ASSERT_TRUE(str_to_num("-128", 10, r8));
    ASSERT_EQ(r8, std::numeric_limits<int8_t>::min());
    ASSERT_TRUE(str_to_num("127", 10, r8));
    ASSERT_EQ(r8, std::numeric_limits<int8_t>::max());

    // 16 bit
    int16_t r16;
    ASSERT_TRUE(str_to_num("-32768", 10, r16));
    ASSERT_EQ(r16, std::numeric_limits<int16_t>::min());
    ASSERT_TRUE(str_to_num("32767", 10, r16));
    ASSERT_EQ(r16, std::numeric_limits<int16_t>::max());

    // 32 bit
    int32_t r32;
    ASSERT_TRUE(str_to_num("-2147483648", 10, r32));
    ASSERT_EQ(r32, std::numeric_limits<int32_t>::min());
    ASSERT_TRUE(str_to_num("2147483647", 10, r32));
    ASSERT_EQ(r32, std::numeric_limits<int32_t>::max());

    // 64 bit
    int64_t r64;
    ASSERT_TRUE(str_to_num("-9223372036854775808", 10, r64));
    ASSERT_EQ(r64, std::numeric_limits<int64_t>::min());
    ASSERT_TRUE(str_to_num("9223372036854775807", 10, r64));
    ASSERT_EQ(r64, std::numeric_limits<int64_t>::max());
}

TEST(IntegerTest, ConvertUnsignedIntegerLimits)
{
    // 8 bit
    uint8_t r8;
    ASSERT_TRUE(str_to_num("0", 10, r8));
    ASSERT_EQ(r8, std::numeric_limits<uint8_t>::min());
    ASSERT_TRUE(str_to_num("255", 10, r8));
    ASSERT_EQ(r8, std::numeric_limits<uint8_t>::max());

    // 16 bit
    uint16_t r16;
    ASSERT_TRUE(str_to_num("0", 10, r16));
    ASSERT_EQ(r16, std::numeric_limits<uint16_t>::min());
    ASSERT_TRUE(str_to_num("65535", 10, r16));
    ASSERT_EQ(r16, std::numeric_limits<uint16_t>::max());

    // 32 bit
    uint32_t r32;
    ASSERT_TRUE(str_to_num("0", 10, r32));
    ASSERT_EQ(r32, std::numeric_limits<uint32_t>::min());
    ASSERT_TRUE(str_to_num("4294967295", 10, r32));
    ASSERT_EQ(r32, std::numeric_limits<uint32_t>::max());

    // 64 bit
    uint64_t r64;
    ASSERT_TRUE(str_to_num("0", 10, r64));
    ASSERT_EQ(r64, std::numeric_limits<uint64_t>::min());
    ASSERT_TRUE(str_to_num("18446744073709551615", 10, r64));
    ASSERT_EQ(r64, std::numeric_limits<uint64_t>::max());
}

TEST(IntegerTest, ConvertSignedIntegerOutOfRange)
{
    // 8 bit
    int8_t r8;
    ASSERT_FALSE(str_to_num("-129", 10, r8));
    ASSERT_EQ(errno, ERANGE);
    ASSERT_FALSE(str_to_num("128", 10, r8));
    ASSERT_EQ(errno, ERANGE);

    // 16 bit
    int16_t r16;
    ASSERT_FALSE(str_to_num("-32769", 10, r16));
    ASSERT_EQ(errno, ERANGE);
    ASSERT_FALSE(str_to_num("32768", 10, r16));
    ASSERT_EQ(errno, ERANGE);

    // 32 bit
    int32_t r32;
    ASSERT_FALSE(str_to_num("-2147483649", 10, r32));
    ASSERT_EQ(errno, ERANGE);
    ASSERT_FALSE(str_to_num("2147483648", 10, r32));
    ASSERT_EQ(errno, ERANGE);

    // 64 bit
    int64_t r64;
    ASSERT_FALSE(str_to_num("-9223372036854775809", 10, r64));
    ASSERT_EQ(errno, ERANGE);
    ASSERT_FALSE(str_to_num("9223372036854775808", 10, r64));
    ASSERT_EQ(errno, ERANGE);
}

TEST(IntegerTest, ConvertUnsignedIntegerOutOfRange)
{
    // 8 bit
    uint8_t r8;
    ASSERT_FALSE(str_to_num("-1", 10, r8));
    ASSERT_EQ(errno, ERANGE);
    ASSERT_FALSE(str_to_num("256", 10, r8));
    ASSERT_EQ(errno, ERANGE);

    // 16 bit
    uint16_t r16;
    ASSERT_FALSE(str_to_num("-1", 10, r16));
    ASSERT_EQ(errno, ERANGE);
    ASSERT_FALSE(str_to_num("65536", 10, r16));
    ASSERT_EQ(errno, ERANGE);

    // 32 bit
    uint32_t r32;
    ASSERT_FALSE(str_to_num("-1", 10, r32));
    ASSERT_EQ(errno, ERANGE);
    ASSERT_FALSE(str_to_num("4294967296", 10, r32));
    ASSERT_EQ(errno, ERANGE);

    // 64 bit
    uint64_t r64;
    ASSERT_FALSE(str_to_num("-1", 10, r64));
    ASSERT_EQ(errno, ERANGE);
    ASSERT_FALSE(str_to_num("18446744073709551616", 10, r64));
    ASSERT_EQ(errno, ERANGE);
}

TEST(IntegerTest, ConvertSignedToUnsigned)
{
    // 8 bit
    EXPECT_EQ(make_signed_v(std::numeric_limits<uint8_t>::max()), -1);
    EXPECT_EQ(make_signed_v(std::numeric_limits<uint8_t>::min()), 0);
    EXPECT_EQ(make_signed_v(
            static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() / 2)),
            std::numeric_limits<int8_t>::max());
    EXPECT_EQ(make_signed_v(
            static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() / 2 + 1)),
            std::numeric_limits<int8_t>::min());

    // 16 bit
    EXPECT_EQ(make_signed_v(std::numeric_limits<uint16_t>::max()), -1);
    EXPECT_EQ(make_signed_v(std::numeric_limits<uint16_t>::min()), 0);
    EXPECT_EQ(make_signed_v(
            static_cast<uint16_t>(std::numeric_limits<uint16_t>::max() / 2)),
            std::numeric_limits<int16_t>::max());
    EXPECT_EQ(make_signed_v(
            static_cast<uint16_t>(std::numeric_limits<uint16_t>::max() / 2 + 1)),
            std::numeric_limits<int16_t>::min());

    // 32 bit
    EXPECT_EQ(make_signed_v(std::numeric_limits<uint32_t>::max()), -1);
    EXPECT_EQ(make_signed_v(std::numeric_limits<uint32_t>::min()), 0);
    EXPECT_EQ(make_signed_v(
            static_cast<uint32_t>(std::numeric_limits<uint32_t>::max() / 2)),
            std::numeric_limits<int32_t>::max());
    EXPECT_EQ(make_signed_v(
            static_cast<uint32_t>(std::numeric_limits<uint32_t>::max() / 2 + 1)),
            std::numeric_limits<int32_t>::min());

    // 64 bit
    EXPECT_EQ(make_signed_v(std::numeric_limits<uint64_t>::max()), -1);
    EXPECT_EQ(make_signed_v(std::numeric_limits<uint64_t>::min()), 0);
    EXPECT_EQ(make_signed_v(
            static_cast<uint64_t>(std::numeric_limits<uint64_t>::max() / 2)),
            std::numeric_limits<int64_t>::max());
    EXPECT_EQ(make_signed_v(
            static_cast<uint64_t>(std::numeric_limits<uint64_t>::max() / 2 + 1)),
            std::numeric_limits<int64_t>::min());
}

TEST(IntegerTest, ConvertUnsignedToSigned)
{
    // 8 bit
    EXPECT_EQ(make_unsigned_v(std::numeric_limits<int8_t>::max()),
              static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() / 2));
    EXPECT_EQ(make_unsigned_v(std::numeric_limits<int8_t>::min()),
              static_cast<uint8_t>(std::numeric_limits<uint8_t>::max() / 2 + 1));
    EXPECT_EQ(make_unsigned_v(static_cast<int8_t>(-1)),
              std::numeric_limits<uint8_t>::max());
    EXPECT_EQ(make_unsigned_v(static_cast<int8_t>(0)),
              std::numeric_limits<uint8_t>::min());

    // 16 bit
    EXPECT_EQ(make_unsigned_v(std::numeric_limits<int16_t>::max()),
              static_cast<uint16_t>(std::numeric_limits<uint16_t>::max() / 2));
    EXPECT_EQ(make_unsigned_v(std::numeric_limits<int16_t>::min()),
              static_cast<uint16_t>(std::numeric_limits<uint16_t>::max() / 2 + 1));
    EXPECT_EQ(make_unsigned_v(static_cast<int16_t>(-1)),
              std::numeric_limits<uint16_t>::max());
    EXPECT_EQ(make_unsigned_v(static_cast<int16_t>(0)),
              std::numeric_limits<uint16_t>::min());

    // 32 bit
    EXPECT_EQ(make_unsigned_v(std::numeric_limits<int32_t>::max()),
              static_cast<uint32_t>(std::numeric_limits<uint32_t>::max() / 2));
    EXPECT_EQ(make_unsigned_v(std::numeric_limits<int32_t>::min()),
              static_cast<uint32_t>(std::numeric_limits<uint32_t>::max() / 2 + 1));
    EXPECT_EQ(make_unsigned_v(static_cast<int32_t>(-1)),
              std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(make_unsigned_v(static_cast<int32_t>(0)),
              std::numeric_limits<uint32_t>::min());

    // 64 bit
    EXPECT_EQ(make_unsigned_v(std::numeric_limits<int64_t>::max()),
              static_cast<uint64_t>(std::numeric_limits<uint64_t>::max() / 2));
    EXPECT_EQ(make_unsigned_v(std::numeric_limits<int64_t>::min()),
              static_cast<uint64_t>(std::numeric_limits<uint64_t>::max() / 2 + 1));
    EXPECT_EQ(make_unsigned_v(static_cast<int64_t>(-1)),
              std::numeric_limits<uint64_t>::max());
    EXPECT_EQ(make_unsigned_v(static_cast<int64_t>(0)),
              std::numeric_limits<uint64_t>::min());
}
