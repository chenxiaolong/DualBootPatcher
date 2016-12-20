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

#include "mbutil/properties.h"

#include <memory>
#include <vector>
#include <cstdio>
#include <cstring>

#include <pthread.h>
#include <sys/stat.h>

#if __ANDROID_API__ >= 21
#include <dlfcn.h>
#endif

#include "mblog/logging.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/finally.h"
#include "mbutil/string.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include "mbutil/external/_system_properties.h"

#if !DYNAMICALLY_LINKED
#define __system_property_get       mb__system_property_get
#define __system_property_set       mb__system_property_set
#define __system_property_find      mb__system_property_find
#define __system_property_read      mb__system_property_read
#define __system_property_find_nth  mb__system_property_find_nth
#define __system_property_foreach   mb__system_property_foreach
#endif

namespace mb
{
namespace util
{

static bool initialized = false;
static pthread_mutex_t initialized_lock = PTHREAD_MUTEX_INITIALIZER;

#if DYNAMICALLY_LINKED
static const char *libc_path = nullptr;
static void *libc_handle = nullptr;
static void *SYMBOL__system_property_get = nullptr;
static void *SYMBOL__system_property_set = nullptr;
static void *SYMBOL__system_property_find = nullptr;
static void *SYMBOL__system_property_read = nullptr;
static void *SYMBOL__system_property_find_nth = nullptr;
static void *SYMBOL__system_property_foreach = nullptr;

static void detect_libc(void)
{
    if (libc_path) {
        return;
    }

    struct stat sb;
    if (stat("/sbin/recovery", &sb) == 0) {
        if (stat("/sbin/libc.so", &sb) == 0) {
            // TWRP
            libc_path = "/sbin/libc.so";
        } else {
            // Custom libc.so if recovery is statically linked
            libc_path = "/tmp/libc.so";
        }
    } else {
        libc_path = "/system/lib/libc.so";
    }
}

static void *get_libc_handle(void)
{
    detect_libc();

    void *handle = dlopen(libc_path, RTLD_LOCAL);
    if (!handle) {
        LOGE("%s: Failed to dlopen: %s", libc_path, dlerror());
    }

    return handle;
}

static void *get_libc_symbol(const char *name)
{
    if (!libc_handle) {
        return nullptr;
    }

    void *symbol = dlsym(libc_handle, name);
    if (!symbol) {
        LOGE("Failed to dlsym %s: %s", name, dlerror());
    }

    return symbol;
}

static void dlopen_libc(void)
{
    if (libc_handle) {
        return;
    }

    libc_handle = get_libc_handle();
    if (!libc_handle) {
        return;
    }

    SYMBOL__system_property_get = get_libc_symbol("__system_property_get");
    SYMBOL__system_property_set = get_libc_symbol("__system_property_set");
    SYMBOL__system_property_find = get_libc_symbol("__system_property_find");
    SYMBOL__system_property_read = get_libc_symbol("__system_property_read");
    SYMBOL__system_property_find_nth = get_libc_symbol("__system_property_find_nth");
    SYMBOL__system_property_foreach = get_libc_symbol("__system_property_foreach");
}

__attribute__((unused))
static void dlclose_libc(void)
{
    if (libc_handle) {
        dlclose(libc_handle);
        libc_handle = nullptr;
    }
}
#else
void initialize_properties()
{
    pthread_mutex_lock(&initialized_lock);
    if (!initialized) {
        mb__system_properties_init();
        initialized = true;
    }
    pthread_mutex_unlock(&initialized_lock);
}
#endif

int libc_system_property_get(const char *name, char *value)
{
#if DYNAMICALLY_LINKED
    dlopen_libc();
    if (!SYMBOL__system_property_get) {
        value[0] = '\0';
        return 0;
    }

    auto __system_property_get =
            reinterpret_cast<int (*)(const char *, char *)>(
                    SYMBOL__system_property_get);
#else
    initialize_properties();
#endif

    return __system_property_get(name, value);
}

int libc_system_property_set(const char *key, const char *value)
{
#if DYNAMICALLY_LINKED
    dlopen_libc();
    if (!SYMBOL__system_property_set) {
        return -1;
    }

    auto __system_property_set =
            reinterpret_cast<int (*)(const char *, const char *)>(
                    SYMBOL__system_property_set);
#else
    initialize_properties();
#endif

    return __system_property_set(key, value);
}

const prop_info *libc_system_property_find(const char *name)
{
#if DYNAMICALLY_LINKED
    dlopen_libc();
    if (!SYMBOL__system_property_find) {
        return nullptr;
    }

    auto __system_property_find =
            reinterpret_cast<const prop_info * (*)(const char *)>(
                    SYMBOL__system_property_find);
#else
    initialize_properties();
#endif

    return __system_property_find(name);
}

int libc_system_property_read(const prop_info *pi, char *name, char *value)
{
#if DYNAMICALLY_LINKED
    dlopen_libc();
    if (!SYMBOL__system_property_read) {
        name[0] = '\0';
        value[0] = '\0';
        return 0;
    }

    auto __system_property_read =
            reinterpret_cast<int (*)(const prop_info *, char *, char *)>(
                    SYMBOL__system_property_read);
#else
    initialize_properties();
#endif

    return __system_property_read(pi, name, value);
}

const prop_info *libc_system_property_find_nth(unsigned n)
{
#if DYNAMICALLY_LINKED
    dlopen_libc();
    if (!SYMBOL__system_property_find_nth) {
        return nullptr;
    }

    auto __system_property_find_nth =
            reinterpret_cast<const prop_info * (*)(unsigned)>(
                    SYMBOL__system_property_find_nth);
#else
    initialize_properties();
#endif

    return __system_property_find_nth(n);
}

int libc_system_property_foreach(
        void (*propfn)(const prop_info *pi, void *cookie),
        void *cookie)
{
#if DYNAMICALLY_LINKED
    dlopen_libc();
    if (!SYMBOL__system_property_foreach) {
        return -1;
    }

    auto __system_property_foreach =
            reinterpret_cast<int (*)(void (*)(const prop_info *, void *), void*)>(
                    SYMBOL__system_property_foreach);
#else
    initialize_properties();
#endif

    return __system_property_foreach(propfn, cookie);
}

// BEGIN: Code partially based on libcutils properties.c

int property_get(const char *key, char *value_out, const char *default_value)
{
    int n = libc_system_property_get(key, value_out);

    if (n > 0) {
        return n;
    } else {
        n = strlen(default_value);
        if (n >= PROP_VALUE_MAX) {
            n = PROP_VALUE_MAX - 1;
        }
        memcpy(value_out, default_value, n);
        value_out[n] = '\0';
    }

    return n;
}

int property_set(const char *key, const char *value)
{
    return libc_system_property_set(key, value);
}

bool property_get_bool(const char *key, bool default_value)
{
    char buf[PROP_VALUE_MAX];
    bool result = default_value;

    int n = property_get(key, buf, "");
    if (n == 1) {
        if (buf[0] == '0' || buf[0] == 'n') {
            result = false;
        } else if (buf[0] == '1' || buf[0] == 'y') {
            result = true;
        }
    } else if (n > 1) {
         if (strcmp(buf, "no") == 0
                || strcmp(buf, "false") == 0
                || strcmp(buf, "off") == 0) {
            result = false;
        } else if (strcmp(buf, "yes") == 0
                || strcmp(buf, "true") == 0
                || strcmp(buf, "on") == 0) {
            result = true;
        }
    }

    return result;
}

struct property_list_cb_data
{
    property_list_cb propfn;
    void *cookie;
};

static void property_list_callback(const prop_info *pi, void *cookie)
{
    char name[PROP_NAME_MAX];
    char value[PROP_VALUE_MAX];
    property_list_cb_data *data = static_cast<property_list_cb_data *>(cookie);

    libc_system_property_read(pi, name, value);
    data->propfn(name, value, data->cookie);
}

int property_list(property_list_cb propfn, void *cookie)
{
    property_list_cb_data data = { propfn, cookie };
    return libc_system_property_foreach(property_list_callback, &data);
}

// END: Code partially based on libcutils properties.c

static void _add_property(const prop_info *pi, void *cookie)
{
    char name[PROP_NAME_MAX];
    char value[PROP_VALUE_MAX];
    std::unordered_map<std::string, std::string> *props =
            static_cast<std::unordered_map<std::string, std::string> *>(cookie);

    if (libc_system_property_read(pi, name, value) >= 0) {
        (*props)[name] = value;
    }
}

bool get_all_properties(std::unordered_map<std::string, std::string> *map)
{
    std::unordered_map<std::string, std::string> props;
    libc_system_property_foreach(&_add_property, &props);
    map->swap(props);
    return true;
}

bool file_get_property(const std::string &path,
                       const std::string &key,
                       std::string *out,
                       const std::string &default_value)
{
    autoclose::file fp(autoclose::fopen(path.c_str(), "r"));
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
    autoclose::file fp(autoclose::fopen(path.c_str(), "r"));
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
    autoclose::file fp(autoclose::fopen(path.c_str(), "wb"));
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
