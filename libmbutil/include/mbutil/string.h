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
#include <vector>

#include "mbcommon/common.h"

namespace mb::util
{

void replace(std::string &source,
             const std::string &from, const std::string &to);
void replace_all(std::string &source,
                 const std::string &from, const std::string &to);

std::vector<std::string> tokenize(std::string str, const std::string &delims);

std::string hex_string(const unsigned char *data, size_t size);

char ** dup_cstring_list(const char * const *list);
void free_cstring_list(char **list);

}
