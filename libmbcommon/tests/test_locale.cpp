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

#include "mbcommon/locale.h"

TEST(LocaleTest, ConvertMbsToWcs)
{
    constexpr char mbs_str[] = "Hello, world!";
    constexpr wchar_t wcs_str[] = L"Hello, world!";
    std::wstring result;

    ASSERT_TRUE(mb::mbs_to_wcs(result, mbs_str));
    ASSERT_EQ(result, wcs_str);
}

TEST(LocaleTest, ConvertWcsToMbs)
{
    constexpr char mbs_str[] = "Hello, world!";
    constexpr wchar_t wcs_str[] = L"Hello, world!";
    std::string result;

    ASSERT_TRUE(mb::wcs_to_mbs(result, wcs_str));
    ASSERT_EQ(result, mbs_str);
}

TEST(LocaleTest, ConvertUtf8ToWcs)
{
    constexpr char utf8_str[] = "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8"
                                "\x96\xe7\x95\x8c\xef\xbc\x81";
    constexpr wchar_t wcs_str[] = L"你好，世界！";
    std::wstring result;

    ASSERT_TRUE(mb::utf8_to_wcs(result, utf8_str));
    ASSERT_EQ(result, wcs_str);
}

TEST(LocaleTest, ConvertWcsToUtf8)
{
    constexpr char utf8_str[] = "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8"
                                "\x96\xe7\x95\x8c\xef\xbc\x81";
    constexpr wchar_t wcs_str[] = L"你好，世界！";
    std::string result;

    ASSERT_TRUE(mb::wcs_to_utf8(result, wcs_str));
    ASSERT_EQ(result, utf8_str);
}

TEST(LocaleTest, ConvertEmptyString)
{
    std::string mbs_result;
    std::wstring wcs_result;

    wcs_result = L"dummy";
    ASSERT_TRUE(mb::mbs_to_wcs(wcs_result, ""));
    ASSERT_EQ(wcs_result, L"");

    mbs_result = "dummy";
    ASSERT_TRUE(mb::wcs_to_mbs(mbs_result, L""));
    ASSERT_EQ(mbs_result, "");

    wcs_result = L"dummy";
    ASSERT_TRUE(mb::utf8_to_wcs(wcs_result, ""));
    ASSERT_EQ(wcs_result, L"");

    mbs_result = "dummy";
    ASSERT_TRUE(mb::wcs_to_utf8(mbs_result, L""));
    ASSERT_EQ(mbs_result, "");
}

TEST(LocaleTest, ConvertPartialString)
{
    constexpr char utf8_str[] = "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8"
                                "\x96\xe7\x95\x8c\xef\xbc\x81";
    constexpr char utf8_str_short[] = "\xe4\xbd\xa0\xe5\xa5\xbd";
    constexpr wchar_t wcs_str[] = L"你好，世界！";
    constexpr wchar_t wcs_str_short[] = L"你好";

    std::string mbs_result;
    std::wstring wcs_result;

    ASSERT_TRUE(mb::mbs_to_wcs_n(wcs_result, "Hello, world!", 5));
    ASSERT_EQ(wcs_result, L"Hello");

    ASSERT_TRUE(mb::wcs_to_mbs_n(mbs_result, L"Hello, world!", 5));
    ASSERT_EQ(mbs_result, "Hello");

    ASSERT_TRUE(mb::utf8_to_wcs_n(wcs_result, utf8_str, strlen(utf8_str_short)));
    ASSERT_EQ(wcs_result, wcs_str_short);

    ASSERT_TRUE(mb::wcs_to_utf8_n(mbs_result, wcs_str, wcslen(wcs_str_short)));
    ASSERT_EQ(mbs_result, utf8_str_short);
}

TEST(LocaleTest, ConvertLargeString)
{
    std::string str1_mbs;
    std::wstring str1_wcs;
    std::string str2_utf8;
    std::wstring str2_wcs;

    std::string str1_mbs_pattern("Hello!");
    std::wstring str1_wcs_pattern(L"Hello!");
    std::string str2_utf8_pattern("\xe4\xbd\xa0\xe5\xa5\xbd");
    std::wstring str2_wcs_pattern(L"你好");

    str1_mbs.reserve(str1_mbs_pattern.size() * 1024 * 1024 + 1);
    str1_wcs.reserve(str1_wcs_pattern.size() * 1024 * 1024 + 1);
    str2_utf8.reserve(str2_utf8_pattern.size() * 1024 * 1024 * 1);
    str2_wcs.reserve(str2_wcs_pattern.size() * 1024 * 1024 + 1);

    for (int i = 0; i < 1024 * 1024; ++i) {
        str1_mbs += str1_mbs_pattern;
        str1_wcs += str1_wcs_pattern;
        str2_utf8 += str2_utf8_pattern;
        str2_wcs += str2_wcs_pattern;
    }

    std::string mbs_result;
    std::wstring wcs_result;

    ASSERT_TRUE(mb::mbs_to_wcs(wcs_result, str1_mbs));
    ASSERT_EQ(wcs_result, str1_wcs);

    ASSERT_TRUE(mb::wcs_to_mbs(mbs_result, str1_wcs));
    ASSERT_EQ(mbs_result, str1_mbs);

    ASSERT_TRUE(mb::utf8_to_wcs(wcs_result, str2_utf8));
    ASSERT_EQ(wcs_result, str2_wcs);

    ASSERT_TRUE(mb::wcs_to_utf8(mbs_result, str2_wcs));
    ASSERT_EQ(mbs_result, str2_utf8);
}

TEST(LocaleTest, ConvertInvalidUtf8String)
{
    constexpr char utf8_str[] = "\xe4\xbd\xa0\xe5\xa5";
    std::wstring result;

    ASSERT_FALSE(mb::utf8_to_wcs(result, utf8_str));
}
