/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include "util/properties.h"

#include <string.h>

int mb_get_property(const char *name, char *value_out, char *default_value)
{
    int len = __system_property_get(name, value_out);
    if (len == 0) {
        if (default_value) {
            return (int) strlcpy(value_out, default_value, MB_PROP_VALUE_MAX);
        } else {
            return 0;
        }
    } else {
        return len;
    }
}

int mb_set_property(const char *name, const char *value)
{
    return __system_property_set(name, value);
}
