/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#ifdef RECOVERY
#include "backup.h"
#include "rom_installer.h"
#include "update_binary.h"
#include "update_binary_tool.h"
#include "utilities.h"
#else
#include "appsync.h"
#include "auditd.h"
#include "daemon.h"
#include "init.h"
#include "miniadbd.h"
#include "properties.h"
#include "sepolpatch.h"
#include "signature.h"
#include "uevent_dump.h"
#endif

#include "mbcommon/version.h"
#include "mblog/logging.h"
#include "mbutil/process.h"
#include "mbutil/string.h"


int main_multicall(int argc, char *argv[]);
int main_normal(int argc, char *argv[]);
static int mbtool_main(int argc, char *argv[]);


#define TOOL(name) { #name, mb::name##_main }

struct tool {
    const char *name;
    int (*func)(int, char **);
};

struct tool tools[] = {
    { "mbtool", mbtool_main },
    { "mbtool_recovery", mbtool_main },
    // Tools
#ifdef RECOVERY
    { "backup", mb::backup_main },
    { "restore", mb::restore_main },
    { "rom-installer", mb::rom_installer_main },
    { "updater", mb::update_binary_main }, // TWRP
    { "update_binary", mb::update_binary_main }, // CWM, Philz
    { "update-binary-tool", mb::update_binary_tool_main },
    { "utilities", mb::utilities_main },
#else
    { "adbd", mb::miniadbd_main },
    { "appsync", mb::appsync_main },
    { "auditd", mb::auditd_main },
    { "daemon", mb::daemon_main },
    { "init", mb::init_main },
    { "miniadbd", mb::miniadbd_main },
    { "properties", mb::properties_main },
    { "sepolpatch", mb::sepolpatch_main },
    { "sigverify", mb::sigverify_main },
    { "uevent_dump", mb::uevent_dump_main },
#endif
    { nullptr, nullptr }
};


static void mbtool_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Version: %s\n"
            "Git version: %s\n\n"
            "Usage: mbtool [tool] [tool arguments ...]\n\n"
            "This is a multicall binary. The individual tools can be invoked\n"
            "by passing the tool name as the first argument to mbtool or by\n"
            "creating a symbolic link with from the tool name to mbtool.\n\n"
            "To see the usage and other help text for a tool, pass --help to\n"
            "the tool.\n\n"
            "Available tools:\n",
            mb::version(),
            mb::git_version());
    for (int i = 0; tools[i].name; ++i) {
        if (strcmp(tools[i].name, "mbtool") != 0
                && strcmp(tools[i].name, "mbtool_recovery") != 0) {
            fprintf(stream, "  %s\n", tools[i].name);
        }
    }
}

static int mbtool_main(int argc, char *argv[])
{
    if (argc > 1) {
        return main_multicall(argc - 1, argv + 1);
    } else {
        mbtool_usage(1);
        return EXIT_FAILURE;
    }
}

struct tool * find_tool(const char *name)
{
    for (int i = 0; tools[i].name; ++i) {
        if (strcmp(tools[i].name, name) == 0) {
            return &tools[i];
        }
    }

    return nullptr;
}

int main_multicall(int argc, char *argv[])
{
    char *name;
    char *prog;

    prog = strrchr(argv[0], '/');
    if (prog) {
        name = prog + 1;
    } else {
        name = argv[0];
    }

    struct tool *tool = find_tool(name);
    if (tool) {
        return tool->func(argc, argv);
    } else {
        fprintf(stderr, "%s: tool not found\n", name);
        return EXIT_FAILURE;
    }
}

int main_normal(int argc, char *argv[])
{
    if (argc < 2) {
        mbtool_usage(1);
        return EXIT_FAILURE;
    }

    char *name = argv[1];
    struct tool *tool = find_tool(name);
    if (tool) {
        return tool->func(argc - 1, argv + 1);
    } else {
        fprintf(stderr, "%s: tool not found\n", name);
        return EXIT_FAILURE;
    }
}

int main(int argc, char *argv[])
{
    // This works because argv is NULL-terminated
    char **argv_copy = mb::util::dup_cstring_list(argv);
    if (!argv_copy) {
        fprintf(stderr, "Failed to copy arguments: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (!mb::util::set_process_title_init(argc, argv)) {
        fprintf(stderr, "set_process_title_init() failed: %s\n",
                strerror(errno));
        return EXIT_FAILURE;
    }

    umask(0);

    if (!setlocale(LC_ALL, "C")) {
        fprintf(stderr, "Failed to set default locale\n");
    }

    int ret;

    char *no_multicall = getenv("MBTOOL_NO_MULTICALL");
    if (no_multicall && strcmp(no_multicall, "true") == 0) {
        ret = main_normal(argc, argv_copy);
    } else {
        ret = main_multicall(argc, argv_copy);
    }

    mb::util::free_cstring_list(argv_copy);

    return ret;
}
