/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/config.h"

// #include <cerrno>
// #include <cstring>
//
// #include <sys/mount.h>
// #include <sys/stat.h>
// #include <sys/sysmacros.h>
// #include <unistd.h>
//
// #include "mbcommon/finally.h"
//
#include "mblog/logging.h"

#include "mbutil/path.h"
#include "mbutil/properties.h"

#define LOG_TAG             "mbtool/util/config"

#define DBP_SHIM_PROPS      "/dbp.shim.prop"
#define PROP_UEVENT_PATH    "dbp.shim.uevent_path"
#define PROP_ARCHIVE_PATH   "dbp.shim.archive_path"

namespace mb
{

//! Return sanitized uevent path or std::nullopt if it's unsafe or invalid
static std::optional<std::string>
sanitized_uevent_path(const std::string &path)
{
    auto pieces = util::path_split(path);
    util::normalize_path(pieces);

    // Must be a block device uevent file in sysfs
    if (pieces.size() < 4 || !pieces[0].empty() || pieces[1] != "sys"
            || pieces[2] != "block" || pieces.back() != "uevent") {
        return std::nullopt;
    }

    return util::path_join(pieces);
}

//! Return sanitized archive path or std::nullopt if it's unsafe or invalid
static std::optional<std::string>
sanitized_archive_path(const std::string &path)
{
    auto pieces = util::path_split(path);
    util::normalize_path(pieces);

    // Must not be absolute or begin with '..'
    if (pieces.empty() || pieces.front().empty() || pieces.front() == "..") {
        return std::nullopt;
    }

    return util::path_join(pieces);
}

//! Load DBP_SHIM_PROPS
std::optional<ShimConfig> load_shim_config()
{
    ShimConfig config;

    if (!util::property_file_iter(DBP_SHIM_PROPS, {},
            [&](std::string_view key, std::string_view value) {
        std::string str_value(value);

        if (key == PROP_UEVENT_PATH) {
            config.uevent_path = value;
        } else if (key == PROP_ARCHIVE_PATH) {
            config.archive_path = value;
        } else {
            LOGD("%s: Ignoring unknown key '%s'",
                 DBP_SHIM_PROPS, std::string(key).c_str());
        }
        return util::PropertyIterAction::Continue;
    })) {
        LOGE("%s: Failed to load properties", DBP_SHIM_PROPS);
        return std::nullopt;
    }

    if (auto p = sanitized_uevent_path(config.uevent_path)) {
        config.uevent_path.swap(*p);
    } else {
        LOGE("%s: Invalid '%s' path: '%s'",
             DBP_SHIM_PROPS, PROP_UEVENT_PATH, config.uevent_path.c_str());
        return std::nullopt;
    }

    if (auto p = sanitized_archive_path(config.archive_path)) {
        config.archive_path.swap(*p);
    } else {
        LOGE("%s: Invalid '%s' path: '%s'",
             DBP_SHIM_PROPS, PROP_ARCHIVE_PATH, config.archive_path.c_str());
        return std::nullopt;
    }

    return config;
}

}
