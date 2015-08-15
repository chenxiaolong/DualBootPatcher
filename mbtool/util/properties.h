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

#include <sys/system_properties.h>

#define MB_PROP_NAME_MAX  PROP_NAME_MAX
#define MB_PROP_VALUE_MAX PROP_VALUE_MAX

namespace mb
{
namespace util
{

void get_property(const std::string &name,
                  std::string *value_out,
                  const std::string &default_value);
bool set_property(const std::string &name,
                  const std::string &value);
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
