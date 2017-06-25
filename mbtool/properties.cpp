/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "properties.h"

#include <string>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <getopt.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/properties.h"

namespace mb
{

static void properties_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: properties get <property>\n"
            "   OR: properties set <property> <value> [--force]\n"
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
            util::property_list([](const std::string &key,
                                   const std::string &value,
                                   void *cookie){
                (void) cookie;
                fputs(key.c_str(), stdout);
                fputc('=', stdout);
                fputs(value.c_str(), stdout);
                fputc('\n', stdout);
            }, nullptr);
        } else if (argc - optind == 1) {
            std::string value = util::property_get_string(argv[optind], {});

            fputs(value.c_str(), stdout);
            fputc('\n', stdout);
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
        bool ret;

        if (force) {
            ret = util::property_set_direct(key, value);
        } else {
            prop_info *pi = const_cast<prop_info *>(
                    util::libc_system_property_find(key));
            if (pi && mb_starts_with(key, "ro.")) {
                fprintf(stderr, "Cannot overwrite read-only property '%s'"
                        " without -f/--force\n", key);
                return EXIT_FAILURE;
            }

            ret = util::property_set(key, value);
        }

        if (!ret) {
            fprintf(stderr, "Failed to set property '%s'='%s'\n", key, value);
            return EXIT_FAILURE;
        }
    } else {
        LOGE("Unknown action: %s", action);
        properties_usage(stderr);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

}
