/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/properties.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>

#include <cstdio>
#include <cstring>

#include <sys/stat.h>

#include "mbcommon/common.h"
#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mbutil/string.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include "mbutil/external/_system_properties.h"
#include "mbutil/external/system_properties.h"


typedef std::unique_ptr<FILE, decltype(fclose) *> ScopedFILE;

namespace mb::util
{

static bool initialized = false;
static std::mutex initialized_lock;

static void initialize_properties()
{
    std::lock_guard<std::mutex> lock(initialized_lock);

    if (!initialized) {
        __system_properties_init();
        initialized = true;
    }
}

// Helper functions

static std::optional<bool> string_to_bool(const std::string_view &str)
{
    if (str == "1"
            || str == "y"
            || str == "yes"
            || str == "true"
            || str == "on") {
        return true;
    } else if (str == "0"
            || str == "n"
            || str == "no"
            || str == "false"
            || str == "off") {
        return false;
    }

    return std::nullopt;
}

static PropertyIterAction read_property_cb(const prop_info *pi,
                                           const std::function<PropertyIterCb> &fn)
{
    struct Ctx
    {
        const std::function<PropertyIterCb> *fn;
        PropertyIterAction result;
    };

    Ctx ctx{&fn, PropertyIterAction::Stop};

    // Assume properties are already initialized if there's a prop_info object
    __system_property_read_callback(
            pi, [](void *cookie, const char *name, const char *value,
                   uint32_t serial) {
        (void) serial;
        auto *ctx_ = static_cast<Ctx *>(cookie);

        ctx_->result = (*ctx_->fn)(name, value);
    }, &ctx);

    return ctx.result;
}

std::optional<std::string> property_get(const std::string &key)
{
    initialize_properties();

    const prop_info *pi = __system_property_find(key.c_str());
    if (!pi) {
        return std::nullopt;
    }

    std::string result;

    read_property_cb(pi, [&](std::string_view key_, std::string_view value) {
        (void) key_;
        result = value;
        return PropertyIterAction::Stop;
    });

    return std::move(result);
}

std::string property_get_string(const std::string &key,
                                const std::string &default_value)
{
    if (auto value = property_get(key); value && !value->empty()) {
        return std::move(*value);
    }

    return default_value;
}

bool property_get_bool(const std::string &key, bool default_value)
{
    if (auto value = property_get(key)) {
        if (auto result = string_to_bool(*value)) {
            return *result;
        }
    }

    return default_value;
}

bool property_set(const std::string &key, const std::string &value)
{
    initialize_properties();

    return __system_property_set(key.c_str(), value.c_str()) == 0;
}

bool property_set_direct(const std::string &key, const std::string &value)
{
    initialize_properties();

    prop_info *pi = const_cast<prop_info *>(
            __system_property_find(key.c_str()));
    int ret;

    if (pi) {
        ret = __system_property_update(pi, value.c_str(),
                                       static_cast<unsigned int>(value.size()));
    } else {
        ret = __system_property_add(key.c_str(),
                                    static_cast<unsigned int>(key.size()),
                                    value.c_str(),
                                    static_cast<unsigned int>(value.size()));
    }

    return ret == 0;
}

bool property_iter(const std::function<PropertyIterCb> &fn)
{
    struct Ctx
    {
        const std::function<PropertyIterCb> *fn;
        bool done;
    };

    Ctx ctx{&fn, false};

    initialize_properties();

    return __system_property_foreach(
            [](const prop_info *pi, void *cookie) {
        auto *ctx_ = static_cast<Ctx *>(cookie);

        // The bionic properties doesn't have a way to stop iterating, so we
        // simulate it
        if (!ctx_->done && read_property_cb(pi, *ctx_->fn)
                == PropertyIterAction::Stop) {
            ctx_->done = true;
        }
    }, &ctx);
}

std::optional<PropertiesMap> property_get_all()
{
    PropertiesMap result;

    initialize_properties();

    if (__system_property_foreach([](const prop_info *pi, void *cookie) {
        auto *map = static_cast<PropertiesMap *>(cookie);

        read_property_cb(pi, [&](std::string_view key, std::string_view value) {
            map->insert_or_assign(std::string{key}, std::string{value});
            return PropertyIterAction::Stop;
        });
    }, &result)) {
        return std::move(result);
    } else {
        return std::nullopt;
    }
}

// Properties file functions

// Same as boost
template <class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
    seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct PairHash
{
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &val) const
    {
        size_t seed = 0;
        hash_combine(seed, val.first);
        hash_combine(seed, val.second);
        return seed;
    }
};

using DevInode = std::pair<dev_t, ino_t>;
using DevInodeSet = std::unordered_set<DevInode, PairHash>;

static std::string_view trim_whitespace(std::string_view sv)
{
    while (!sv.empty() && isspace(sv.front())) {
        sv.remove_prefix(1);
    }
    while (!sv.empty() && isspace(sv.back())) {
        sv.remove_suffix(1);
    }
    return sv;
}

static bool property_file_iter_impl(const std::string &path,
                                    std::string_view filter,
                                    const std::function<PropertyIterCb> &cb,
                                    DevInodeSet &seen_files)
{
    ScopedFILE fp(fopen(path.c_str(), "re"), &fclose);
    if (!fp) {
        return false;
    }

    struct stat sb;
    if (fstat(fileno(fp.get()), &sb) < 0) {
        return false;
    }

    // Ignore duplicate imports
    DevInode di{sb.st_dev, sb.st_ino};
    if (seen_files.find(di) != seen_files.end()) {
        return true;
    }

    seen_files.emplace(di);

    char *line = nullptr;
    size_t len = 0;
    ssize_t read;

    auto free_line = finally([&] {
        free(line);
    });

    while ((read = getline(&line, &len, fp.get())) >= 0) {
        std::string_view sv(line, static_cast<size_t>(read));

        sv = trim_whitespace(sv);

        // Skip empty and comment lines
        if (sv.empty() || sv.front() == '#') {
            continue;
        }

        if (starts_with(sv, "import ")) {
            sv.remove_prefix(7);
            sv = trim_whitespace(sv);

            std::string_view new_filter;

            if (auto space = sv.find(' '); space != std::string_view::npos) {
                new_filter = trim_whitespace(sv.substr(space + 1));
                sv = sv.substr(0, space);
            }

            if (!property_file_iter_impl(std::string(sv), new_filter, cb,
                                         seen_files) && errno != ENOENT) {
                // Missing files are OK
                // (follows the AOSP implementation behavior)
                return false;
            }
        } else {
            auto equals = sv.find('=');
            if (equals == std::string_view::npos) {
                continue;
            }

            auto key = trim_whitespace(sv.substr(0, equals));
            auto value = trim_whitespace(sv.substr(equals + 1));

            if (!filter.empty()) {
                if (filter.back() == '*') {
                    if (!starts_with(key, filter.substr(0, filter.size() - 1))) {
                        continue;
                    }
                } else {
                    if (key != filter) {
                        continue;
                    }
                }
            }

            if (cb(key, value) == PropertyIterAction::Stop) {
                return true;
            }
        }
    }

    return !ferror(fp.get());
}

std::optional<std::string> property_file_get(const std::string &path,
                                             const std::string &key)
{
    std::optional<std::string> value;

    property_file_iter(
            path, {}, [&](std::string_view key_, std::string_view value_) {
        if (key == key_) {
            value = value_;
            return PropertyIterAction::Stop;
        }
        return PropertyIterAction::Continue;
    });

    return value;
}

std::string property_file_get_string(const std::string &path,
                                     const std::string &key,
                                     const std::string &default_value)
{
    if (auto value = property_file_get(path, key); value && !value->empty()) {
        return std::move(*value);
    }

    return default_value;
}

bool property_file_get_bool(const std::string &path, const std::string &key,
                            bool default_value)
{
    if (auto value = property_file_get(path, key)) {
        if (auto result = string_to_bool(*value)) {
            return *result;
        }
    }

    return default_value;
}

bool property_file_iter(const std::string &path, std::string_view filter,
                        const std::function<PropertyIterCb> &fn)
{
    DevInodeSet seen_files;
    return property_file_iter_impl(path, filter, fn, seen_files);
}

std::optional<PropertiesMap> property_file_get_all(const std::string &path)
{
    PropertiesMap result;

    if (property_file_iter(path, {},
            [&](std::string_view key, std::string_view value) {
        result.insert_or_assign(std::string{key}, std::string{value});
        return PropertyIterAction::Continue;
    })) {
        return std::move(result);
    } else {
        return std::nullopt;
    }
}

bool property_file_write_all(const std::string &path, const PropertiesMap &map)
{
    ScopedFILE fp(fopen(path.c_str(), "wbe"), fclose);
    if (!fp) {
        return false;
    }

    for (auto const &pair : map) {
        if (fputs(pair.first.c_str(), fp.get()) == EOF
                || fputc('=', fp.get()) == EOF
                || fputs(pair.second.c_str(), fp.get()) == EOF
                || fputc('\n', fp.get()) == EOF) {
            return false;
        }
    }

    return true;
}

}
