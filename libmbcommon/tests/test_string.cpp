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

#include "mbcommon/string.h"

using namespace mb;

TEST(StringTest, FormatString)
{
    ASSERT_EQ(format("Hello, %s!", "World"), "Hello, World!");
}

TEST(StringTest, FormatEmptyString)
{
    ASSERT_EQ(format(""), "");
}

TEST(StringTest, FormatSizeT)
{
    ptrdiff_t signed_val = 0x7FFFFFFF;
    size_t unsigned_val = 0xFFFFFFFFu;

    ASSERT_EQ(format("%" MB_PRIzd, signed_val), "2147483647");
    ASSERT_EQ(format("%" MB_PRIzi, signed_val), "2147483647");
    ASSERT_EQ(format("%" MB_PRIzo, unsigned_val), "37777777777");
    ASSERT_EQ(format("%" MB_PRIzu, unsigned_val), "4294967295");
    ASSERT_EQ(format("%" MB_PRIzx, unsigned_val), "ffffffff");
    ASSERT_EQ(format("%" MB_PRIzX, unsigned_val), "FFFFFFFF");
}

TEST(StringTest, CheckStartsWithNormal)
{
    // Check equal strings
    ASSERT_TRUE(starts_with("Hello, World!", "Hello, World!"));
    ASSERT_TRUE(starts_with_icase("Hello, World!", "HELLO, WORLD!"));

    // Check matching prefix
    ASSERT_TRUE(starts_with("Hello, World!", "Hello"));
    ASSERT_TRUE(starts_with_icase("Hello, World!", "HELLO"));

    // Check non-matching prefix
    ASSERT_FALSE(starts_with("abcd", "abcde"));
    ASSERT_FALSE(starts_with_icase("abcd", "ABCDE"));
}

TEST(StringTest, CheckStartsWithNotNullTerminated)
{
    char source[] = { 'a', 'b', 'c', 'd', 'e' };
    char prefix1[] = { 'a', 'b', 'c', 'd' };
    char prefix2[] = { 'a', 'b', 'c', 'e' };
    char prefix3[] = { 'A', 'B', 'C', 'D' };
    char prefix4[] = { 'A', 'B', 'C', 'E' };
    size_t source_len = sizeof(source) / sizeof(char);
    size_t prefix1_len = sizeof(prefix1) / sizeof(char);
    size_t prefix2_len = sizeof(prefix2) / sizeof(char);
    size_t prefix3_len = sizeof(prefix3) / sizeof(char);
    size_t prefix4_len = sizeof(prefix4) / sizeof(char);

    ASSERT_TRUE(starts_with({source, source_len}, {prefix1, prefix1_len}));
    ASSERT_FALSE(starts_with({source, source_len}, {prefix2, prefix2_len}));
    ASSERT_TRUE(starts_with_icase({source, source_len}, {prefix3, prefix3_len}));
    ASSERT_FALSE(starts_with_icase({source, source_len}, {prefix4, prefix4_len}));
}

TEST(StringTest, CheckStartsWithEmpty)
{
    // Empty prefix
    ASSERT_TRUE(starts_with("Hello, World!", ""));
    ASSERT_TRUE(starts_with_icase("Hello, World!", ""));

    // Empty source
    ASSERT_FALSE(starts_with("", "abcde"));
    ASSERT_FALSE(starts_with_icase("", "abcde"));

    // Empty source and prefix
    ASSERT_TRUE(starts_with("", ""));
    ASSERT_TRUE(starts_with_icase("", ""));
}

TEST(StringTest, CheckEndsWithNormal)
{
    // Check equal strings
    ASSERT_TRUE(ends_with("Hello, World!", "Hello, World!"));
    ASSERT_TRUE(ends_with_icase("Hello, World!", "HELLO, WORLD!"));

    // Check matching suffix
    ASSERT_TRUE(ends_with("Hello, World!", "World!"));
    ASSERT_TRUE(ends_with_icase("Hello, World!", "WORLD!"));

    // Check non-matching prefix
    ASSERT_FALSE(ends_with("abcd", "abcde"));
    ASSERT_FALSE(ends_with_icase("abcd", "ABCDE"));
}

TEST(StringTest, CheckEndsWithNotNullTerminated)
{
    char source[] = { 'a', 'b', 'c', 'd', 'e' };
    char suffix1[] = { 'b', 'c', 'd', 'e' };
    char suffix2[] = { 'b', 'c', 'd', 'f' };
    char suffix3[] = { 'B', 'C', 'D', 'E' };
    char suffix4[] = { 'B', 'C', 'D', 'F' };
    size_t source_len = sizeof(source) / sizeof(char);
    size_t suffix1_len = sizeof(suffix1) / sizeof(char);
    size_t suffix2_len = sizeof(suffix2) / sizeof(char);
    size_t suffix3_len = sizeof(suffix3) / sizeof(char);
    size_t suffix4_len = sizeof(suffix4) / sizeof(char);

    ASSERT_TRUE(ends_with({source, source_len}, {suffix1, suffix1_len}));
    ASSERT_FALSE(ends_with({source, source_len}, {suffix2, suffix2_len}));
    ASSERT_TRUE(ends_with_icase({source, source_len}, {suffix3, suffix3_len}));
    ASSERT_FALSE(ends_with_icase({source, source_len}, {suffix4, suffix4_len}));
}

TEST(StringTest, CheckEndsWithEmpty)
{
    // Empty suffix
    ASSERT_TRUE(ends_with("Hello, World!", ""));
    ASSERT_TRUE(ends_with_icase("Hello, World!", ""));

    // Empty source
    ASSERT_FALSE(ends_with("", "abcde"));
    ASSERT_FALSE(ends_with_icase("", "abcde"));

    // Empty source and prefix
    ASSERT_TRUE(ends_with("", ""));
    ASSERT_TRUE(ends_with_icase("", ""));
}

TEST(StringTest, TrimInPlace)
{
    std::string s;

    trim_left(s);
    ASSERT_EQ(s, "");
    trim_right(s);
    ASSERT_EQ(s, "");
    trim(s);
    ASSERT_EQ(s, "");

    s = " \t abcd \t ";
    trim_left(s);
    ASSERT_EQ(s, "abcd \t ");
    trim_right(s);
    ASSERT_EQ(s, "abcd");

    s = " \f\n\r\t\v";
    trim(s);
    ASSERT_EQ(s, "");

    s = "abcd";
    trim(s);
    ASSERT_EQ(s, "abcd");
}

TEST(StringTest, TrimStringView)
{
    ASSERT_EQ(trimmed_left(""), "");
    ASSERT_EQ(trimmed_right(""), "");
    ASSERT_EQ(trimmed(""), "");

    ASSERT_EQ(trimmed_left(" \t abcd \t "), "abcd \t ");
    ASSERT_EQ(trimmed_right(" \t abcd \t "), " \t abcd");
    ASSERT_EQ(trimmed(" \t abcd \t "), "abcd");

    ASSERT_EQ(trimmed(" \f\n\r\t\v"), "");

    ASSERT_EQ(trimmed("abcd"), "abcd");
}

TEST(StringTest, CheckSplit)
{
    using VS = std::vector<std::string>;

    // Empty delimiter or string
    ASSERT_EQ(split("", ""), VS({""}));
    ASSERT_EQ(split("abc", ""), VS({"abc"}));
    ASSERT_EQ(split("", ':'), VS({""}));

    // Normal split
    ASSERT_EQ(split(":a:b:c:", ':'), VS({"", "a", "b", "c", ""}));

    // Repeated delimiters in source
    ASSERT_EQ(split("a:::b", ':'), VS({"a", "", "", "b"}));

    // Multiple delimiters
    ASSERT_EQ(split("a:b;c,d", ",:;"), VS({"a", "b", "c", "d"}));

    // Multiple repeated delimiters
    ASSERT_EQ(split("a:,:;b", ",:;"), VS({"a", "", "", "", "b"}));
}

TEST(StringTest, CheckSplitStringView)
{
    using VSV = std::vector<std::string_view>;

    // Empty delimiter or string
    ASSERT_EQ(split_sv("", ""), VSV({""}));
    ASSERT_EQ(split_sv("abc", ""), VSV({"abc"}));
    ASSERT_EQ(split_sv("", ':'), VSV({""}));

    // Normal split
    ASSERT_EQ(split_sv(":a:b:c:", ':'), VSV({"", "a", "b", "c", ""}));

    // Repeated delimiters in source
    ASSERT_EQ(split_sv("a:::b", ':'), VSV({"a", "", "", "b"}));

    // Multiple delimiters
    ASSERT_EQ(split_sv("a:b;c,d", ",:;"), VSV({"a", "b", "c", "d"}));

    // Multiple repeated delimiters
    ASSERT_EQ(split_sv("a:,:;b", ",:;"), VSV({"a", "", "", "", "b"}));
}

TEST(StringTest, CheckJoin)
{
    using VS = std::vector<std::string>;
    using VSV = std::vector<std::string_view>;

    // Empty delimiter
    ASSERT_EQ(join(VS(), ""), "");
    ASSERT_EQ(join(VS({"a", "b"}), ""), "ab");

    // Empty list
    ASSERT_EQ(join(VS(), ','), "");

    // One-element list
    ASSERT_EQ(join(VS({"a"}), ','), "a");

    // List of empty strings
    ASSERT_EQ(join(VS({"", "", ""}), ','), ",,");

    // Normal
    ASSERT_EQ(join(VS({"a", "b", "c"}), ','), "a,b,c");
    ASSERT_EQ(join(VSV({"a", "b", "c"}), ','), "a,b,c");

    // String delimiter
    ASSERT_EQ(join(VS({"a", "b", "c"}), ", "), "a, b, c");
    ASSERT_EQ(join(VSV({"a", "b", "c"}), ", "), "a, b, c");
}
