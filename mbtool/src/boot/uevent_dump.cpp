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

#include "boot/uevent_dump.h"

#include <cstdio>
#include <cstdlib>

#include <getopt.h>

#include "mbcommon/version.h"

#include "boot/init/uevent_listener.h"

namespace mb
{

static void uevent_dump_usage(bool error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: uevent_dump"
            "\n"
            "Note: This tool takes no arguments\n"
            "\n"
            "This tool runs the uevent hardware detection code without creating\n"
            "any actual device files in /dev.\n");
}

int uevent_dump_main(int argc, char *argv[])
{
    using namespace android::init;

    int opt;

    static constexpr char short_options[] = "h";
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            uevent_dump_usage(false);
            return EXIT_SUCCESS;
        default:
            uevent_dump_usage(true);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        uevent_dump_usage(true);
        return EXIT_FAILURE;
    }

    printf("mbtool version %s (%s)\n", version(), git_version());

    UeventListener listener;

    listener.RegenerateUevents([] (const Uevent &uevent) {
        printf("uevent { action=%s, path=%s, subsystem=%s, firmware=%s, "
               "partition_name=%s, device_name=%s, partition_num=%d, "
               "major=%d, minor=%d }\n",
               uevent.action.c_str(), uevent.path.c_str(),
               uevent.subsystem.c_str(), uevent.firmware.c_str(),
               uevent.partition_name.c_str(), uevent.device_name.c_str(),
               uevent.partition_num, uevent.major, uevent.minor);

        return ListenerAction::kContinue;
    });

    return EXIT_SUCCESS;
}

}
