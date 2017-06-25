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
    ASSERT_TRUE(mb_starts_with("Hello, World!", "Hello, World!"));
    ASSERT_TRUE(mb_starts_with_icase("Hello, World!", "HELLO, WORLD!"));

    // Check matching prefix
    ASSERT_TRUE(mb_starts_with("Hello, World!", "Hello"));
    ASSERT_TRUE(mb_starts_with_icase("Hello, World!", "HELLO"));

    // Check non-matching prefix
    ASSERT_FALSE(mb_starts_with("abcd", "abcde"));
    ASSERT_FALSE(mb_starts_with_icase("abcd", "ABCDE"));
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

    ASSERT_TRUE(mb_starts_with_n(source, source_len,
                                 prefix1, prefix1_len));
    ASSERT_FALSE(mb_starts_with_n(source, source_len,
                                  prefix2, prefix2_len));
    ASSERT_TRUE(mb_starts_with_icase_n(source, source_len,
                                       prefix3, prefix3_len));
    ASSERT_FALSE(mb_starts_with_icase_n(source, source_len,
                                        prefix4, prefix4_len));
}

TEST(StringTest, CheckStartsWithEmpty)
{
    // Empty prefix
    ASSERT_TRUE(mb_starts_with("Hello, World!", ""));
    ASSERT_TRUE(mb_starts_with_icase("Hello, World!", ""));

    // Empty source
    ASSERT_FALSE(mb_starts_with("", "abcde"));
    ASSERT_FALSE(mb_starts_with_icase("", "abcde"));

    // Empty source and prefix
    ASSERT_TRUE(mb_starts_with("", ""));
    ASSERT_TRUE(mb_starts_with_icase("", ""));
}

TEST(StringTest, CheckEndsWithNormal)
{
    // Check equal strings
    ASSERT_TRUE(mb_ends_with("Hello, World!", "Hello, World!"));
    ASSERT_TRUE(mb_ends_with_icase("Hello, World!", "HELLO, WORLD!"));

    // Check matching suffix
    ASSERT_TRUE(mb_ends_with("Hello, World!", "World!"));
    ASSERT_TRUE(mb_ends_with_icase("Hello, World!", "WORLD!"));

    // Check non-matching prefix
    ASSERT_FALSE(mb_ends_with("abcd", "abcde"));
    ASSERT_FALSE(mb_ends_with_icase("abcd", "ABCDE"));
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

    ASSERT_TRUE(mb_ends_with_n(source, source_len,
                               suffix1, suffix1_len));
    ASSERT_FALSE(mb_ends_with_n(source, source_len,
                                suffix2, suffix2_len));
    ASSERT_TRUE(mb_ends_with_icase_n(source, source_len,
                                     suffix3, suffix3_len));
    ASSERT_FALSE(mb_ends_with_icase_n(source, source_len,
                                      suffix4, suffix4_len));
}

TEST(StringTest, CheckEndsWithEmpty)
{
    // Empty suffix
    ASSERT_TRUE(mb_ends_with("Hello, World!", ""));
    ASSERT_TRUE(mb_ends_with_icase("Hello, World!", ""));

    // Empty source
    ASSERT_FALSE(mb_ends_with("", "abcde"));
    ASSERT_FALSE(mb_ends_with_icase("", "abcde"));

    // Empty source and prefix
    ASSERT_TRUE(mb_ends_with("", ""));
    ASSERT_TRUE(mb_ends_with_icase("", ""));
}

TEST(StringTest, InsertMemory)
{
    struct {
        const void *source;
        size_t source_size;
        const void *target;
        size_t target_size;
        size_t pos;
        const void *data;
        size_t data_size;
    } test_cases[] = {
        // Insert in middle
        { "abcd", 4, "abecd", 5, 2, "e", 1 },
        // Insert at beginning
        { "abcd", 4, "eabcd", 5, 0, "e", 1 },
        // Insert at end
        { "abcd", 4, "abcde", 5, 4, "e", 1 },
        // Insert nothing
        { "abcd", 4, "abcd", 4, 0, "", 0 },
        // Insert into nothing
        { "", 0, "a", 1, 0, "a", 1 },
        // Insert nothing into nothing
        { "", 0, "", 0, 0, "", 0 },
        // End
        { nullptr, 0, nullptr, 0, 0, nullptr, 0 },
    };

    for (auto it = test_cases; it->source; ++it) {
        size_t buf_size = it->source_size;
        void *buf = malloc(buf_size);
        ASSERT_NE(buf, nullptr);
        memcpy(buf, it->source, buf_size);

        ASSERT_EQ(mb_mem_insert(&buf, &buf_size, it->pos,
                                it->data, it->data_size), 0);

        // Ensure target string matches
        ASSERT_EQ(buf_size, it->target_size);
        ASSERT_EQ(memcmp(it->target, buf, buf_size), 0);

        free(buf);
    }
}

TEST(StringTest, InsertMemoryOutOfRange)
{
    void *buf = nullptr;
    size_t buf_size = 0;

    ASSERT_LT(mb_mem_insert(&buf, &buf_size, 1, "", 0), 0);
}

TEST(StringTest, InsertString)
{
    struct {
        const char *source;
        const char *target;
        size_t pos;
        const char *data;
    } test_cases[] = {
        // Insert in middle
        { "abcd", "abecd", 2, "e" },
        // Insert at beginning
        { "abcd", "eabcd", 0, "e" },
        // Insert at end
        { "abcd", "abcde", 4, "e" },
        // Insert nothing
        { "abcd", "abcd", 0, "" },
        // Insert into nothing
        { "", "a", 0, "a" },
        // Insert nothing into nothing
        { "", "", 0, "" },
        // End
        { nullptr, nullptr, 0, nullptr },
    };

    for (auto it = test_cases; it->source; ++it) {
        char *buf = strdup(it->source);
        ASSERT_NE(buf, nullptr);

        ASSERT_EQ(mb_str_insert(&buf, it->pos, it->data), 0);

        // Ensure target string matches
        ASSERT_STREQ(buf, it->target);

        free(buf);
    }
}

TEST(StringTest, ReplaceMemory)
{
    struct {
        const void *source;
        size_t source_size;
        const void *target;
        size_t target_size;
        const void *from;
        size_t from_size;
        const void *to;
        size_t to_size;
        size_t n;
        size_t n_expected;
    } test_cases[] = {
        // Match not at beginning
        { "abcd", 4, "aefg", 4, "bcd", 3, "efg", 3, 0, 1 },
        // Match not at end
        { "abcd", 4, "efgd", 4, "abc", 3, "efg", 3, 0, 1 },
        // Full string replacement
        { "abc", 3, "def", 3, "abc", 3, "def", 3, 0, 1 },
        // Multiple instances
        { "abcabcabc", 9, "defdefdef", 9, "abc", 3, "def", 3, 0, 3 },
        // Multiple instances with limited replacements
        { "abcabcabc", 9, "defdefabc", 9, "abc", 3, "def", 3, 2, 2 },
        // No matches
        { "abcabcabc", 9, "abcabcabc", 9, "def", 3, "ghi", 3, 0, 0 },
        // Empty data
        { "", 0, "", 0, "abc", 3, "def", 3, 0, 0 },
        // Empty source
        { "abc", 3, "abc", 3, "", 0, "def", 3, 0, 0 },
        // Empty target
        { "abc", 3, "", 0, "abc", 3, "", 0, 0, 1 },
        // Empty everything
        { "", 0, "", 0, "", 0, "", 0, 0, 0 },
        // End
        { nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, 0, 0 },
    };

    for (auto it = test_cases; it->source; ++it) {
        size_t buf_size = it->source_size;
        void *buf = malloc(buf_size);
        ASSERT_NE(buf, nullptr);
        memcpy(buf, it->source, buf_size);

        size_t matches;
        ASSERT_EQ(mb_mem_replace(&buf, &buf_size, it->from, it->from_size,
                                 it->to, it->to_size, it->n, &matches), 0);

        // Ensure target string matches
        ASSERT_EQ(buf_size, it->target_size);
        ASSERT_EQ(memcmp(it->target, buf, buf_size), 0);

        // Ensure number of replacements matches
        ASSERT_EQ(matches, it->n_expected);

        free(buf);
    }
}

TEST(StringTest, ReplaceString)
{
    struct {
        const char *source;
        const char *target;
        const char *from;
        const char *to;
        size_t n;
        size_t n_expected;
    } test_cases[] = {
        // Match not at beginning
        { "abcd", "aefg", "bcd", "efg", 0, 1 },
        // Match not at end
        { "abcd", "efgd", "abc", "efg", 0, 1 },
        // Full string replacement
        { "abc", "def", "abc", "def", 0, 1 },
        // Multiple instances
        { "abcabcabc", "defdefdef", "abc", "def", 0, 3 },
        // Multiple instances with limited replacements
        { "abcabcabc", "defdefabc", "abc", "def", 2, 2 },
        // No matches
        { "abcabcabc", "abcabcabc", "def", "ghi", 0, 0 },
        // Empty data
        { "", "", "abc", "def", 0, 0 },
        // Empty source
        { "abc", "abc", "", "def", 0, 0 },
        // Empty target
        { "abc", "", "abc", "", 0, 1 },
        // Empty everything
        { "", "", "", "", 0, 0 },
        // End
        { nullptr, nullptr, nullptr, nullptr, 0, 0 },
    };

    for (auto it = test_cases; it->source; ++it) {
        char *buf = strdup(it->source);
        ASSERT_NE(buf, nullptr);

        size_t matches;
        ASSERT_EQ(mb_str_replace(&buf, it->from, it->to, it->n, &matches), 0);

        // Ensure target string matches
        ASSERT_STREQ(buf, it->target);

        // Ensure number of replacements matches
        ASSERT_EQ(matches, it->n_expected);

        free(buf);
    }
}
