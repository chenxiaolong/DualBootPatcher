/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "auditd.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <getopt.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/finally.h"

#include "external/audit/libaudit.h"


namespace mb
{

static bool audit_mainloop()
{
    int fd = audit_open();
    if (fd < 0) {
        LOGE("Failed to open audit socket: %s", strerror(errno));
        return false;
    }

    auto close_fd = util::finally([&]{
        audit_close(fd);
    });

    if (audit_setup(fd, getpid()) < 0) {
        return false;
    }

    while (true) {
        struct audit_message reply;
        reply.nlh.nlmsg_type = 0;
        reply.nlh.nlmsg_len = 0;
        reply.data[0] = '\0';

        if (audit_get_reply(fd, &reply, GET_REPLY_BLOCKING, 0) < 0) {
            LOGE("Failed to get reply from audit socket: %s", strerror(errno));
            return false;
        }

        LOGV("type=%d %.*s",
             reply.nlh.nlmsg_type, reply.nlh.nlmsg_len, reply.data);
    }

    return false;
}

static void auditd_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: auditd [options]\n\n"
            "Options:\n"
            "  -h, --help       Display this help message\n");
}

int auditd_main(int argc, char *argv[])
{
    int opt;

    static const char short_options[] = "h";

    static struct option long_options[] = {
        {"help",  no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            auditd_usage(stdout);
            return EXIT_SUCCESS;

        default:
            auditd_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 0) {
        auditd_usage(stderr);
        return EXIT_FAILURE;
    }

    return audit_mainloop() ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
