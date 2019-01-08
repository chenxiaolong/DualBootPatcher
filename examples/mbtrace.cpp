/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <optional>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <getopt.h>
#include <unistd.h>

#include "mbcommon/integer.h"

#include "mbsystrace/hooks.h"
#include "mbsystrace/tracer.h"
#include "mbsystrace/tracee.h"


static void usage(FILE *stream, const char *prog_name)
{
    fprintf(stream,
            "Usage: %s [option...] -- <cmd> [<arg>...]\n"
            "       %s [option...] -p <PID>\n"
            "\n"
            "Options:\n"
            "  -f, --follow     Trace new children of tracee\n"
            "  -h, --help       Display this help message\n"
            "  -p, --pid <PID>  Attach to PID instead of running new command\n",
            prog_name, prog_name);
}

int main(int argc, char *argv[])
{
    using namespace mb::systrace;

    int opt;

    static constexpr char short_options[] = "fhp:";

    static option long_options[] = {
        {"follow", no_argument,       nullptr, 'f'},
        {"help",   no_argument,       nullptr, 'h'},
        {"pid",    required_argument, nullptr, 'p'},
        {nullptr,  0,                 nullptr, 0},
    };

    std::optional<pid_t> pid;
    Flags flags;
    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'f':
            flags |= Flag::TraceChildren;
            break;

        case 'p':
            pid_t value;
            if (!mb::str_to_num(optarg, 10, value)) {
                fprintf(stderr, "Invalid PID: %s\n", optarg);
                return EXIT_FAILURE;
            }
            pid = value;
            break;

        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;

        default:
            usage(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (pid ? (argc - optind > 0) : (argc - optind == 0)) {
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    Hooks hooks;

    hooks.new_tracee = [](auto tracee) {
        printf("[%d] New tracee began executing\n", tracee->tid);
    };

    hooks.tracee_exit = [](auto tid, auto exit_code) {
        printf("[%d] Exited with status %d\n", tid, exit_code);
        return action::Default{};
    };

    hooks.tracee_death = [](auto tid, auto signal) {
        printf("[%d] Killed by signal %d\n", tid, signal);
        return action::Default{};
    };

    hooks.tracee_disappear = [](auto tid) {
        printf("[%d] Disappeared due to execve call\n", tid);
    };

    hooks.tracee_signal = [](auto tracee, auto signal) {
        printf("[%d] Received signal %d\n", tracee->tid, signal);
        return action::Default{};
    };

    hooks.group_stop = [](auto tracee, auto signal) {
        printf("[%d] Entering group stop from signal %d\n",
               tracee->tid, signal);
        return action::Default{};
    };

    hooks.interrupt_stop = [](auto tracee) {
        printf("[%d] Interrupted\n", tracee->tid);
        return action::Default{};
    };

    hooks.syscall_entry = [](auto tracee, auto &info) {
        printf("[%d] Entering syscall: %s\n", tracee->tid, info.syscall.name());
        return action::Default{};
    };

    hooks.syscall_exit = [](auto tracee, auto &info) {
        printf("[%d] Exiting syscall: %s\n", tracee->tid, info.syscall.name());
        return action::Default{};
    };

    hooks.unknown_child = [](auto tid, auto status) {
        printf("[%d] Unknown child reported event with status 0x%x\n",
               tid, status);
    };

    Tracer tracer;

    if (pid) {
        if (auto r = tracer.attach(*pid, flags); !r) {
            fprintf(stderr, "Failed to attach to PID %d: %s\n",
                    *pid, r.error().message().c_str());
            return EXIT_FAILURE;
        }
    } else {
        if (auto r = tracer.fork([&] {
            execvp(argv[1], argv + 1);
            fprintf(stderr, "%s: Failed to execute: %s\n",
                    argv[1], strerror(errno));
        }, flags); !r) {
            fprintf(stderr, "Failed to create process: %s\n",
                    r.error().message().c_str());
            return EXIT_FAILURE;
        }
    }

    if (auto r = tracer.execute(hooks); !r) {
        fprintf(stderr, "Error in tracer: %s\n",
                r.error().message().c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
