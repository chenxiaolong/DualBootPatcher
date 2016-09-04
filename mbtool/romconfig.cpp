/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "romconfig.h"

#include <jansson.h>

#include "mblog/logging.h"
#include "mbutil/finally.h"

#define CONFIG_KEY_ID                      "id"
#define CONFIG_KEY_NAME                    "name"

namespace mb {

/*
 * Example JSON structure:
 *
 * {
 *     "id": "primary",
 *     "name": "TouchWiz 5.0"
 * }
 */
bool RomConfig::load_file(const std::string &path)
{
    json_t *root;
    json_error_t error;

    root = json_load_file(path.c_str(), 0, &error);
    if (!root) {
        LOGE("JSON error on line %d: %s", error.line, error.text);
        return false;
    }

    auto free_on_return = util::finally([&]{
        json_decref(root);
    });

    if (!json_is_object(root)) {
        LOGE("[root]: Not an object");
        return false;
    }

    // ROM ID
    json_t *j_id = json_object_get(root, CONFIG_KEY_ID);
    if (j_id) {
        if (!json_is_string(j_id)) {
            LOGE("[root]->id: Not a string");
            return false;
        }
        id = json_string_value(j_id);
    }

    // ROM name
    json_t *j_name = json_object_get(root, CONFIG_KEY_NAME);
    if (j_name) {
        if (!json_is_string(j_name)) {
            LOGE("[root]->name: Not a string");
            return false;
        }
        name = json_string_value(j_name);
    }

    return true;
}

}
