/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "private/stringutils.h"

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

static bool starts_with_internal(const char *string, std::size_t len_string,
                                 const char *prefix, std::size_t len_prefix,
                                 bool case_insensitive)
{
    return len_string < len_prefix ? false :
            (case_insensitive ? strncasecmp : strncmp)
                    (string, prefix, len_prefix) == 0;
}

static bool ends_with_internal(const char *string, std::size_t len_string,
                               const char *suffix, std::size_t len_suffix,
                               bool case_insensitive)
{
    return len_string < len_suffix ? false :
            (case_insensitive ? strncasecmp : strncmp)
                    (string + len_string - len_suffix, suffix, len_suffix) == 0;
}

bool StringUtils::starts_with(const std::string &string, const std::string &prefix)
{
    return starts_with_internal(string.c_str(), string.size(),
                                prefix.c_str(), prefix.length(),
                                false);
}

bool StringUtils::starts_with(const char *string, const char *prefix)
{
    return starts_with_internal(string, strlen(string),
                                prefix, strlen(prefix),
                                false);
}

bool StringUtils::ends_with(const std::string &string, const std::string &suffix)
{
    return ends_with_internal(string.c_str(), string.size(),
                              suffix.c_str(), suffix.size(),
                              false);
}

bool StringUtils::ends_with(const char *string, const char *suffix)
{
    return ends_with_internal(string, strlen(string),
                              suffix, strlen(suffix),
                              false);
}

bool StringUtils::istarts_with(const std::string &string, const std::string &prefix)
{
    return starts_with_internal(string.c_str(), string.size(),
                                prefix.c_str(), prefix.length(),
                                true);
}

bool StringUtils::istarts_with(const char *string, const char *prefix)
{
    return starts_with_internal(string, strlen(string),
                                prefix, strlen(prefix),
                                true);
}

bool StringUtils::iends_with(const std::string &string, const std::string &suffix)
{
    return ends_with_internal(string.c_str(), string.size(),
                              suffix.c_str(), suffix.size(),
                              true);
}

bool StringUtils::iends_with(const char *string, const char *suffix)
{
    return ends_with_internal(string, strlen(string),
                              suffix, strlen(suffix),
                              true);
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

std::string StringUtils::join(std::vector<std::string> &list, char delim)
{
    std::string result;
    bool first = true;

    for (const std::string &str : list) {
        if (!first) {
            result.push_back(delim);
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
