/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/capi/util.h"

#include <cstdlib>
#include <cstring>

namespace mb
{

char * capi_str_to_cstr(const std::string &str)
{
    if (!str.empty()) {
        return strdup(str.c_str());
    } else {
        return nullptr;
    }
}

std::string capi_cstr_to_str(const char *cstr)
{
    if (cstr) {
        return cstr;
    } else {
        return {};
    }
}

char ** capi_vector_to_cstr_array(const std::vector<std::string> &array)
{
    if (!array.empty()) {
        auto c_array = static_cast<char **>(std::malloc(
                sizeof(char *) * (array.size() + 1)));
        if (!c_array) {
            return nullptr;
        }

        for (size_t i = 0; i < array.size(); ++i) {
            c_array[i] = strdup(array[i].c_str());
            if (!c_array[i]) {
                for (size_t j = 0; j < i; ++j) {
                    free(c_array[j]);
                }
                free(c_array);
                return nullptr;
            }
        }
        c_array[array.size()] = nullptr;

        return c_array;
    } else {
        return nullptr;
    }
}

std::vector<std::string> capi_cstr_array_to_vector(const char * const *array)
{
    if (array) {
        std::vector<std::string> list;
        for (; *array != nullptr; ++array) {
            list.emplace_back(*array);
        }
        return list;
    } else {
        return {};
    }
}

}
