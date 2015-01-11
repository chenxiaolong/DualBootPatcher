/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "mount_fstab.h"
#include "sepolpatch.h"
#ifdef RECOVERY
#include "update_binary.h"
#include "update_binary_tool.h"
#endif

#include "util/logging.h"


const char *main_argv0 = NULL;
static int first_run = 1;


int main(int argc, char *argv[]);
static int mbtool_main(int argc, char *argv[]);


#define TOOL(name) { #name, name##_main }

struct tool {
    const char *name;
    int (*func)(int, char **);
};

struct tool tools[] = {
    TOOL(mbtool),
    // Tools
    TOOL(mount_fstab),
    TOOL(sepolpatch),
#ifdef RECOVERY
    { "updater", update_binary_main }, // TWRP
    { "update-binary", update_binary_main },
    { "update-binary-tool", update_binary_tool_main },
#endif
    { NULL, NULL }
};


static void mbtool_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: mbtool [tool] [tool arguments ...]\n\n"
            "This is a multicall binary. The individual tools can be invoked\n"
            "by passing the tool name as the first argument to mbtool or by\n"
            "creating a symbolic link with from the tool name to mbtool.\n\n"
            "To see the usage and other help text for a tool, pass --help to\n"
            "the tool.\n\n"
            "Available tools:\n");
    for (int i = 0; tools[i].name; ++i) {
        if (strcmp(tools[i].name, "mbtool") != 0) {
            fprintf(stream, "  %s\n", tools[i].name);
        }
    }
}

static int mbtool_main(int argc, char *argv[])
{
    if (argc > 1) {
        return main(argc - 1, argv + 1);
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

    return NULL;
}

int main(int argc, char *argv[])
{
    if (first_run) {
        main_argv0 = argv[0];
        first_run = 0;
    }

    char *name;
    char *prog;

    prog = strrchr(argv[0], '/');
    if (prog) {
        name = prog + 1;
    } else {
        name = argv[0];
    }

    umask(0);

    struct tool *tool = find_tool(name);
    if (tool) {
        return tool->func(argc, argv);
    } else {
        fprintf(stderr, "%s: tool not found\n", name);
        return EXIT_FAILURE;
    }
}

const char * mb_self_get_path(void)
{
    return main_argv0;
}
