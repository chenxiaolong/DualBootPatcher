/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "romconfig.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/reader.h>

#include "mblog/logging.h"

#define CONFIG_KEY_ID                      "id"
#define CONFIG_KEY_NAME                    "name"
#define CONFIG_KEY_APP_SHARING             "app_sharing"
#define CONFIG_KEY_INDIVIDUAL_APP_SHARING  "individual"
#define CONFIG_KEY_PACKAGES                "packages"
#define CONFIG_KEY_PACKAGE_ID              "pkg_id"
#define CONFIG_KEY_SHARE_DATA              "share_data"

namespace mb {

/*
 * Example JSON structure:
 *
 * {
 *     "id": "primary",
 *     "name": "TouchWiz 5.0",
 *     "app_sharing": {
 *         "individual": true
 *         "packages": [
 *             {
 *                 "pkg_id": "com.android.chrome",
 *                 "share_data": true
 *             },{
 *                 "pkg_id": "com.android.vending",
 *                 "share_data":true
 *             }
 *         ]
 *     }
 * }
 */
bool RomConfig::load_file(const std::string &path)
{
    using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;
    using namespace rapidjson;

    ScopedFILE fp(fopen(path.c_str(), "r"), &fclose);
    if (!fp) {
        LOGE("%s: Failed to open for reading: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    char buf[65536];
    Reader reader;
    FileReadStream is(fp.get(), buf, sizeof(buf));
    Document d;

    if (d.ParseStream(is).HasParseError()) {
        fprintf(stderr, "Error at offset %zu: %s\n",
                reader.GetErrorOffset(),
                GetParseError_En(reader.GetParseErrorCode()));
        return false;
    }

    if (!d.IsObject()) {
        LOGE("[root]: Not an object");
        return false;
    }

    // ROM ID
    auto const j_id = d.FindMember(CONFIG_KEY_ID);
    if (j_id != d.MemberEnd()) {
        if (!j_id->value.IsString()) {
            LOGE("[root]->id: Not a string");
            return false;
        }
        id = j_id->value.GetString();
    }

    // ROM name
    auto const j_name = d.FindMember(CONFIG_KEY_NAME);
    if (j_name != d.MemberEnd()) {
        if (!j_name->value.IsString()) {
            LOGE("[root]->name: Not a string");
            return false;
        }
        name = j_name->value.GetString();
    }

    // App sharing
    auto const j_app_sharing = d.FindMember(CONFIG_KEY_APP_SHARING);
    if (j_app_sharing != d.MemberEnd()) {
        if (!j_app_sharing->value.IsObject()) {
            LOGE("[root]->app_sharing: Not an object");
            return false;
        }

        // Individual app sharing
        auto const j_individual = j_app_sharing->value.FindMember(
                CONFIG_KEY_INDIVIDUAL_APP_SHARING);
        if (j_individual != j_app_sharing->value.MemberEnd()) {
            if (!j_individual->value.IsBool()) {
                LOGE("[root]->app_sharing->individual: Not a boolean");
                return false;
            }
            indiv_app_sharing = j_individual->value.GetBool();
        }

        // Shared packages
        auto const j_pkgs = j_app_sharing->value.FindMember(
                CONFIG_KEY_PACKAGES);
        if (j_pkgs != j_app_sharing->value.MemberEnd()) {
            if (!j_pkgs->value.IsArray()) {
                LOGE("[root]->app_sharing->packages: Not an array");
                return false;
            }

            size_t i = 0;
            for (auto const &j_data : j_pkgs->value.GetArray()) {
                if (!j_data.IsObject()) {
                    LOGE("[root]->app_sharing->packages[%zu]: Not an object", i);
                    return false;
                }

                SharedPackage shared_pkg;

                // Package ID
                auto const j_pkg_id = j_data.FindMember(CONFIG_KEY_PACKAGE_ID);
                if (j_pkg_id != j_data.MemberEnd()) {
                    if (!j_pkg_id->value.IsString()) {
                        LOGE("[root]->app_sharing->packages[%zu]->pkg_id: Not a string", i);
                        return false;
                    }
                    shared_pkg.pkg_id = j_pkg_id->value.GetString();
                } else {
                    // Skip empty package names
                    continue;
                }

                // Shared data
                auto const j_share_data = j_data.FindMember(CONFIG_KEY_SHARE_DATA);
                if (j_share_data != j_data.MemberEnd()) {
                    if (!j_share_data->value.IsBool()) {
                        LOGE("[root]->app_sharing->packages[%zu]->share_data: Not a boolean", i);
                        return false;
                    }
                    shared_pkg.share_data = j_share_data->value.GetBool();
                }

                shared_pkgs.push_back(std::move(shared_pkg));

                ++i;
            }
        }
    }

    return true;
}

}
