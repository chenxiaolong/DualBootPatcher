/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "boot/properties.h"

#include <string>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <getopt.h>

#include "mbcommon/string.h"
#include "mbutil/properties.h"


namespace mb
{

static void print_prop(std::string_view key, std::string_view value)
{
    if (!key.empty()) {
        fwrite(key.data(), 1, key.size(), stdout);
        fputc('=', stdout);
    }
    fwrite(value.data(), 1, value.size(), stdout);
    fputc('\n', stdout);
}

static bool set_if_possible(const std::string &key, const std::string &value,
                            bool force)
{
    bool ret;

    if (force) {
        ret = util::property_set_direct(key, value);
    } else {
        if (auto r = util::property_get(key); r && starts_with(key, "ro.")) {
            fprintf(stderr, "Cannot overwrite read-only property '%s'"
                    " without -f/--force\n", key.c_str());
            return false;
        }

        ret = util::property_set(key, value);
    }

    if (!ret) {
        fprintf(stderr, "Failed to set property '%s'='%s'\n",
                key.c_str(), value.c_str());
        return false;
    }

    return true;
}

static bool load_prop_file(const char *path, bool force)
{
    bool ret = true;

    if (!util::property_file_iter(path, {}, [&](std::string_view key,
                                                std::string_view value) {
        if (!set_if_possible(std::string(key), std::string(value), force)) {
            ret = false;
        }

        return util::PropertyIterAction::Continue;
    })) {
        fprintf(stderr, "%s: Failed to load properties file\n", path);
        return false;
    }

    return ret;
}

static void properties_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: properties get <property>\n"
            "   OR: properties set <property> <value> [--force]\n"
            "   OR: properties set-file <file> [--force]\n"
            "\n"
            "Options:\n"
            "  -f, --force     Allow overwriting read-only properties\n");
}

int properties_main(int argc, char *argv[])
{
    bool force = false;

    int opt;

    static const char short_options[] = "hf";

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"force", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'f':
            force = true;
            break;

        case 'h':
            properties_usage(stdout);
            return EXIT_SUCCESS;

        default:
            properties_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind == 0) {
        properties_usage(stderr);
        return EXIT_FAILURE;
    }

    const char *action = argv[optind];
    ++optind;

    if (strcmp(action, "get") == 0) {
        if (argc - optind == 0) {
            util::property_iter([](std::string_view key,
                                   std::string_view value) {
                print_prop(key, value);
                return util::PropertyIterAction::Continue;
            });
        } else if (argc - optind == 1) {
            print_prop({}, util::property_get_string(argv[optind], {}));
        } else {
            properties_usage(stderr);
            return EXIT_FAILURE;
        }
    } else if (strcmp(action, "set") == 0) {
        if (argc - optind != 2) {
            properties_usage(stderr);
            return EXIT_FAILURE;
        }

        const char *key = argv[optind];
        const char *value = argv[optind + 1];

        if (!set_if_possible(key, value, force)) {
            return EXIT_FAILURE;
        }
    } else if (strcmp(action, "set-file") == 0) {
        if (argc - optind != 1) {
            properties_usage(stderr);
            return EXIT_FAILURE;
        }

        const char *path = argv[optind];

        if (!load_prop_file(path, force)) {
            return EXIT_FAILURE;
        }
    } else {
        fprintf(stderr, "Unknown action: %s\n", action);
        properties_usage(stderr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

}
