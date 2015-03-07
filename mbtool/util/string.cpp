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

namespace mb
{
namespace util
{

bool starts_with(const std::string &string, const std::string &prefix)
{
    return string.compare(0, prefix.length(), prefix) == 0;
}

bool ends_with(const std::string &string, const std::string &suffix)
{
    if (string.size() < suffix.size()) {
        return false;
    }

    return string.compare(string.size() - suffix.size(), suffix.size(), suffix) == 0;
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