/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "mbcommon/integer.h"


namespace mb::util
{

enum class PropertyIterAction
{
    Continue,
    Stop,
};

using PropertyIterCb = PropertyIterAction(std::string_view key,
                                          std::string_view value);
using PropertiesMap = std::unordered_map<std::string, std::string>;

// Helper functions

std::optional<std::string> property_get(const std::string &key);
std::string property_get_string(const std::string &key,
                                const std::string &default_value);
bool property_get_bool(const std::string &key, bool default_value);

template<typename IntType>
inline IntType property_get_num(const std::string &key, IntType default_value)
{
    if (auto value = property_get(key)) {
        IntType result;
        if (str_to_num(value->c_str(), 10, result)) {
            return result;
        }
    }

    return default_value;
}

bool property_set(const std::string &key, const std::string &value);
bool property_set_direct(const std::string &key, const std::string &value);

bool property_iter(const std::function<PropertyIterCb> &fn);

std::optional<PropertiesMap> property_get_all();

// Properties file functions

std::optional<std::string> property_file_get(const std::string &path,
                                             const std::string &key);
std::string property_file_get_string(const std::string &path,
                                     const std::string &key,
                                     const std::string &default_value);
bool property_file_get_bool(const std::string &path, const std::string &key,
                            bool default_value);

template<typename IntType>
inline IntType property_file_get_num(const std::string &path,
                                     const std::string &key,
                                     IntType default_value)
{
    if (auto value = property_file_get(path, key)) {
        IntType result;
        if (str_to_num(value->c_str(), 10, result)) {
            return result;
        }
    }

    return default_value;
}

bool property_file_iter(const std::string &path, std::string_view filter,
                        const std::function<PropertyIterCb> &fn);

std::optional<PropertiesMap> property_file_get_all(const std::string &path);

bool property_file_write_all(const std::string &path, const PropertiesMap &map);

}
