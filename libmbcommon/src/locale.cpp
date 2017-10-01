/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/locale.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#endif

#include "mbcommon/error.h"

namespace mb
{

#ifdef _WIN32

static wchar_t * win32_convert_to_wcs(UINT code_page,
                                      const char *in_buf, size_t in_size)
{
    wchar_t *out_buf = nullptr, *result = nullptr;
    int n;

    // Win32 cannot handle empty strings, so we'll manually handle the trivial
    // case
    if (in_size == 0) {
        return wcsdup(L"");
    }

    // MultiByteToWideChar() only accepts an int
    if (in_size > INT_MAX) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    n = MultiByteToWideChar(code_page, 0, in_buf, static_cast<int>(in_size),
                            nullptr, 0);
    if (n == 0) {
        goto done;
    }

    // n does not include the NULL-terminator if the input did not contain one
    out_buf = static_cast<wchar_t *>(malloc(
            static_cast<size_t>(n + 1) * sizeof(wchar_t)));
    if (!out_buf) {
        goto done;
    }

    n = MultiByteToWideChar(code_page, 0, in_buf, static_cast<int>(in_size),
                            out_buf, n);
    if (n == 0) {
        goto done;
    }

    // Conversion completed successfully. NULL-terminate the buffer.
    out_buf[n] = L'\0';

    result = out_buf;

done:
    ErrorRestorer restorer;

    if (!result) {
        free(out_buf);
    }

    return result;
}

static char * win32_convert_to_mbs(UINT code_page,
                                   const wchar_t *in_buf, size_t in_size)
{
    char *out_buf = nullptr, *result = nullptr;
    int n;

    // Win32 cannot handle empty strings, so we'll manually handle the trivial
    // case
    if (in_size == 0) {
        return strdup("");
    }

    // WideCharToMultiByte() only accepts an int
    if (in_size > INT_MAX) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto done;
    }

    n = WideCharToMultiByte(code_page, 0, in_buf, static_cast<int>(in_size),
                            nullptr, 0, nullptr, nullptr);
    if (n == 0) {
        goto done;
    }

    // n does not include the NULL-terminator if the input did not contain one
    out_buf = static_cast<char *>(malloc(
            static_cast<size_t>(n + 1) * sizeof(char)));
    if (!out_buf) {
        goto done;
    }

    n = WideCharToMultiByte(code_page, 0, in_buf, static_cast<int>(in_size),
                            out_buf, n, nullptr, nullptr);
    if (n == 0) {
        goto done;
    }

    // Conversion completed successfully. NULL-terminate the buffer.
    out_buf[n] = '\0';

    result = out_buf;

done:
    ErrorRestorer restorer;

    if (!result) {
        free(out_buf);
    }

    return result;
}

#else

#define ICONV_CODE_DEFAULT          ""
#define ICONV_CODE_WCHAR_T          "WCHAR_T"
#define ICONV_CODE_UTF_8            "UTF-8"

static char * iconv_convert(const char *from_code, const char *to_code,
                            const char *in_buf, size_t in_size)
{
    iconv_t cd = nullptr;
    char *out_buf = nullptr, *result = nullptr;
    size_t out_size, nconv, unit_size;
    // For manipulation by iconv
    char **in_iconv_buf_ptr, *out_iconv_buf;
    size_t *in_iconv_size_ptr, out_iconv_size;
    bool is_wide_char;

    is_wide_char = strcmp("WCHAR_T", to_code) == 0;

    // Get size of one unit (char or wchar_t). This is needed in order to leave
    // room for one extra unit for NULL-termination
    if (is_wide_char) {
        unit_size = sizeof(wchar_t);
    } else {
        unit_size = sizeof(char);
    }

    cd = iconv_open(to_code, from_code);
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        goto done;
    }

    // Start off with a buffer size equal to the input
    out_size = in_size;
    if (out_size > SIZE_MAX - unit_size) {
        out_size = SIZE_MAX - unit_size;
    }
    out_buf = static_cast<char *>(malloc(out_size + unit_size));
    if (!out_buf) {
        goto done;
    }

    // Set pointer and size for iconv. iconv will modify these during the
    // conversion
    out_iconv_buf = out_buf;
    out_iconv_size = out_size;
    in_iconv_buf_ptr = const_cast<char **>(&in_buf);
    in_iconv_size_ptr = &in_size;

    while (true) {
        nconv = iconv(cd, in_iconv_buf_ptr, in_iconv_size_ptr,
                      &out_iconv_buf, &out_iconv_size);

        if (nconv != static_cast<size_t>(-1)) {
            // Conversion completed successfully. Now, we need to flush the
            // state. We do this by setting the input pointers to NULL.

            if (!in_iconv_buf_ptr && !in_iconv_size_ptr) {
                // The flushing completed successfully; we can exit
                break;
            }

            in_iconv_buf_ptr = nullptr;
            in_iconv_size_ptr = nullptr;
        } else if (errno == E2BIG) {
            // Our buffer is too small, so we need to increase it.
            // NOTE: Maybe make this less aggressive? We double the buffer size
            // currently, which grows exponentially

            size_t new_out_size;
            char *new_out_buf;

            if (out_size == SIZE_MAX - unit_size) {
                // Reached maximum size
                goto done;
            } else if (out_size <= (SIZE_MAX - unit_size) >> 1) {
                new_out_size = out_size << 1;
            } else {
                new_out_size = SIZE_MAX - unit_size;
            }

            new_out_buf = static_cast<char *>(realloc(
                    out_buf, new_out_size + unit_size));
            if (!new_out_buf) {
                goto done;
            }

            // Adjust iconv pointer and size appropriately
            out_iconv_buf = new_out_buf + (out_size - out_iconv_size);
            out_iconv_size += new_out_size - out_size;

            // Save new pointer and size
            out_buf = new_out_buf;
            out_size = new_out_size;
        } else {
            // Ran into other error
            goto done;
        }
    }

    // Conversion completed successfully. NULL-terminate the buffer.
    if (is_wide_char) {
        *reinterpret_cast<wchar_t *>(out_iconv_buf) = L'\0';
    } else {
        *out_iconv_buf = '\0';
    }

    result = out_buf;

done:
    ErrorRestorer restorer;

    if (cd) {
        iconv_close(cd);
    }
    if (!result) {
        free(out_buf);
    }

    return result;
}

#endif

bool mbs_to_wcs_n(std::wstring &out, const char *str, size_t len)
{
#ifdef _WIN32
    wchar_t *result = win32_convert_to_wcs(CP_ACP, str, len);
#else
    char *c_result = iconv_convert(ICONV_CODE_DEFAULT, ICONV_CODE_WCHAR_T,
                                   str, len * sizeof(char));
    auto result = reinterpret_cast<wchar_t *>(c_result);
#endif

    if (!result) {
        return false;
    }

    out = result;
    free(result);
    return true;
}

std::wstring mbs_to_wcs_n(const char *str, size_t len)
{
    std::wstring result;
    mbs_to_wcs_n(result, str, len);
    return result;
}

bool wcs_to_mbs_n(std::string &out, const wchar_t *str, size_t len)
{
#ifdef _WIN32
    char *result = win32_convert_to_mbs(CP_ACP, str, len);
#else
    char *result = iconv_convert(ICONV_CODE_WCHAR_T, ICONV_CODE_DEFAULT,
                                 reinterpret_cast<const char *>(str),
                                 len * sizeof(wchar_t));
#endif

    if (!result) {
        return false;
    }

    out = result;
    free(result);
    return true;
}

std::string wcs_to_mbs_n(const wchar_t *str, size_t len)
{
    std::string result;
    wcs_to_mbs_n(result, str, len);
    return result;
}

bool utf8_to_wcs_n(std::wstring &out, const char *str, size_t len)
{
#ifdef _WIN32
    wchar_t *result = win32_convert_to_wcs(CP_UTF8, str, len);
#else
    char *c_result = iconv_convert(ICONV_CODE_UTF_8, ICONV_CODE_WCHAR_T,
                                   str, len * sizeof(char));
    auto result = reinterpret_cast<wchar_t *>(c_result);
#endif

    if (!result) {
        return false;
    }

    out = result;
    free(result);
    return true;
}

std::wstring utf8_to_wcs_n(const char *str, size_t len)
{
    std::wstring result;
    utf8_to_wcs_n(result, str, len);
    return result;
}

bool wcs_to_utf8_n(std::string &out, const wchar_t *str, size_t len)
{
#ifdef _WIN32
    char *result = win32_convert_to_mbs(CP_UTF8, str, len);
#else
    char *result = iconv_convert(ICONV_CODE_WCHAR_T, ICONV_CODE_UTF_8,
                                 reinterpret_cast<const char *>(str),
                                 len * sizeof(wchar_t));
#endif

    if (!result) {
        return false;
    }

    out = result;
    free(result);
    return true;
}

std::string wcs_to_utf8_n(const wchar_t *str, size_t len)
{
    std::string result;
    wcs_to_utf8_n(result, str, len);
    return result;
}

bool mbs_to_wcs(std::wstring &out, const char *str)
{
    return mbs_to_wcs_n(out, str, strlen(str));
}

bool mbs_to_wcs(std::wstring &out, const std::string &str)
{
    return mbs_to_wcs_n(out, str.data(), str.size());
}

std::wstring mbs_to_wcs(const char *str)
{
    return mbs_to_wcs_n(str, strlen(str));
}

std::wstring mbs_to_wcs(const std::string &str)
{
    return mbs_to_wcs_n(str.data(), str.size());
}

bool wcs_to_mbs(std::string &out, const wchar_t *str)
{
    return wcs_to_mbs_n(out, str, wcslen(str));
}

bool wcs_to_mbs(std::string &out, const std::wstring &str)
{
    return wcs_to_mbs_n(out, str.data(), str.size());
}

std::string wcs_to_mbs(const wchar_t *str)
{
    return wcs_to_mbs_n(str, wcslen(str));
}

std::string wcs_to_mbs(const std::wstring &str)
{
    return wcs_to_mbs_n(str.data(), str.size());
}

bool utf8_to_wcs(std::wstring &out, const char *str)
{
    return utf8_to_wcs_n(out, str, strlen(str));
}

bool utf8_to_wcs(std::wstring &out, const std::string &str)
{
    return utf8_to_wcs_n(out, str.data(), str.size());
}

std::wstring utf8_to_wcs(const char *str)
{
    return utf8_to_wcs_n(str, strlen(str));
}

std::wstring utf8_to_wcs(const std::string &str)
{
    return utf8_to_wcs_n(str.data(), str.size());
}

bool wcs_to_utf8(std::string &out, const wchar_t *str)
{
    return wcs_to_utf8_n(out, str, wcslen(str));
}

bool wcs_to_utf8(std::string &out, const std::wstring &str)
{
    return wcs_to_utf8_n(out, str.data(), str.size());
}

std::string wcs_to_utf8(const wchar_t *str)
{
    return wcs_to_utf8_n(str, wcslen(str));
}

std::string wcs_to_utf8(const std::wstring &str)
{
    return wcs_to_utf8_n(str.data(), str.size());
}

}
