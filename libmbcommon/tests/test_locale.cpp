/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

TEST(LocaleTest, ConvertInvalidUtf8String)
{
    const char *utf8_str = "\xe4\xbd\xa0\xe5\xa5";

    ASSERT_EQ(mb::utf8_to_wcs(utf8_str), nullptr);
}

int main(int argc, char *argv[])
{
    setlocale(LC_ALL, "");

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
