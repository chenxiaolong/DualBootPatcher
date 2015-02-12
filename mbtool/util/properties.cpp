/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <vector>
#include <cstring>

#if __ANDROID_API__ >= 21
#include <dlfcn.h>
#endif

#include "util/logging.h"

namespace mb
{
namespace util
{

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

    int (*__system_property_get)(const char *, char *) =
            dlsym(handle, "__system_property_get");
#endif

    std::vector<char> value(MB_PROP_VALUE_MAX);
    int len = __system_property_get(name.c_str(), value.data());

#ifdef MB_LIBC_DEBUG
    dlclose(handle);
#endif

    if (len == 0) {
        *value_out = default_value;
    } else {
        value_out->assign(value.begin(), value.end());
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

    int (*__system_property_set)(const char *, const char *) =
            dlsym(handle, "__system_property_set");
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

}
}