/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/string.h"

#include <memory>

#include <cstdarg>
#include <cstring>

namespace mb
{
namespace util
{

std::string format(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    std::string result = formatv(fmt, ap);
    va_end(ap);

    return result;
}

std::string formatv(const char *fmt, va_list ap)
{
    va_list copy;

    va_copy(copy, ap);
    int chars = vsnprintf(nullptr, 0, fmt, ap);
    va_end(copy);

    if (chars < 0) {
        return std::string();
    }

    std::unique_ptr<char[]> buf(new char[chars + 1]);

    va_copy(copy, ap);
    chars = vsnprintf(buf.get(), chars + 1, fmt, ap);
    va_end(copy);

    if (chars < 0) {
        return std::string();
    }

    return std::string(buf.get(), buf.get() + chars);
}

bool starts_with_internal(const char *string, std::size_t len_string,
                          const char *prefix, std::size_t len_prefix,
                          bool case_insensitive)
{
    return len_string < len_prefix ? false :
            (case_insensitive ? strncasecmp : strncmp)
                    (string, prefix, len_prefix) == 0;
}

bool ends_with_internal(const char *string, std::size_t len_string,
                        const char *suffix, std::size_t len_suffix,
                        bool case_insensitive)
{
    return len_string < len_suffix ? false :
            (case_insensitive ? strncasecmp : strncmp)
                    (string + len_string - len_suffix, suffix, len_suffix) == 0;
}

bool starts_with(const std::string &string, const std::string &prefix)
{
    return starts_with_internal(string.c_str(), string.size(),
                                prefix.c_str(), prefix.length(),
                                false);
}

bool starts_with(const char *string, const char *prefix)
{
    return starts_with_internal(string, strlen(string),
                                prefix, strlen(prefix),
                                false);
}

bool ends_with(const std::string &string, const std::string &suffix)
{
    return ends_with_internal(string.c_str(), string.size(),
                              suffix.c_str(), suffix.size(),
                              false);
}

bool ends_with(const char *string, const char *suffix)
{
    return ends_with_internal(string, strlen(string),
                              suffix, strlen(suffix),
                              false);
}

static void replace_internal(std::string *source,
                             const std::string &from, const std::string &to,
                             bool replace_first_only)
{
    if (from.empty()) {
        return;
    }

    std::size_t pos = 0;
    while ((pos = source->find(from, pos)) != std::string::npos) {
        source->replace(pos, from.size(), to);
        if (replace_first_only) {
            return;
        }
        pos += to.size();
    }
}

void replace(std::string *source,
             const std::string &from, const std::string &to)
{
    replace_internal(source, from, to, true);
}

void replace_all(std::string *source,
                 const std::string &from, const std::string &to)
{
    replace_internal(source, from, to, false);
}

std::vector<std::string> split(const std::string &str, const std::string &delim)
{
    std::size_t begin = 0;
    std::size_t end;
    std::vector<std::string> result;

    if (!delim.empty()) {
        while ((end = str.find(delim, begin)) != std::string::npos) {
            result.push_back(str.substr(begin, end - begin));
            begin = end + delim.size();
        }
        result.push_back(str.substr(begin));
    }

    return result;
}

std::string join(const std::vector<std::string> &list, std::string delim)
{
    std::string result;
    bool first = true;

    for (const std::string &str : list) {
        if (!first) {
            result += delim;
        } else {
            first = false;
        }
        result += str;
    }

    return result;
}

std::vector<std::string> tokenize(const std::string &str,
                                  const std::string &delims)
{
    std::vector<char> linebuf(str.begin(), str.end());
    linebuf.resize(linebuf.size() + 1);
    std::vector<std::string> tokens;

    char *temp;
    char *token;

    token = strtok_r(linebuf.data(), delims.c_str(), &temp);
    while (token != nullptr) {
        tokens.push_back(token);

        token = strtok_r(nullptr, delims.c_str(), &temp);
    }

    return tokens;
}

/*!
 * \brief Convert binary data to its hex string representation
 *
 * The size of the output string should be at least `2 * size + 1` bytes.
 * (Two characters in string for each byte in the binary data + one byte for
 * the NULL terminator.)
 *
 * \param data Binary data
 * \param size Size of binary data
 */
std::string hex_string(unsigned char *data, size_t size)
{
    static const char digits[] = "0123456789abcdef";

    std::string result;
    result.resize(size * 2);

    for (unsigned int i = 0; i < size; ++i) {
        result[2 * i] = digits[(data[i] >> 4) & 0xf];
        result[2 * i + 1] = digits[data[i] & 0xf];
    }

    return result;
}

}
}
