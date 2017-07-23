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

#include "mbcommon/common.h"

#ifdef __cplusplus
#  include <string>
#  include <vector>
#endif

#ifdef __cplusplus
namespace mb
{

MB_EXPORT char * capi_str_to_cstr(const std::string &str);
MB_EXPORT std::string capi_cstr_to_str(const char *cstr);

MB_EXPORT char ** capi_vector_to_cstr_array(const std::vector<std::string> &array);
MB_EXPORT std::vector<std::string> capi_cstr_array_to_vector(const char * const *array);

}
#endif
