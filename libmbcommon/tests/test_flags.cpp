/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/flags.h"

enum class TestFlag : uint64_t
{
    A = 1ull << 0,
    B = 1ull << 1,
    C = 1ull << 2,
    D = 1ull << 10,
    E = 1ull << 20,
    F = 1ull << 21,
    G = 1ull << 63,
};
MB_DECLARE_FLAGS(TestFlags, TestFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(TestFlags)

using TestFlagType = std::underlying_type_t<TestFlag>;

#define RAW(x) static_cast<TestFlagType>(x)

constexpr TestFlags ALL_FLAGS =
        TestFlag::A
        | TestFlag::B
        | TestFlag::C
        | TestFlag::D
        | TestFlag::E
        | TestFlag::F
        | TestFlag::G;

template<TestFlagType N>
bool equals_constexpr(TestFlagType n)
{
    return N == n;
}

TEST(FlagsTest, CheckZeroFlagConstructor)
{
    TestFlags flags;
    ASSERT_EQ(RAW(flags), RAW(0));
}

TEST(FlagsTest, CheckSingleFlagConstructor)
{
    TestFlags flags(TestFlag::A);
    ASSERT_EQ(RAW(flags), RAW(TestFlag::A));
}

TEST(FlagsTest, CheckTestFlag)
{
    auto flags = TestFlag::A | TestFlag::B;
    ASSERT_TRUE(flags.test_flag(TestFlag::A));
    ASSERT_TRUE(flags.test_flag(TestFlag::B));
    ASSERT_FALSE(flags.test_flag(TestFlag::C));
}

TEST(FlagsTest, CheckSetFlag)
{
    TestFlags flags(TestFlag::A);
    flags.set_flag(TestFlag::B, true);
    ASSERT_EQ(RAW(flags), RAW(TestFlag::A) | RAW(TestFlag::B));
    flags.set_flag(TestFlag::A, false);
    ASSERT_EQ(RAW(flags), RAW(TestFlag::B));
    flags.set_flag(TestFlag::B, false);
    ASSERT_EQ(RAW(flags), RAW(0));
    flags.set_flag(TestFlag::C, true);
    ASSERT_EQ(RAW(flags), RAW(TestFlag::C));
}

TEST(FlagsTest, CheckNotOperator)
{
    TestFlags flags;
    ASSERT_FALSE(flags);
    ASSERT_TRUE(!flags);
    flags |= TestFlag::A;
    ASSERT_TRUE(flags);
    ASSERT_FALSE(!flags);
}

TEST(FlagsTest, CheckArithmeticAssignmentOperators)
{
    TestFlags flags;
    flags |= TestFlag::A;
    ASSERT_EQ(RAW(flags), RAW(TestFlag::A));
    flags |= TestFlags(TestFlag::B);
    ASSERT_EQ(RAW(flags), RAW(TestFlag::A) | RAW(TestFlag::B));
    flags ^= TestFlag::B;
    ASSERT_EQ(RAW(flags), RAW(TestFlag::A));
    flags ^= TestFlags(TestFlag::C);
    ASSERT_EQ(RAW(flags), RAW(TestFlag::A) | RAW(TestFlag::C));
    flags &= TestFlag::A;
    ASSERT_EQ(RAW(flags), RAW(TestFlag::A));
    flags &= TestFlags(TestFlag::A);
    ASSERT_EQ(RAW(flags), RAW(TestFlag::A));
}

TEST(FlagsTest, CheckArithmeticNonAssignmentOperators)
{
    ASSERT_EQ(RAW(TestFlags(TestFlag::A) | TestFlag::B),
              RAW(TestFlag::A) | RAW(TestFlag::B));
    ASSERT_EQ(RAW(TestFlags(TestFlag::A) | TestFlags(TestFlag::B)),
              RAW(TestFlag::A) | RAW(TestFlag::B));
    ASSERT_EQ(RAW(TestFlags(TestFlag::A) ^ TestFlag::A),
              RAW(0));
    ASSERT_EQ(RAW(TestFlags(TestFlag::A) ^ TestFlags(TestFlag::B)),
              RAW(TestFlag::A) | RAW(TestFlag::B));
    ASSERT_EQ(RAW(TestFlags(TestFlag::A) & TestFlag::A),
              RAW(TestFlag::A));
    ASSERT_EQ(RAW(TestFlags(TestFlag::A) & TestFlags(TestFlag::B)),
              RAW(0));
    ASSERT_EQ(RAW(~TestFlags() ^ TestFlags()),
              ~RAW(0));
}

TEST(FlagsTest, CheckConstExprOperations)
{
    ASSERT_TRUE(equals_constexpr<TestFlag::A | TestFlag::B>(
            RAW(TestFlag::A | TestFlag::B)));
    ASSERT_TRUE(equals_constexpr<TestFlag::A & TestFlag::B>(
            RAW(0)));
    ASSERT_TRUE(equals_constexpr<TestFlag::A & ~TestFlags(TestFlag::B)>(
            RAW(TestFlag::A)));
    ASSERT_TRUE(equals_constexpr<(TestFlag::A | TestFlag::B) ^ TestFlag::B>(
            RAW(TestFlag::A)));
}

TEST(FlagsTest, IterateNormalFlags)
{
    auto it = ALL_FLAGS.begin();
    ASSERT_EQ(*it++, TestFlag::A);
    ASSERT_EQ(*it++, TestFlag::B);
    ASSERT_EQ(*it++, TestFlag::C);
    ASSERT_EQ(*it++, TestFlag::D);
    ASSERT_EQ(*it++, TestFlag::E);
    ASSERT_EQ(*it++, TestFlag::F);
    ASSERT_EQ(*it++, TestFlag::G);
    ASSERT_EQ(it, ALL_FLAGS.end());
}

TEST(FlagsTest, IterateEmptyFlags)
{
    TestFlags flags;
    ASSERT_EQ(flags.begin(), flags.end());
}

TEST(FlagsTest, CheckIteratorEquality)
{
    auto it1 = ALL_FLAGS.begin();
    auto it2 = ALL_FLAGS.begin();

    ++it1;
    ASSERT_NE(it1, it2);
    ++it2;
    ASSERT_EQ(it1, it2);
}

TEST(FlagsTest, CheckConstExprIterator)
{
    ASSERT_TRUE(equals_constexpr<TestFlags(*ALL_FLAGS.begin())>(
            RAW(TestFlag::A)));
}
