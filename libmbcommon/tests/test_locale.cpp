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
    const char *mbs_str = "Hello, world!";
    const wchar_t *wcs_str = L"Hello, world!";
    wchar_t *result;

    result = mb::mbs_to_wcs(mbs_str);
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(wcscmp(result, wcs_str), 0);
    free(result);
}

TEST(LocaleTest, ConvertWcsToMbs)
{
    const char *mbs_str = "Hello, world!";
    const wchar_t *wcs_str = L"Hello, world!";
    char *result;

    result = mb::wcs_to_mbs(wcs_str);
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, mbs_str);
    free(result);
}

TEST(LocaleTest, ConvertUtf8ToWcs)
{
    const char *utf8_str = "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8\x96"
                           "\xe7\x95\x8c\xef\xbc\x81";
    const wchar_t *wcs_str = L"你好，世界！";
    wchar_t *result;

    result = mb::utf8_to_wcs(utf8_str);
    ASSERT_NE(result, nullptr);
    ASSERT_EQ(wcscmp(result, wcs_str), 0);
    free(result);
}

TEST(LocaleTest, ConvertWcsToUtf8)
{
    const char *utf8_str = "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8\x96"
                           "\xe7\x95\x8c\xef\xbc\x81";
    const wchar_t *wcs_str = L"你好，世界！";
    char *result;

    result = mb::wcs_to_utf8(wcs_str);
    ASSERT_NE(result, nullptr);
    ASSERT_STREQ(result, utf8_str);
    free(result);
}

TEST(LocaleTest, ConvertEmptyString)
{
    char *mbs_result;
    wchar_t *wcs_result;

    wcs_result = mb::mbs_to_wcs("");
    ASSERT_NE(wcs_result, nullptr);
    ASSERT_EQ(wcscmp(wcs_result, L""), 0);
    free(wcs_result);

    mbs_result = mb::wcs_to_mbs(L"");
    ASSERT_NE(mbs_result, nullptr);
    ASSERT_STREQ(mbs_result, "");
    free(mbs_result);

    wcs_result = mb::utf8_to_wcs("");
    ASSERT_NE(wcs_result, nullptr);
    ASSERT_EQ(wcscmp(wcs_result, L""), 0);
    free(wcs_result);

    mbs_result = mb::wcs_to_utf8(L"");
    ASSERT_NE(mbs_result, nullptr);
    ASSERT_STREQ(mbs_result, "");
    free(mbs_result);
}

TEST(LocaleTest, ConvertPartialString)
{
    const char *utf8_str = "\xe4\xbd\xa0\xe5\xa5\xbd\xef\xbc\x8c\xe4\xb8\x96"
                           "\xe7\x95\x8c\xef\xbc\x81";
    const char *utf8_str_short = "\xe4\xbd\xa0\xe5\xa5\xbd";
    const wchar_t *wcs_str = L"你好，世界！";
    const wchar_t *wcs_str_short = L"你好";

    char *mbs_result;
    wchar_t *wcs_result;

    wcs_result = mb::mbs_to_wcs_len("Hello, world!", 5);
    ASSERT_NE(wcs_result, nullptr);
    ASSERT_EQ(wcscmp(wcs_result, L"Hello"), 0);
    free(wcs_result);

    mbs_result = mb::wcs_to_mbs_len(L"Hello, world!", 5);
    ASSERT_NE(mbs_result, nullptr);
    ASSERT_STREQ(mbs_result, "Hello");
    free(mbs_result);

    wcs_result = mb::utf8_to_wcs_len(utf8_str, strlen(utf8_str_short));
    ASSERT_NE(wcs_result, nullptr);
    ASSERT_EQ(wcscmp(wcs_result, wcs_str_short), 0);
    free(wcs_result);

    mbs_result = mb::wcs_to_utf8_len(wcs_str, wcslen(wcs_str_short));
    ASSERT_NE(mbs_result, nullptr);
    ASSERT_STREQ(mbs_result, utf8_str_short);
    free(mbs_result);
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

    char *mbs_result;
    wchar_t *wcs_result;

    wcs_result = mb::mbs_to_wcs(str1_mbs.c_str());
    ASSERT_NE(wcs_result, nullptr);
    ASSERT_EQ(wcs_result, str1_wcs);
    free(wcs_result);

    mbs_result = mb::wcs_to_mbs(str1_wcs.c_str());
    ASSERT_NE(mbs_result, nullptr);
    ASSERT_EQ(mbs_result, str1_mbs);
    free(mbs_result);

    wcs_result = mb::utf8_to_wcs(str2_utf8.c_str());
    ASSERT_NE(wcs_result, nullptr);
    ASSERT_EQ(wcs_result, str2_wcs);
    free(wcs_result);

    mbs_result = mb::wcs_to_utf8(str2_wcs.c_str());
    ASSERT_NE(mbs_result, nullptr);
    ASSERT_EQ(mbs_result, str2_utf8);
    free(mbs_result);
}

TEST(LocaleTest, ConvertInvalidUtf8String)
{
    const char *utf8_str = "\xe4\xbd\xa0\xe5\xa5";

    ASSERT_EQ(mb::utf8_to_wcs(utf8_str), nullptr);
}
