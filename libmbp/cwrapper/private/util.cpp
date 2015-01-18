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

#include "cwrapper/private/util.h"

#include <cstdlib>
#include <cstring>

char * string_to_cstring(const std::string &str)
{
    return strdup(str.c_str());
}

char ** vector_to_cstring_array(const std::vector<std::string> &array)
{
    char **cArray = static_cast<char **>(std::malloc(
            sizeof(char *) * (array.size() + 1)));
    for (unsigned int i = 0; i < array.size(); ++i) {
        cArray[i] = strdup(array[i].c_str());
    }
    cArray[array.size()] = nullptr;

    return cArray;
}

std::vector<std::string> cstring_array_to_vector(const char **array)
{
    std::vector<std::string> list;
    while (*array != nullptr) {
        list.push_back(*array);
        ++array;
    }
    return list;
}

void vector_to_data(const std::vector<unsigned char> &data,
                    void **data_out, size_t *size_out)
{
    *data_out = std::malloc(data.size());
    *size_out = data.size();
    std::memcpy(*data_out, data.data(), data.size());
}

size_t vector_to_data2(const std::vector<unsigned char> &data, void **data_out)
{
    size_t size;
    vector_to_data(data, data_out, &size);
    return size;
}

std::vector<unsigned char> data_to_vector(const void *data, size_t size)
{
    std::vector<unsigned char> v(size);
    std::memcpy(v.data(), data, size);
    return v;
}
