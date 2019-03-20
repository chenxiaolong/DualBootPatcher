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

#include "mbcommon/type_traits.h"

#include "mblog/logging.h"

#define LOG_TAG "mbtool/util/romconfig"

#define KEY_ID                "id"
#define KEY_NAME              "name"
#define KEY_CACHED_PROPERTIES "cached_properties"

using namespace rapidjson;

namespace mb
{

using ScopedFILE = std::unique_ptr<FILE, TypeFn<fclose>>;

/*
 * Example JSON structure:
 *
 * {
 *     "id": "primary",
 *     "name": "TouchWiz 7.0",
 *     "cached_properties": {
 *         "ro.build.version.release": "7.0",
 *         "ro.build.display.id": "NRD90M.G955FXXU1AQL5"
 *     }
 * }
 */

static inline std::string get_string(const Value &node)
{
    return {node.GetString(), node.GetStringLength()};
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
        } else {
            LOGW("%s.%s: Skipping unknown key", context, key.c_str());
        }
    }

    return true;
}

bool RomConfig::load_file(const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "re"));
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

    ScopedFILE fp(fopen(path.c_str(), "we"));
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
