/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/schema.h"

#include <array>

#include <rapidjson/schema.h>

#include "schemas_gen.h"

using namespace rapidjson;

namespace mb::device
{

const char * find_schema(const std::string &uri)
{
    using SchemaItem = decltype(g_schemas)::value_type;

    auto it = std::find_if(g_schemas.begin(), g_schemas.end(),
                           [&uri](const SchemaItem &item) {
        return uri == item.first;
    });

    return it == g_schemas.end() ? nullptr : it->second;
}

}
