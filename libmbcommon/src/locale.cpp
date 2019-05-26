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

#include "mbcommon/locale.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#ifdef _WIN32
#  include <windows.h>
#elif defined(USE_EXTERNAL_ICONV)
#  include "mbcommon/external/iconv.h"
#else
#  include <iconv.h>
#endif

#include "mbcommon/error.h"
#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"

namespace mb
{

#ifdef _WIN32

static oc::result<std::wstring> win32_convert_to_wcs(UINT code_page,
                                                     std::string_view in)
{
    // Win32 cannot handle empty strings, so we'll manually handle the trivial
    // case
    if (in.empty()) {
        return L"";
    }

    // MultiByteToWideChar() only accepts an int
    if (in.size() > INT_MAX) {
        return ec_from_win32(ERROR_INVALID_PARAMETER);
    }

    int n = MultiByteToWideChar(code_page, 0, in.data(),
                                static_cast<int>(in.size()), nullptr, 0);
    if (n == 0) {
        return ec_from_win32();
    }

    // n does not include the NULL-terminator if the input did not contain one
    std::wstring out_buf;
    out_buf.resize(static_cast<size_t>(n));

    n = MultiByteToWideChar(code_page, 0, in.data(),
                            static_cast<int>(in.size()), out_buf.data(), n);
    if (n == 0) {
        return ec_from_win32();
    }

    return std::move(out_buf);
}

static oc::result<std::string> win32_convert_to_mbs(UINT code_page,
                                                    std::wstring_view in)
{
    // Win32 cannot handle empty strings, so we'll manually handle the trivial
    // case
    if (in.empty()) {
        return "";
    }

    // WideCharToMultiByte() only accepts an int
    if (in.size() > INT_MAX) {
        return ec_from_win32(ERROR_INVALID_PARAMETER);
    }

    int n = WideCharToMultiByte(code_page, 0, in.data(),
                                static_cast<int>(in.size()), nullptr, 0,
                                nullptr, nullptr);
    if (n == 0) {
        return ec_from_win32();
    }

    // n does not include the NULL-terminator if the input did not contain one
    std::string out_buf;
    out_buf.resize(static_cast<size_t>(n));

    n = WideCharToMultiByte(code_page, 0, in.data(),
                            static_cast<int>(in.size()), out_buf.data(), n,
                            nullptr, nullptr);
    if (n == 0) {
        return ec_from_win32();
    }

    return std::move(out_buf);
}

#else

#ifdef __ANDROID__
static constexpr char ICONV_CODE_DEFAULT[] = "UTF-8";
#else
static constexpr char ICONV_CODE_DEFAULT[] = "";
#endif
static constexpr char ICONV_CODE_WCHAR_T[] = "WCHAR_T";
static constexpr char ICONV_CODE_UTF_8  [] = "UTF-8";

template<class Container>
static char * get_char_ptr(const Container &str)
{
    return reinterpret_cast<char *>(const_cast<std::remove_const_t<
            typename Container::value_type> *>(str.data()));
}

template<class Container>
static size_t get_bytes(const Container &str)
{
    return str.size() * sizeof(typename Container::value_type);
}

template<class InStrV, class OutStr>
static oc::result<OutStr> iconv_convert(const char *from_code,
                                        const char *to_code,
                                        InStrV in)
{
    OutStr result;
    // Start off with a buffer size equal to the input
    result.resize(in.size());

    // For manipulation by iconv. iconv() works with chars only
    char *in_buf = get_char_ptr(in);
    size_t in_bytes = get_bytes(in);
    char *out_buf = get_char_ptr(result);
    size_t out_bytes = get_bytes(result);

    iconv_t cd = iconv_open(to_code, from_code);
    if (cd == reinterpret_cast<iconv_t>(-1)) {
        return ec_from_errno();
    }

    auto close_cd = finally([&] {
        ErrorRestorer restorer;
        iconv_close(cd);
    });

    while (true) {
        size_t nconv = iconv(cd, &in_buf, &in_bytes, &out_buf, &out_bytes);

        if (nconv != static_cast<size_t>(-1)) {
            // Conversion completed successfully. Now, we need to flush the
            // state. We do this by setting the input pointers to NULL.

            if (!in_buf) {
                // The flushing completed successfully; we can exit
                break;
            }

            in_buf = nullptr;
        } else if (errno == E2BIG) {
            // Our buffer is too small, so we need to increase it.
            // NOTE: Maybe make this less aggressive? We double the buffer size
            // currently, which grows exponentially

            auto prev_offset = out_buf - get_char_ptr(result);
            size_t prev_size = result.size();
            size_t new_size;

            if (result.size() == SIZE_MAX) {
                // Reached maximum size
                return ec_from_errno();
            } else if (result.size() <= SIZE_MAX >> 1) {
                new_size = result.size() << 1;
            } else {
                new_size = SIZE_MAX;
            }

            result.resize(new_size);

            // Adjust iconv pointer and size appropriately
            out_buf = get_char_ptr(result) + prev_offset;
            out_bytes += new_size - prev_size;
        } else {
            // Ran into other error
            return ec_from_errno();
        }
    }

    // Resize to correct length
    auto typed_ptr = reinterpret_cast<typename OutStr::value_type *>(out_buf);
    result.resize(static_cast<typename OutStr::size_type>(
            typed_ptr - result.data()));

    // Conversion completed successfully
    return std::move(result);
}

#endif

oc::result<std::wstring> mbs_to_wcs(std::string_view str)
{
#ifdef _WIN32
    return win32_convert_to_wcs(CP_ACP, str);
#else
    return iconv_convert<std::string_view, std::wstring>(
            ICONV_CODE_DEFAULT, ICONV_CODE_WCHAR_T, str);
#endif
}

oc::result<std::string> wcs_to_mbs(std::wstring_view str)
{
#ifdef _WIN32
    return win32_convert_to_mbs(CP_ACP, str);
#else
    return iconv_convert<std::wstring_view, std::string>(
            ICONV_CODE_WCHAR_T, ICONV_CODE_DEFAULT, str);
#endif
}

oc::result<std::wstring> utf8_to_wcs(std::string_view str)
{
#ifdef _WIN32
    return win32_convert_to_wcs(CP_UTF8, str);
#else
    return iconv_convert<std::string_view, std::wstring>(
            ICONV_CODE_UTF_8, ICONV_CODE_WCHAR_T, str);
#endif
}

oc::result<std::string> wcs_to_utf8(std::wstring_view str)
{
#ifdef _WIN32
    return win32_convert_to_mbs(CP_UTF8, str);
#else
    return iconv_convert<std::wstring_view, std::string>(
            ICONV_CODE_WCHAR_T, ICONV_CODE_UTF_8, str);
#endif
}

}
