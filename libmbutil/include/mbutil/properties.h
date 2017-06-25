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

#pragma once

#include <string>
#include <unordered_map>

#include "mbutil/integer.h"
#include "mbutil/external/system_properties.h"

struct timespec;

namespace mb
{
namespace util
{

typedef void (*PropertyListCb)(const std::string &key,
                               const std::string &value,
                               void *cookie);

// Non-deprecated libc system properties wrapper functions

int libc_system_property_set(const char *key, const char *value);
const prop_info *libc_system_property_find(const char *name);
void libc_system_property_read_callback(
        const prop_info *pi,
        void (*callback)(void *cookie, const char *name, const char *value,
                         uint32_t serial),
        void *cookie);
int libc_system_property_foreach(
        void (*propfn)(const prop_info *pi, void *cookie),
        void *cookie);
bool libc_system_property_wait(const prop_info *pi,
                               uint32_t old_serial,
                               uint32_t *new_serial_ptr,
                               const struct timespec *relative_timeout);

// Helper functions

bool property_get(const std::string &key, std::string &value_out);
std::string property_get_string(const std::string &key,
                                const std::string &default_value);
bool property_get_bool(const std::string &key, bool default_value);

template<typename SNumType>
inline SNumType property_get_snum(const std::string &key,
                                  SNumType default_value)
{
    std::string value;
    SNumType result;

    if (property_get(key, value) && str_to_snum(value.c_str(), 10, &result)) {
        return result;
    }

    return default_value;
}

template<typename UNumType>
inline UNumType property_get_unum(const std::string &key,
                                  UNumType default_value)
{
    std::string value;
    UNumType result;

    if (property_get(key, value) && str_to_unum(value.c_str(), 10, &result)) {
        return result;
    }

    return default_value;
}

bool property_set(const std::string &key, const std::string &value);
bool property_set_direct(const std::string &key, const std::string &value);

bool property_list(PropertyListCb prop_fn, void *cookie);

bool property_get_all(std::unordered_map<std::string, std::string> &map);

// Properties file functions

bool property_file_get(const std::string &path, const std::string &key,
                       std::string &value_out);
std::string property_file_get_string(const std::string &path,
                                     const std::string &key,
                                     const std::string &default_value);
bool property_file_get_bool(const std::string &path, const std::string &key,
                            bool default_value);

template<typename SNumType>
inline SNumType property_file_get_snum(const std::string &path,
                                       const std::string &key,
                                       SNumType default_value)
{
    std::string value;
    SNumType result;

    if (property_file_get(path, key, value)
            && str_to_snum(value.c_str(), 10, &result)) {
        return result;
    }

    return default_value;
}

template<typename UNumType>
inline UNumType property_file_get_unum(const std::string &path,
                                       const std::string &key,
                                       UNumType default_value)
{
    std::string value;
    UNumType result;

    if (property_file_get(path, key, value)
            && str_to_unum(value.c_str(), 10, &result)) {
        return result;
    }

    return default_value;
}

bool property_file_list(const std::string &path, PropertyListCb prop_fn,
                        void *cookie);

bool property_file_get_all(const std::string &path,
                           std::unordered_map<std::string, std::string> &map);

bool property_file_write_all(const std::string &path,
                             const std::unordered_map<std::string, std::string> &map);

}
}
