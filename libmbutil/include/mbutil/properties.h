/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <string>
#include <unordered_map>

#include "mbutil/integer.h"
#include "mbutil/external/system_properties.h"

namespace mb
{
namespace util
{

// Wrappers around the libc functions that automatically dlopen libc.so
int libc_system_property_get(const char *name, char *value);
int libc_system_property_set(const char *key, const char *value);
const prop_info *libc_system_property_find(const char *name);
int libc_system_property_read(const prop_info *pi, char *name, char *value);
const prop_info *libc_system_property_find_nth(unsigned n);
int libc_system_property_foreach(
        void (*propfn)(const prop_info *pi, void *cookie),
        void *cookie);

int property_get(const char *key, char *value_out, const char *default_value);
int property_set(const char *key, const char *value);

bool property_get_bool(const char *key, bool default_value);

template<typename SIntType>
inline SIntType property_get_snum(const char *key, SIntType default_value)
{
    char buf[PROP_VALUE_MAX];
    SIntType value;

    int n = property_get(key, buf, "");
    return (n >= 0 && str_to_snum(buf, 10, value))
            ? value : default_value;
}

template<typename UIntType>
inline UIntType property_get_unum(const char *key, UIntType default_value)
{
    char buf[PROP_VALUE_MAX];
    UIntType value;

    int n = property_get(key, buf, "");
    return (n >= 0 && str_to_unum(buf, 10, value))
            ? value : default_value;
}

typedef void (*property_list_cb)(const char *key, const char *value,
                                 void *cookie);

int property_list(property_list_cb propfn, void *cookie);

bool get_all_properties(std::unordered_map<std::string, std::string> *map);
bool file_get_property(const std::string &path,
                       const std::string &key,
                       std::string *out,
                       const std::string &default_value);
bool file_get_all_properties(const std::string &path,
                             std::unordered_map<std::string, std::string> *map);
bool file_write_properties(const std::string &path,
                           const std::unordered_map<std::string, std::string> &map);

}
}
