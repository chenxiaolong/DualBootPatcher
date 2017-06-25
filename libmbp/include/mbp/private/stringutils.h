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

#pragma once

#include <string>
#include <vector>

#include "mbcommon/common.h"

class StringUtils
{
public:
    static std::string format(const char *fmt, ...) MB_PRINTF(1, 2);

    static std::vector<std::string> splitData(const std::vector<unsigned char> &data,
                                              unsigned char delim);
    static std::vector<std::string> split(const std::string &str, char delim);
    static std::vector<unsigned char> joinData(std::vector<std::string> &list,
                                               unsigned char delim);
    static std::string join(std::vector<std::string> &list,
                            const std::string &delim);

    static void replace(std::string *source,
                        const std::string &from, const std::string &to);
    static void replace_all(std::string *source,
                            const std::string &from, const std::string &to);

    static std::string toHex(const unsigned char *data, std::size_t size);

    static std::string toMaxString(const char *str, std::size_t maxSize);

    static std::string toPrintable(const unsigned char *data, std::size_t size);
};
