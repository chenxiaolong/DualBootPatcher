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

#include "boot/reboot.h"

#include <getopt.h>

#include "mbutil/reboot.h"


namespace mb
{

static void reboot_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: reboot [OPTION...]\n\n"
            "Options:\n"
            "  -a, --argument <arg>\n"
            "                   Reboot argument (init and direct reboot only)\n"
            "  -c, --confirm    Ask for user confirmation before reboot (framework reboot only)\n"
            "  -m, --method <method>\n"
            "                   Reboot method (framework, init, or direct)\n"
            "                   (Default: init)\n"
            "  -h, --help       Display this help message\n"
            "\n"
            "Reboot methods:\n"
            "  - framework: Use Android framework to reboot. Behaves the same as the power menu.\n"
            "  - init: Use init (pid 1) to reboot. Same as \"adb reboot [<arg>]\".\n"
            "  - direct: Reboot directly by remounting filesystems as read-only and issuing the reboot syscall.\n");
}

static void shutdown_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: shutdown [OPTION...]\n\n"
            "Options:\n"
            "  -c, --confirm    Ask for user confirmation before shutting down\n"
            "                   (framework shut down only)\n"
            "  -m, --method <method>\n"
            "                   Shut down method (framework, init, or direct)\n"
            "                   (Default: init)\n"
            "  -h, --help       Display this help message\n"
            "\n"
            "Reboot types:\n"
            "  - framework: Use Android framework to shut down. Behaves the same as the power menu.\n"
            "  - init: Use init (pid 1) to shut down. Same as \"adb reboot -p\".\n"
            "  - direct: Shut down directly by remounting filesystems as read-only and issuing the reboot syscall.\n");
}

int reboot_main(int argc, char *argv[])
{
    int opt;

    static const char *short_options = "a:cm:h";
    static struct option long_options[] = {
        {"argument", required_argument, 0, 'a'},
        {"confirm",  no_argument,       0, 'c'},
        {"method",   required_argument, 0, 'm'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    std::string argument;
    bool confirm = false;
    std::string method;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'a':
            argument = optarg;
            break;
        case 'c':
            confirm = true;
            break;
        case 'm':
            method = optarg;
            break;
        case 'h':
            reboot_usage(stdout);
            return EXIT_SUCCESS;
        default:
            reboot_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        reboot_usage(stderr);
        return EXIT_FAILURE;
    }

    if (!argument.empty() && method == "framework") {
        fprintf(stderr, "Reboot arguments are not supported for framework reboots\n");
        return EXIT_FAILURE;
    } else if (confirm && method != "framework") {
        fprintf(stderr, "Reboot confirmation is only supported for framework reboots\n");
        return EXIT_FAILURE;
    }

    bool ret;

    if (method == "framework") {
        ret = util::reboot_via_framework(confirm);
    } else if (method == "init") {
        ret = util::reboot_via_init(argument);
    } else if (method == "direct") {
        ret = util::reboot_via_syscall(argument);
    } else {
        fprintf(stderr, "Invalid reboot method: %s\n", method.c_str());
        return EXIT_FAILURE;
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

int shutdown_main(int argc, char *argv[])
{
    int opt;

    static const char *short_options = "cm:h";
    static struct option long_options[] = {
        {"confirm",  no_argument,       0, 'c'},
        {"method",   required_argument, 0, 'm'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    bool confirm = false;
    std::string method;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'c':
            confirm = true;
            break;
        case 'm':
            method = optarg;
            break;
        case 'h':
            shutdown_usage(stdout);
            return EXIT_SUCCESS;
        default:
            shutdown_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        shutdown_usage(stderr);
        return EXIT_FAILURE;
    }

    if (confirm && method != "framework") {
        fprintf(stderr, "Shutdown confirmation is only supported for framework shutdowns\n");
        return EXIT_FAILURE;
    }

    bool ret;

    if (method == "framework") {
        ret = util::shutdown_via_framework(confirm);
    } else if (method == "init") {
        ret = util::shutdown_via_init();
    } else if (method == "direct") {
        ret = util::shutdown_via_syscall();
    } else {
        fprintf(stderr, "Invalid shutdown method: %s\n", method.c_str());
        return EXIT_FAILURE;
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
