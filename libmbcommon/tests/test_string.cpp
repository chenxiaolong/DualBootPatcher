/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>

#include "mbcommon/string.h"

TEST(StringTest, FormatString)
{
    char *str = mb_format("Hello, %s!", "World");
    ASSERT_NE(str, nullptr);
    ASSERT_STREQ(str, "Hello, World!");
    free(str);
}

TEST(StringTest, FormatEmptyString)
{
    char *str = mb_format("");
    ASSERT_NE(str, nullptr);
    ASSERT_STREQ(str, "");
    free(str);
}

TEST(StringTest, FormatSizeT)
{
    ptrdiff_t signed_val = 0x7FFFFFFF;
    size_t unsigned_val = 0xFFFFFFFFu;

    char *signed_d_str = mb_format("%" MB_PRIzd, signed_val);
    ASSERT_NE(signed_d_str, nullptr);
    ASSERT_STREQ(signed_d_str, "2147483647");
    free(signed_d_str);

    char *signed_i_str = mb_format("%" MB_PRIzi, signed_val);
    ASSERT_NE(signed_i_str, nullptr);
    ASSERT_STREQ(signed_i_str, "2147483647");
    free(signed_i_str);

    char *unsigned_o_str = mb_format("%" MB_PRIzo, unsigned_val);
    ASSERT_NE(unsigned_o_str, nullptr);
    ASSERT_STREQ(unsigned_o_str, "37777777777");
    free(unsigned_o_str);

    char *unsigned_u_str = mb_format("%" MB_PRIzu, unsigned_val);
    ASSERT_NE(unsigned_u_str, nullptr);
    ASSERT_STREQ(unsigned_u_str, "4294967295");
    free(unsigned_u_str);

    char *unsigned_x_str = mb_format("%" MB_PRIzx, unsigned_val);
    ASSERT_NE(unsigned_x_str, nullptr);
    ASSERT_STREQ(unsigned_x_str, "ffffffff");
    free(unsigned_x_str);

    char *unsigned_X_str = mb_format("%" MB_PRIzX, unsigned_val);
    ASSERT_NE(unsigned_X_str, nullptr);
    ASSERT_STREQ(unsigned_X_str, "FFFFFFFF");
    free(unsigned_X_str);
}

TEST(StringTest, CheckStartsWithNormal)
{
    // Check equal strings
    ASSERT_TRUE(mb_starts_with("Hello, World!", "Hello, World!", false));
    ASSERT_TRUE(mb_starts_with("Hello, World!", "HELLO, WORLD!", true));
    ASSERT_TRUE(mb_starts_with_w(L"Hello, World!", L"Hello, World!", false));
    ASSERT_TRUE(mb_starts_with_w(L"Hello, World!", L"HELLO, WORLD!", true));

    // Check matching prefix
    ASSERT_TRUE(mb_starts_with("Hello, World!", "Hello", false));
    ASSERT_TRUE(mb_starts_with("Hello, World!", "HELLO", true));
    ASSERT_TRUE(mb_starts_with_w(L"Hello, World!", L"Hello", false));
    ASSERT_TRUE(mb_starts_with_w(L"Hello, World!", L"HELLO", true));

    // Check non-matching prefix
    ASSERT_FALSE(mb_starts_with("abcd", "abcde", false));
    ASSERT_FALSE(mb_starts_with("abcd", "ABCDE", true));
    ASSERT_FALSE(mb_starts_with_w(L"abcd", L"abcde", false));
    ASSERT_FALSE(mb_starts_with_w(L"abcd", L"ABCDE", true));
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

    wchar_t w_source[] = { L'a', L'b', L'c', L'd', L'e' };
    wchar_t w_prefix1[] = { L'a', L'b', L'c', L'd' };
    wchar_t w_prefix2[] = { L'a', L'b', L'c', L'e' };
    wchar_t w_prefix3[] = { L'A', L'B', L'C', L'D' };
    wchar_t w_prefix4[] = { L'A', L'B', L'C', L'E' };

    ASSERT_EQ(sizeof(source), 5);
    ASSERT_EQ(sizeof(prefix1), 4);
    ASSERT_EQ(sizeof(prefix2), 4);
    ASSERT_EQ(sizeof(prefix3), 4);
    ASSERT_EQ(sizeof(prefix4), 4);
    size_t w_source_len = sizeof(w_source) / sizeof(wchar_t);
    size_t w_prefix1_len = sizeof(w_prefix1) / sizeof(wchar_t);
    size_t w_prefix2_len = sizeof(w_prefix2) / sizeof(wchar_t);
    size_t w_prefix3_len = sizeof(w_prefix3) / sizeof(wchar_t);
    size_t w_prefix4_len = sizeof(w_prefix4) / sizeof(wchar_t);

    ASSERT_TRUE(mb_starts_with_n(source, source_len,
                                 prefix1, prefix1_len, false));
    ASSERT_FALSE(mb_starts_with_n(source, source_len,
                                  prefix2, prefix2_len, false));
    ASSERT_TRUE(mb_starts_with_n(source, source_len,
                                 prefix3, prefix3_len, true));
    ASSERT_FALSE(mb_starts_with_n(source, source_len,
                                  prefix4, prefix4_len, true));

    ASSERT_TRUE(mb_starts_with_w_n(w_source, w_source_len,
                                   w_prefix1, w_prefix1_len, false));
    ASSERT_FALSE(mb_starts_with_w_n(w_source, w_source_len,
                                    w_prefix2, w_prefix2_len, false));
    ASSERT_TRUE(mb_starts_with_w_n(w_source, w_source_len,
                                   w_prefix3, w_prefix3_len, true));
    ASSERT_FALSE(mb_starts_with_w_n(w_source, w_source_len,
                                    w_prefix4, w_prefix4_len, true));
}

TEST(StringTest, CheckStartsWithEmpty)
{
    // Empty prefix
    ASSERT_TRUE(mb_starts_with("Hello, World!", "", false));
    ASSERT_TRUE(mb_starts_with("Hello, World!", "", true));
    ASSERT_TRUE(mb_starts_with_w(L"Hello, World!", L"", false));
    ASSERT_TRUE(mb_starts_with_w(L"Hello, World!", L"", true));

    // Empty source
    ASSERT_FALSE(mb_starts_with("", "abcde", false));
    ASSERT_FALSE(mb_starts_with("", "abcde", true));
    ASSERT_FALSE(mb_starts_with_w(L"", L"abcde", false));
    ASSERT_FALSE(mb_starts_with_w(L"", L"abcde", true));

    // Empty source and prefix
    ASSERT_TRUE(mb_starts_with("", "", false));
    ASSERT_TRUE(mb_starts_with("", "", true));
    ASSERT_TRUE(mb_starts_with_w(L"", L"", false));
    ASSERT_TRUE(mb_starts_with_w(L"", L"", true));
}

TEST(StringTest, CheckEndsWithNormal)
{
    // Check equal strings
    ASSERT_TRUE(mb_ends_with("Hello, World!", "Hello, World!", false));
    ASSERT_TRUE(mb_ends_with("Hello, World!", "HELLO, WORLD!", true));
    ASSERT_TRUE(mb_ends_with_w(L"Hello, World!", L"Hello, World!", false));
    ASSERT_TRUE(mb_ends_with_w(L"Hello, World!", L"HELLO, WORLD!", true));

    // Check matching suffix
    ASSERT_TRUE(mb_ends_with("Hello, World!", "World!", false));
    ASSERT_TRUE(mb_ends_with("Hello, World!", "WORLD!", true));
    ASSERT_TRUE(mb_ends_with_w(L"Hello, World!", L"World!", false));
    ASSERT_TRUE(mb_ends_with_w(L"Hello, World!", L"WORLD!", true));

    // Check non-matching prefix
    ASSERT_FALSE(mb_ends_with("abcd", "abcde", false));
    ASSERT_FALSE(mb_ends_with("abcd", "ABCDE", true));
    ASSERT_FALSE(mb_ends_with_w(L"abcd", L"abcde", false));
    ASSERT_FALSE(mb_ends_with_w(L"abcd", L"ABCDE", true));
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

    wchar_t w_source[] = { L'a', L'b', L'c', L'd', L'e' };
    wchar_t w_suffix1[] = { L'b', L'c', L'd', L'e' };
    wchar_t w_suffix2[] = { L'b', L'c', L'd', L'f' };
    wchar_t w_suffix3[] = { L'B', L'C', L'D', L'E' };
    wchar_t w_suffix4[] = { L'B', L'C', L'D', L'F' };
    size_t w_source_len = sizeof(w_source) / sizeof(wchar_t);
    size_t w_suffix1_len = sizeof(w_suffix1) / sizeof(wchar_t);
    size_t w_suffix2_len = sizeof(w_suffix2) / sizeof(wchar_t);
    size_t w_suffix3_len = sizeof(w_suffix3) / sizeof(wchar_t);
    size_t w_suffix4_len = sizeof(w_suffix4) / sizeof(wchar_t);

    ASSERT_TRUE(mb_ends_with_n(source, source_len,
                               suffix1, suffix1_len, false));
    ASSERT_FALSE(mb_ends_with_n(source, source_len,
                                suffix2, suffix2_len, false));
    ASSERT_TRUE(mb_ends_with_n(source, source_len,
                               suffix3, suffix3_len, true));
    ASSERT_FALSE(mb_ends_with_n(source, source_len,
                                suffix4, suffix4_len, true));

    ASSERT_TRUE(mb_ends_with_w_n(w_source, w_source_len,
                                 w_suffix1, w_suffix1_len, false));
    ASSERT_FALSE(mb_ends_with_w_n(w_source, w_source_len,
                                  w_suffix2, w_suffix2_len, false));
    ASSERT_TRUE(mb_ends_with_w_n(w_source, w_source_len,
                                 w_suffix3, w_suffix3_len, true));
    ASSERT_FALSE(mb_ends_with_w_n(w_source, w_source_len,
                                  w_suffix4, w_suffix4_len, true));
}

TEST(StringTest, CheckEndsWithEmpty)
{
    // Empty suffix
    ASSERT_TRUE(mb_ends_with("Hello, World!", "", false));
    ASSERT_TRUE(mb_ends_with("Hello, World!", "", true));
    ASSERT_TRUE(mb_ends_with_w(L"Hello, World!", L"", false));
    ASSERT_TRUE(mb_ends_with_w(L"Hello, World!", L"", true));

    // Empty source
    ASSERT_FALSE(mb_ends_with("", "abcde", false));
    ASSERT_FALSE(mb_ends_with("", "abcde", true));
    ASSERT_FALSE(mb_ends_with_w(L"", L"abcde", false));
    ASSERT_FALSE(mb_ends_with_w(L"", L"abcde", true));

    // Empty source and prefix
    ASSERT_TRUE(mb_ends_with("", "", false));
    ASSERT_TRUE(mb_ends_with("", "", true));
    ASSERT_TRUE(mb_ends_with_w(L"", L"", false));
    ASSERT_TRUE(mb_ends_with_w(L"", L"", true));
}
