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

#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#if defined(SHIM)
#include "shim/init.h"
#else
#include "main/auditd.h"
#include "main/backup.h"
#include "main/init.h"
#include "main/properties.h"
#include "main/reboot.h"
#include "main/uevent_dump.h"
#include "main/update_binary.h"
#include "util/sepolpatch.h"
#include "util/signature.h"
#endif

#include "mbcommon/version.h"
#include "mbutil/process.h"
#include "mbutil/string.h"


static int mbtool_main(int argc, char *argv[]);


struct Tool
{
    const char *name;
    int (*func)(int, char **);
    bool show;
};

static Tool g_tools[] = {
#if defined(SHIM)
    { "mbtool-shim", mbtool_main, false },
    { "init", mb::init_main, true },
#else
    { "mbtool", mbtool_main, false },
    { "auditd", mb::auditd_main, true },
    { "backup", mb::backup_main, true },
    { "init", mb::init_main, true },
    { "properties", mb::properties_main, true },
    { "reboot", mb::reboot_main, true },
    { "restore", mb::restore_main, true },
    { "sepolpatch", mb::sepolpatch_main, true },
    { "shutdown", mb::shutdown_main, true },
    { "sigverify", mb::sigverify_main, true },
    { "uevent_dump", mb::uevent_dump_main, true },
    { "updater", mb::update_binary_main, true }, // TWRP
    { "update_binary", mb::update_binary_main, true }, // CWM, Philz
#endif
};


static void mbtool_usage(FILE *stream)
{
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
    for (auto const &tool : g_tools) {
        if (tool.show) {
            fprintf(stream, "  %s\n", tool.name);
        }
    }
}

static const Tool * find_tool(const char *name)
{
    for (auto const &tool : g_tools) {
        if (strcmp(tool.name, name) == 0) {
            return &tool;
        }
    }

    return nullptr;
}

static int main_multicall(int argc, char *argv[])
{
    char *name;
    char *prog;

    prog = strrchr(argv[0], '/');
    if (prog) {
        name = prog + 1;
    } else {
        name = argv[0];
    }

    const Tool *tool = find_tool(name);
    if (tool) {
        return tool->func(argc, argv);
    } else {
        fprintf(stderr, "%s: tool not found\n", name);
        return EXIT_FAILURE;
    }
}

static int main_normal(int argc, char *argv[])
{
    if (argc < 2) {
        mbtool_usage(stderr);
        return EXIT_FAILURE;
    }

    char *name = argv[1];
    const Tool *tool = find_tool(name);
    if (tool) {
        return tool->func(argc - 1, argv + 1);
    } else {
        fprintf(stderr, "%s: tool not found\n", name);
        return EXIT_FAILURE;
    }
}

static int mbtool_main(int argc, char *argv[])
{
    if (argc > 1) {
        return main_multicall(argc - 1, argv + 1);
    } else {
        mbtool_usage(stderr);
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

    if (auto ret = mb::util::set_process_title_init(argc, argv); !ret) {
        fprintf(stderr, "set_process_title_init() failed: %s\n",
                ret.error().message().c_str());
        return EXIT_FAILURE;
    }

    umask(0);

    if (!setlocale(LC_ALL, "")) {
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
