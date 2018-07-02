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

#ifdef RECOVERY
#include "recovery/backup.h"
#include "recovery/rom_installer.h"
#include "recovery/update_binary.h"
#include "recovery/update_binary_tool.h"
#include "recovery/utilities.h"
#else
#include "boot/appsync.h"
#include "boot/auditd.h"
#include "boot/daemon.h"
#include "boot/init.h"
#include "boot/properties.h"
#include "boot/reboot.h"
#include "boot/uevent_dump.h"
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
};

static Tool g_tools[] = {
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
    { "appsync", mb::appsync_main },
    { "auditd", mb::auditd_main },
    { "daemon", mb::daemon_main },
    { "init", mb::init_main },
    { "properties", mb::properties_main },
    { "reboot", mb::reboot_main },
    { "sepolpatch", mb::sepolpatch_main },
    { "shutdown", mb::shutdown_main },
    { "sigverify", mb::sigverify_main },
    { "uevent_dump", mb::uevent_dump_main },
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
        if (strcmp(tool.name, "mbtool") != 0
                && strcmp(tool.name, "mbtool_recovery") != 0) {
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
