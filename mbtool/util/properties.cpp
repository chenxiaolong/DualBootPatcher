/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>
#include <vector>
#include <cstdio>
#include <cstring>

#if __ANDROID_API__ >= 21
#include <dlfcn.h>
#endif

#include "util/finally.h"
#include "util/logging.h"
#include "util/string.h"

namespace mb
{
namespace util
{

typedef std::unique_ptr<std::FILE, int (*)(std::FILE*)> file_ptr;

#ifdef MB_LIBC_DEBUG
static const char *LIBC = nullptr;

static void detect_libc(void)
{
    if (LIBC) {
        return;
    }

    struct stat sb;
    if (stat("/sbin/recovery", &sb) == 0) {
        if (stat("/sbin/libc.so", &sb) == 0) {
            // TWRP
            LIBC = "/sbin/libc.so";
        } else {
            // Custom libc.so if recovery is statically linked
            LIBC = "/tmp/libc.so";
        }
    } else {
        LIBC = "/system/lib/libc.so";
    }
}
#endif

void get_property(const std::string &name,
                  std::string *value_out,
                  const std::string &default_value)
{
#ifdef MB_LIBC_DEBUG
    // __system_property_get is hidden on Android 5.0 for arm64-v8a and x86_64
    // I can't seem to find anything explaining the reason... oh well.

    // NOTE: This is disabled for now since we always link libc statically

    detect_libc();

    void *handle = dlopen(LIBC, RTLD_LOCAL);
    if (!handle) {
        LOGE("Failed to dlopen() %s: %s", LIBC, dlerror());
        return;
    }

    auto __system_property_get =
            reinterpret_cast<int (*)(const char *, char *)>(
                    dlsym(handle, "__system_property_get"));
#endif

    std::vector<char> value(MB_PROP_VALUE_MAX);
    int len = __system_property_get(name.c_str(), value.data());

#ifdef MB_LIBC_DEBUG
    dlclose(handle);
#endif

    if (len == 0) {
        *value_out = default_value;
    } else {
        *value_out = value.data();
    }
}

bool set_property(const std::string &name,
                  const std::string &value)
{
#ifdef MB_LIBC_DEBUG
    detect_libc();

    void *handle = dlopen(LIBC, RTLD_LOCAL);
    if (!handle) {
        LOGE("Failed to dlopen() %s: %s", LIBC, dlerror());
        return false;
    }

    auto __system_property_set =
            reinterpret_cast<int (*)(const char *, const char *)>(
                    dlsym(handle, "__system_property_set"));
#endif

    if (name.size() >= MB_PROP_NAME_MAX - 1) {
        return false;
    }
    if (value.size() >= MB_PROP_VALUE_MAX - 1) {
        return false;
    }

    int ret = __system_property_set(name.c_str(), value.c_str());

#ifdef MB_LIBC_DEBUG
    dlclose(handle);
#endif

    return ret == 0;
}

bool file_get_property(const std::string &path,
                       const std::string &key,
                       std::string *out,
                       const std::string &default_value)
{
    file_ptr fp(std::fopen(path.c_str(), "r"), std::fclose);
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
        if (line[0] == '\0' || line[0] == '#') {
            // Skip empty and comment lines
            continue;
        }

        char *equals = strchr(line, '=');
        if (!equals) {
            // No equals in line
            continue;
        }

        if ((size_t)(equals - line) != key.size()) {
            // Key is not the same length
            continue;
        }

        if (starts_with(line, key)) {
            // Strip newline
            if (line[read - 1] == '\n') {
                line[read - 1] = '\0';
                --read;
            }

            out->assign(equals + 1);
            return true;
        }
    }

    *out = default_value;
    return true;
}

bool file_get_all_properties(const std::string &path,
                             std::unordered_map<std::string, std::string> *map)
{
    file_ptr fp(std::fopen(path.c_str(), "r"), std::fclose);
    if (!fp) {
        return false;
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t read;

    auto free_line = finally([&] {
        free(line);
    });

    std::unordered_map<std::string, std::string> tempMap;

    while ((read = getline(&line, &len, fp.get())) >= 0) {
        if (line[0] == '\0' || line[0] == '#') {
            // Skip empty and comment lines
            continue;
        }

        char *equals = strchr(line, '=');
        if (!equals) {
            // No equals in line
            continue;
        }

        // Strip newline
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
            --read;
        }

        *equals = '\0';
        tempMap[line] = equals + 1;
    }

    map->swap(tempMap);
    return true;
}

bool file_write_properties(const std::string &path,
                           const std::unordered_map<std::string, std::string> &map)
{
    file_ptr fp(std::fopen(path.c_str(), "wb"), std::fclose);
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
