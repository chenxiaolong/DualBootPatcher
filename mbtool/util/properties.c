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

#if __ANDROID_API__ >= 21
#include <dlfcn.h>
#endif
#include <string.h>

#include "util/logging.h"

int mb_get_property(const char *name, char *value_out, char *default_value)
{
#if 0 && __ANDROID_API__ >= 21
    // __system_property_get is hidden on Android 5.0 for arm64-v8a and x86_64
    // I can't seem to find anything explaining the reason... oh well.

    // NOTE: This is disabled for now since we always link libc statically

    void *handle = dlopen("libc.so", RTLD_NOLOAD);
    if (!handle) {
        LOGE("Failed to dlopen() libc.so");
        return 0;
    }

    int (*__system_property_get)(const char *, char *) =
            dlsym(handle, "__system_property_get");
#endif

    int len = __system_property_get(name, value_out);

#if 0 && __ANDROID_API__ >= 21
    dlclose(handle);
#endif

    if (len == 0) {
        if (default_value) {
            return (int) strlcpy(value_out, default_value, MB_PROP_VALUE_MAX);
        } else {
            return 0;
        }
    } else {
        return len;
    }
}

int mb_set_property(const char *name, const char *value)
{
    return __system_property_set(name, value);
}
