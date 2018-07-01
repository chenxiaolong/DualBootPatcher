/*
 * Copyright (C) 2016-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
    auto result = mb::mbs_to_wcs(mbs_str);

    ASSERT_TRUE(result);
    ASSERT_EQ(result.value(), wcs_str);
}

TEST(LocaleTest, ConvertWcsToMbs)
{
    constexpr char mbs_str[] = "Hello, world!";
    constexpr wchar_t wcs_str[] = L"Hello, world!";
    auto result = mb::wcs_to_mbs(wcs_str);

    ASSERT_TRUE(result);
    ASSERT_EQ(result.value(), mbs_str);
}

TEST(LocaleTest, ConvertUtf8ToWcs)
{
    constexpr char utf8_str[] = "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8"
                                "\x96\xe7\x95\x8c\xef\xbc\x81";
    constexpr wchar_t wcs_str[] = L"你好，世界！";
    auto result = mb::utf8_to_wcs(utf8_str);

    ASSERT_TRUE(result);
    ASSERT_EQ(result.value(), wcs_str);
}

TEST(LocaleTest, ConvertWcsToUtf8)
{
    constexpr char utf8_str[] = "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8"
                                "\x96\xe7\x95\x8c\xef\xbc\x81";
    constexpr wchar_t wcs_str[] = L"你好，世界！";
    auto result = mb::wcs_to_utf8(wcs_str);

    ASSERT_TRUE(result);
    ASSERT_EQ(result.value(), utf8_str);
}

TEST(LocaleTest, ConvertEmptyString)
{
    auto wcs_result = mb::mbs_to_wcs("");
    ASSERT_TRUE(wcs_result);
    ASSERT_EQ(wcs_result.value(), L"");

    auto mbs_result = mb::wcs_to_mbs(L"");
    ASSERT_TRUE(mbs_result);
    ASSERT_EQ(mbs_result.value(), "");

    wcs_result = mb::utf8_to_wcs("");
    ASSERT_TRUE(wcs_result);
    ASSERT_EQ(wcs_result.value(), L"");

    mbs_result = mb::wcs_to_utf8(L"");
    ASSERT_TRUE(mbs_result);
    ASSERT_EQ(mbs_result.value(), "");
}

TEST(LocaleTest, ConvertLargeString)
{
    std::string str1_mbs;
    std::wstring str1_wcs;
    std::string str2_utf8;
    std::wstring str2_wcs;

    std::string_view str1_mbs_pattern("Hello!");
    std::wstring_view str1_wcs_pattern(L"Hello!");
    std::string_view str2_utf8_pattern("\xe4\xbd\xa0\xe5\xa5\xbd");
    std::wstring_view str2_wcs_pattern(L"你好");

    str1_mbs.reserve(str1_mbs_pattern.size() * 1024 * 1024);
    str1_wcs.reserve(str1_wcs_pattern.size() * 1024 * 1024);
    str2_utf8.reserve(str2_utf8_pattern.size() * 1024 * 1024);
    str2_wcs.reserve(str2_wcs_pattern.size() * 1024 * 1024);

    for (int i = 0; i < 1024 * 1024; ++i) {
        str1_mbs += str1_mbs_pattern;
        str1_wcs += str1_wcs_pattern;
        str2_utf8 += str2_utf8_pattern;
        str2_wcs += str2_wcs_pattern;
    }

    auto wcs_result = mb::mbs_to_wcs(str1_mbs);
    ASSERT_TRUE(wcs_result);
    ASSERT_EQ(wcs_result.value(), str1_wcs);

    auto mbs_result = mb::wcs_to_mbs(str1_wcs);
    ASSERT_TRUE(mbs_result);
    ASSERT_EQ(mbs_result.value(), str1_mbs);

    wcs_result = mb::utf8_to_wcs(str2_utf8);
    ASSERT_TRUE(wcs_result);
    ASSERT_EQ(wcs_result.value(), str2_wcs);

    mbs_result = mb::wcs_to_utf8(str2_wcs);
    ASSERT_TRUE(mbs_result);
    ASSERT_EQ(mbs_result.value(), str2_utf8);
}

TEST(LocaleTest, ConvertInvalidUtf8String)
{
    constexpr char utf8_str[] = "\xe4\xbd\xa0\xe5\xa5";
    auto result = mb::utf8_to_wcs(utf8_str);

    ASSERT_FALSE(result);
}
