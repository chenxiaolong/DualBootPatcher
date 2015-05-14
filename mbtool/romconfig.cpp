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

#include "util/finally.h"
#include "util/logging.h"

#define CONFIG_KEY_ID                      "id"
#define CONFIG_KEY_NAME                    "name"
#define CONFIG_KEY_APP_SHARING             "app_sharing"
#define CONFIG_KEY_GLOBAL_APP_SHARING      "global"
#define CONFIG_KEY_GLOBAL_APP_SHARING_PAID "global_paid"
#define CONFIG_KEY_INDIVIDUAL_APP_SHARING  "individual"
#define CONFIG_KEY_PACKAGES                "packages"
#define CONFIG_KEY_PACKAGE_ID              "pkg_id"
#define CONFIG_KEY_SHARE_APK               "share_apk"
#define CONFIG_KEY_SHARE_DATA              "share_data"

namespace mb {

/*
 * Example JSON structure:
 *
 * {
 *     "id": "primary",
 *     "name": "TouchWiz 5.0",
 *     "app_sharing": {
 *         "global": false,
 *         "global_paid": false,
 *         "individual": true
 *         "packages": [
 *             {
 *                 "pkg_id": "com.android.chrome",
 *                 "share_apk": true,
 *                 "share_data": true
 *             },{
 *                 "pkg_id": "com.android.vending",
 *                 "share_apk":false,
 *                 "share_data":true
 *             }
 *         ]
 *     }
 * }
 */
bool RomConfig::load_file(const std::string &path)
{
    json_t *root;
    json_error_t error;

    root = json_load_file(path.c_str(), 0, &error);
    if (!root) {
        LOGE("JSON error on line {:d}: {}", error.line, error.text);
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

    // App sharing
    json_t *j_app_sharing = json_object_get(root, CONFIG_KEY_APP_SHARING);
    if (j_app_sharing) {
        if (!json_is_object(j_app_sharing)) {
            LOGE("[root]->app_sharing: Not an object");
            return false;
        }

        // Global app sharing
        json_t *j_global = json_object_get(
                j_app_sharing, CONFIG_KEY_GLOBAL_APP_SHARING);
        if (j_global) {
            if (!json_is_boolean(j_global)) {
                LOGE("[root]->app_sharing->global: Not a boolean");
                return false;
            }
            global_app_sharing = json_is_true(j_global);
        }

        // Global paid app sharing
        json_t *j_global_paid = json_object_get(
                j_app_sharing, CONFIG_KEY_GLOBAL_APP_SHARING_PAID);
        if (j_global_paid) {
            if (!json_is_boolean(j_global_paid)) {
                LOGE("[root]->app_sharing->global_paid: Not a boolean");
                return false;
            }
            global_paid_app_sharing = json_is_true(j_global_paid);
        }

        // Individual app sharing
        json_t *j_individual = json_object_get(
                j_app_sharing, CONFIG_KEY_INDIVIDUAL_APP_SHARING);
        if (j_individual) {
            if (!json_is_boolean(j_individual)) {
                LOGE("[root]->app_sharing->individual: Not a boolean");
                return false;
            }
            indiv_app_sharing = json_is_true(j_individual);
        }

        // Shared packages
        json_t *j_pkgs = json_object_get(j_app_sharing, CONFIG_KEY_PACKAGES);
        if (j_pkgs) {
            if (!json_is_array(j_pkgs)) {
                LOGE("[root]->app_sharing->packages: Not an array");
                return false;
            }

            for (std::size_t i = 0; i < json_array_size(j_pkgs); ++i) {
                json_t *j_data = json_array_get(j_pkgs, i);
                if (j_data) {
                    if (!json_is_object(j_data)) {
                        LOGE("[root]->app_sharing->packages[{}]: Not an object", i);
                        return false;
                    }

                    SharedPackage shared_pkg;

                    // Package ID
                    json_t *j_pkg_id = json_object_get(
                            j_data, CONFIG_KEY_PACKAGE_ID);
                    if (j_pkg_id) {
                        if (!json_is_string(j_pkg_id)) {
                            LOGE("[root]->app_sharing->packages[{}]->pkg_id: Not a string", i);
                            return false;
                        }
                        shared_pkg.pkg_id = json_string_value(j_pkg_id);
                    } else {
                        // Skip empty package names
                        continue;
                    }

                    // Shared APK
                    json_t *j_share_apk = json_object_get(
                            j_data, CONFIG_KEY_SHARE_APK);
                    if (j_share_apk) {
                        if (!json_is_boolean(j_share_apk)) {
                            LOGE("[root]->app_sharing->packages[{}]->share_apk: Not a boolean", i);
                            return false;
                        }
                        shared_pkg.share_apk = json_is_true(j_share_apk);
                    }

                    // Shared data
                    json_t *j_share_data = json_object_get(
                            j_data, CONFIG_KEY_SHARE_DATA);
                    if (j_share_data) {
                        if (!json_is_boolean(j_share_data)) {
                            LOGE("[root]->app_sharing->packages[{}]->share_data: Not a boolean", i);
                            return false;
                        }
                        shared_pkg.share_data = json_is_true(j_share_data);
                    }

                    shared_pkgs.push_back(std::move(shared_pkg));
                }
            }
        }
    }

    return true;
}

}
