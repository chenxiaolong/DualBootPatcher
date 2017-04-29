/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/properties.h"

#include <memory>
#include <mutex>
#include <vector>

#include <cstdio>
#include <cstring>

#include "mbcommon/common.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/finally.h"
#include "mbutil/string.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include "mbutil/external/_system_properties.h"

typedef std::unique_ptr<FILE, decltype(fclose) *> ScopedFILE;

namespace mb
{
namespace util
{

static bool initialized = false;
static std::mutex initialized_lock;

void initialize_properties()
{
    std::lock_guard<std::mutex> lock(initialized_lock);

    if (!initialized) {
        mb__system_properties_init();
        initialized = true;
    }
}

int libc_system_property_set(const char *key, const char *value)
{
    initialize_properties();

    return mb__system_property_set(key, value);
}

const prop_info *libc_system_property_find(const char *name)
{
    initialize_properties();

    return mb__system_property_find(name);
}

void libc_system_property_read_callback(
        const prop_info *pi,
        void (*callback)(void *cookie, const char *name, const char *value,
                         uint32_t serial),
        void *cookie)
{
    initialize_properties();

    return mb__system_property_read_callback(pi, callback, cookie);
}

int libc_system_property_foreach(
        void (*propfn)(const prop_info *pi, void *cookie),
        void *cookie)
{
    initialize_properties();

    return mb__system_property_foreach(propfn, cookie);
}

bool libc_system_property_wait(const prop_info *pi,
                               uint32_t old_serial,
                               uint32_t *new_serial_ptr,
                               const struct timespec *relative_timeout)
{
    initialize_properties();

    return mb__system_property_wait(pi, old_serial, new_serial_ptr,
                                    relative_timeout);
}

// Helper functions

static bool string_to_bool(const std::string &str, bool &value_out)
{
    if (str == "1"
            || str == "y"
            || str == "yes"
            || str == "true"
            || str == "on") {
        value_out = true;
        return true;
    } else if (str == "0"
            || str == "n"
            || str == "no"
            || str == "false"
            || str == "off") {
        value_out = false;
        return true;
    }

    return false;
}

static void read_property(const prop_info *pi, std::string &name_out,
                          std::string &value_out)
{
    using KeyValuePair = std::pair<std::string, std::string>;
    KeyValuePair ctx;

    libc_system_property_read_callback(
            pi, [](void *cookie, const char *name, const char *value,
                   uint32_t serial) {
        (void) serial;
        auto *kvp = static_cast<KeyValuePair *>(cookie);
        kvp->first = name;
        kvp->second = value;
    }, &ctx);

    name_out = std::move(ctx.first);
    value_out = std::move(ctx.second);
}

bool property_get(const std::string &key, std::string &value_out)
{
    const prop_info *pi = libc_system_property_find(key.c_str());
    if (!pi) {
        return false;
    }

    std::string name;
    read_property(pi, name, value_out);
    return true;
}

std::string property_get_string(const std::string &key,
                                const std::string &default_value)
{
    std::string value;

    if (property_get(key, value) && !value.empty()) {
        return value;
    }

    return default_value;
}

bool property_get_bool(const std::string &key, bool default_value)
{
    std::string value;
    bool result;

    if (property_get(key, value) && string_to_bool(value, result)) {
        return result;
    }

    return default_value;
}

bool property_set(const std::string &key, const std::string &value)
{
    return libc_system_property_set(key.c_str(), value.c_str()) == 0;
}

bool property_set_direct(const std::string &key, const std::string &value)
{
    initialize_properties();

    prop_info *pi = const_cast<prop_info *>(
            mb__system_property_find(key.c_str()));
    int ret;

    if (pi) {
        ret = mb__system_property_update(pi, value.c_str(), value.size());
    } else {
        ret = mb__system_property_add(key.c_str(), key.size(), value.c_str(),
                                      value.size());
    }

    return ret == 0;
}

bool property_list(PropertyListCb prop_fn, void *cookie)
{
    struct PropListCtx
    {
        PropertyListCb prop_fn;
        void *cookie;
    };

    PropListCtx ctx{ prop_fn, cookie };

    return libc_system_property_foreach(
            [](const prop_info *pi, void *cookie) {
        auto *ctx = static_cast<PropListCtx *>(cookie);
        std::string name;
        std::string value;

        read_property(pi, name, value);
        ctx->prop_fn(name, value, ctx->cookie);
    }, &ctx);
}

bool property_get_all(std::unordered_map<std::string, std::string> &map)
{
    return libc_system_property_foreach(
            [](const prop_info *pi, void *cookie) {
        auto *map = static_cast<std::unordered_map<std::string, std::string> *>(cookie);
        std::string name;
        std::string value;

        read_property(pi, name, value);
        (*map)[name] = value;
    }, &map);
}

// Properties file functions

enum class PropIterAction
{
    CONTINUE,
    STOP,
};

typedef PropIterAction (*PropIterCb)(const std::string &key,
                                     const std::string &value,
                                     void *cookie);

static bool iterate_property_file(const std::string &path, PropIterCb cb,
                                  void *cookie)
{
    ScopedFILE fp(fopen(path.c_str(), "r"), &fclose);
    if (!fp) {
        return false;
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t read;

    auto free_line = finally([&] {
        free(line);
    });

    while ((read = getline(&line, &len, fp.get())) >= 0) {
        if (read == 0 || line[0] == '#') {
            // Skip empty and comment lines
            continue;
        }

        char *equals = strchr(line, '=');
        if (!equals) {
            // No equals in line
            continue;
        }

        *equals = '\0';

        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }

        if (cb(line, equals + 1, cookie) == PropIterAction::STOP) {
            return true;
        }
    }

    return !ferror(fp.get());
}

bool property_file_get(const std::string &path, const std::string &key,
                       std::string &value_out)
{
    struct Ctx
    {
        const std::string *key;
        std::string *value_out;
        bool found;
    };

    Ctx ctx{&key, &value_out, false};

    bool ret = iterate_property_file(
            path, [](const std::string &key, const std::string &value,
                     void *cookie){
        auto *ctx = static_cast<Ctx *>(cookie);
        if (key == *ctx->key) {
            *ctx->value_out = value;
            ctx->found = true;
            return PropIterAction::STOP;
        }
        return PropIterAction::CONTINUE;
    }, &ctx);

    if (!ctx.found) {
        value_out.clear();
    }

    return ret;
}

std::string property_file_get_string(const std::string &path,
                                     const std::string &key,
                                     const std::string &default_value)
{
    std::string value;

    if (property_file_get(path, key, value) && !value.empty()) {
        return value;
    }

    return default_value;
}

bool property_file_get_bool(const std::string &path, const std::string &key,
                            bool default_value)
{
    std::string value;
    bool result;

    if (property_file_get(path, key, value) && string_to_bool(value, result)) {
        return result;
    }

    return default_value;
}

bool property_file_list(const std::string &path, PropertyListCb prop_fn,
                        void *cookie)
{
    struct Ctx
    {
        PropertyListCb prop_fn;
        void *cookie;
    };

    Ctx ctx{prop_fn, cookie};

    return iterate_property_file(
            path, [](const std::string &key, const std::string &value,
                     void *cookie){
        auto *ctx = static_cast<Ctx *>(cookie);
        ctx->prop_fn(key, value, ctx->cookie);
        return PropIterAction::CONTINUE;
    }, &ctx);
}

bool property_file_get_all(const std::string &path,
                           std::unordered_map<std::string, std::string> &map)
{
    return property_file_list(path,
            [](const std::string &key, const std::string &value, void *cookie) {
        auto *map = static_cast<std::unordered_map<std::string, std::string> *>(cookie);
        (*map)[key] = value;
    }, &map);
}

bool property_file_write_all(const std::string &path,
                             const std::unordered_map<std::string, std::string> &map)
{
    ScopedFILE fp(fopen(path.c_str(), "wb"), fclose);
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
}
