/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/romconfig.h"

#include <cerrno>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>

#include "mblog/logging.h"

#define LOG_TAG "mbtool/util/romconfig"

#define KEY_ID                     "id"
#define KEY_NAME                   "name"
#define KEY_CACHED_PROPERTIES      "cached_properties"
#define KEY_APP_SHARING            "app_sharing"
#define KEY_INDIVIDUAL_APP_SHARING "individual"
#define KEY_PACKAGES               "packages"
#define KEY_PACKAGE_ID             "pkg_id"
#define KEY_SHARE_DATA             "share_data"

using namespace rapidjson;

namespace mb
{

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

/*
 * Example JSON structure:
 *
 * {
 *     "id": "primary",
 *     "name": "TouchWiz 7.0",
 *     "cached_properties": {
 *         "ro.build.version.release": "7.0",
 *         "ro.build.display.id": "NRD90M.G955FXXU1AQL5"
 *     },
 *     "app_sharing": {
 *         "individual": true
 *         "packages": [
 *             {
 *                 "pkg_id": "com.android.chrome",
 *                 "share_data": true
 *             },{
 *                 "pkg_id": "com.android.vending",
 *                 "share_data": true
 *             }
 *         ]
 *     }
 * }
 */

static inline std::string get_string(const Value &node)
{
    return {node.GetString(), node.GetStringLength()};
}

static bool load_app_sharing_packages(RomConfig &config, const Value &node)
{
    static constexpr char context[] = "." KEY_APP_SHARING "." KEY_PACKAGES;

    if (!node.IsArray()) {
        LOGE("%s: Not an array", context);
        return false;
    }

    size_t i = 0;
    for (auto const &item : node.GetArray()) {
        if (!item.IsObject()) {
            LOGE("%s[%zu]: Not an object", context, i);
            return false;
        }

        SharedPackage shared_pkg = {};

        for (auto const &pkg_item : item.GetObject()) {
            auto const &key = get_string(pkg_item.name);

            if (key == KEY_PACKAGE_ID) {
                if (!pkg_item.value.IsString()) {
                    LOGE("%s[%zu].%s: Not a string", context, i, key.c_str());
                    return false;
                }
                shared_pkg.pkg_id = get_string(pkg_item.value);
            } else if (key == KEY_SHARE_DATA) {
                if (!pkg_item.value.IsBool()) {
                    LOGE("%s[%zu].%s: Not a boolean", context, i, key.c_str());
                    return false;
                }
                shared_pkg.share_data = pkg_item.value.GetBool();
            } else {
                LOGW("%s[%zu].%s: Skipping unknown key", context, i, key.c_str());
            }
        }

        if (!shared_pkg.pkg_id.empty()) {
            config.shared_pkgs.push_back(std::move(shared_pkg));
        }

        ++i;
    }

    return true;
}

static bool load_app_sharing(RomConfig &config, const Value &node)
{
    static constexpr char context[] = "." KEY_APP_SHARING;

    if (!node.IsObject()) {
        LOGE("%s: Not an object", context);
        return false;
    }

    for (auto const &item : node.GetObject()) {
        auto const &key = get_string(item.name);

        if (key == KEY_INDIVIDUAL_APP_SHARING) {
            if (!item.value.IsBool()) {
                LOGE("%s.%s: Not a boolean", context, key.c_str());
                return false;
            }
            config.indiv_app_sharing = item.value.GetBool();
        } else if (key == KEY_PACKAGES) {
            if (!load_app_sharing_packages(config, node)) {
                return false;
            }
        } else {
            LOGW("%s.%s: Skipping unknown key", context, key.c_str());
        }
    }

    return true;
}

static bool load_cached_props(RomConfig &config, const Value &node)
{
    static constexpr char context[] = "." KEY_CACHED_PROPERTIES;

    if (!node.IsObject()) {
        LOGE("%s: Not an object", context);
        return false;
    }

    for (auto const &item : node.GetObject()) {
        auto const &key = get_string(item.name);

        if (!item.value.IsString()) {
            LOGE("%s.%s: Not a string", context, key.c_str());
            return false;
        }

        config.cached_props[key] = get_string(item.value);
    }

    return true;
}

static bool load_root(RomConfig &config, const Value &node)
{
    static constexpr char context[] = ".";

    if (!node.IsObject()) {
        LOGE("%s: Not an object", context);
        return false;
    }

    for (auto const &item : node.GetObject()) {
        auto const &key = get_string(item.name);

        if (key == KEY_ID) {
            if (!item.value.IsString()) {
                LOGE("%s.%s: Not a string", context, key.c_str());
                return false;
            }
            config.id = get_string(item.value);
        } else if (key == KEY_NAME) {
            if (!item.value.IsString()) {
                LOGE("%s.%s: Not a string", context, key.c_str());
                return false;
            }
            config.name = get_string(item.value);
        } else if (key == KEY_CACHED_PROPERTIES) {
            if (!load_cached_props(config, item.value)) {
                return false;
            }
        } else if (key == KEY_APP_SHARING) {
            if (!load_app_sharing(config, item.value)) {
                return false;
            }
        } else {
            LOGW("%s.%s: Skipping unknown key", context, key.c_str());
        }
    }

    return true;
}

bool RomConfig::load_file(const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "re"), &fclose);
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

    return load_root(*this, d);
}

bool RomConfig::save_file(const std::string &path)
{
    Document d;
    d.SetObject();

    auto &alloc = d.GetAllocator();

    if (!id.empty()) {
        d.AddMember(KEY_ID, id, alloc);
    }

    if (!name.empty()) {
        d.AddMember(KEY_NAME, name, alloc);
    }

    Value v_cached_props(kObjectType);

    for (auto const &cp : cached_props) {
        v_cached_props.AddMember(StringRef(cp.first), StringRef(cp.second),
                                 alloc);
    }

    if (!v_cached_props.ObjectEmpty()) {
        d.AddMember(KEY_CACHED_PROPERTIES, v_cached_props, alloc);
    }

    if (indiv_app_sharing || !shared_pkgs.empty()) {
        Value v_app_sharing(kObjectType);

        v_app_sharing.AddMember(KEY_INDIVIDUAL_APP_SHARING,
                                indiv_app_sharing, alloc);

        Value v_shared_pkgs(kArrayType);

        for (auto const &sp : shared_pkgs) {
            Value v_shared_pkg(kObjectType);

            v_shared_pkg.AddMember(KEY_PACKAGE_ID, sp.pkg_id, alloc);
            v_shared_pkg.AddMember(KEY_SHARE_DATA, sp.share_data, alloc);

            v_shared_pkgs.PushBack(v_shared_pkg, alloc);
        }

        if (!v_shared_pkgs.Empty()) {
            v_app_sharing.AddMember(KEY_PACKAGES, v_shared_pkgs, alloc);
        }

        d.AddMember(KEY_APP_SHARING, v_app_sharing, alloc);
    }

    ScopedFILE fp(fopen(path.c_str(), "we"), &fclose);
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    char buf[65536];
    FileWriteStream os(fp.get(), buf, sizeof(buf));
    Writer<FileWriteStream> writer(os);

    if (!d.Accept(writer)) {
        return false;
    }

    if (fclose(fp.release()) != 0) {
        LOGE("%s: Failed to close file: %s",
             path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

}
