/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbp/private/stringutils.h"

#include <algorithm>
#include <memory>
#include <cstdarg>
#include <cstring>

std::string StringUtils::format(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    std::size_t size = vsnprintf(nullptr, 0, fmt, ap) + 1;
    va_end(ap);

    std::unique_ptr<char[]> buf(new char[size]);

    va_start(ap, fmt);
    vsnprintf(buf.get(), size, fmt, ap);
    va_end(ap);

    return std::string(buf.get(), buf.get() + size - 1);
}

std::vector<std::string> StringUtils::splitData(const std::vector<unsigned char> &data,
                                                unsigned char delim)
{
    std::vector<unsigned char>::const_iterator begin = data.begin();
    std::vector<unsigned char>::const_iterator end;
    std::vector<std::string> result;

    while ((end = std::find(begin, data.end(), delim)) != data.end()) {
        result.emplace_back(begin, end);
        begin = end + 1;
    }
    result.emplace_back(begin, data.end());

    return result;
}

std::vector<std::string> StringUtils::split(const std::string &str, char delim)
{
    std::size_t begin = 0;
    std::size_t end;
    std::vector<std::string> result;

    while ((end = str.find(delim, begin)) != std::string::npos) {
        result.push_back(str.substr(begin, end - begin));
        begin = end + 1;
    }
    result.push_back(str.substr(begin));

    return result;
}

std::vector<unsigned char> StringUtils::joinData(std::vector<std::string> &list,
                                                 unsigned char delim)
{
    std::vector<unsigned char> result;
    bool first = true;

    for (const std::string &str : list) {
        if (!first) {
            result.push_back(delim);
        } else {
            first = false;
        }
        result.insert(result.end(), str.begin(), str.end());
    }

    return result;
}

std::string StringUtils::join(std::vector<std::string> &list,
                              const std::string &delim)
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

void StringUtils::replace(std::string *source,
                          const std::string &from, const std::string &to)
{
    replace_internal(source, from, to, true);
}

void StringUtils::replace_all(std::string *source,
                              const std::string &from, const std::string &to)
{
    replace_internal(source, from, to, false);
}

std::string StringUtils::toHex(const unsigned char *data, std::size_t size)
{
    static const char digits[] = "0123456789abcdef";

    std::string hex;
    hex.reserve(2 * size);
    for (std::size_t i = 0; i < size; ++i) {
        hex += digits[(data[i] >> 4) & 0xf];
        hex += digits[data[i] & 0xf];
    }

    return hex;
}

std::string StringUtils::toMaxString(const char *str, std::size_t maxSize)
{
    return std::string(str, strnlen(str, maxSize));
}

std::string StringUtils::toPrintable(const unsigned char *data, std::size_t size)
{
    static const char digits[] = "0123456789abcdef";

    std::string output;

    // Most characters are unprintable, resulting in 4 bytes of output per byte
    // of input
    output.reserve(4 * size);

    for (std::size_t i = 0; i < size; ++i) {
        if (data[i] == '\\') {
            output += "\\\\";
        } else if (isprint(data[i])) {
            output += static_cast<char>(data[i]);
        } else if (data[i] == '\a') {
            output += "\\a";
        } else if (data[i] == '\b') {
            output += "\\b";
        } else if (data[i] == '\f') {
            output += "\\f";
        } else if (data[i] == '\n') {
            output += "\\n";
        } else if (data[i] == '\r') {
            output += "\\r";
        } else if (data[i] == '\t') {
            output += "\\t";
        } else if (data[i] == '\v') {
            output += "\\v";
        } else {
            output += "\\x";
            output += digits[(data[i] >> 4) & 0xf];
            output += digits[data[i] & 0xf];
        }
    }

    return output;
}
