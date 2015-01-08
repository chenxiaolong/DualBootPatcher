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

#include "update_binary_tool.h"

#include <errno.h>
#include <fts.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <unistd.h>

#include "util/file.h"
#include "util/logging.h"


#define ACTION_MOUNT "mount"
#define ACTION_UNMOUNT "unmount"
#define ACTION_FORMAT "format"
#define SYSTEM "/system"
#define CACHE "/cache"
#define DATA "/data"

#define TAG "update-binary-tool: "

#define STAMP_FILE "/.system-mounted"


static int do_mount(const char *mountpoint)
{
    if (strcmp(mountpoint, SYSTEM) == 0) {
        if (access(STAMP_FILE, F_OK) == 0) {
            LOGV(TAG "/system already mounted. Skipping");
            return 0;
        }

        if (mount("/tmp/system.img", SYSTEM,
                  "ext4", MS_NOSUID, "") < 0) {
            LOGE(TAG "Failed to mount %s: %s", mountpoint, strerror(errno));
            return -1;
        }

        mb_create_empty_file(STAMP_FILE);
    } else {
        LOGV(TAG "Ignoring mount command for %s", mountpoint);
    }

    return 0;
}

static int do_unmount(const char *mountpoint)
{
    if (strcmp(mountpoint, SYSTEM) == 0) {
        if (umount(SYSTEM) < 0 && errno != EINVAL) {
            LOGE(TAG "Failed to unmount %s: %s", mountpoint, strerror(errno));
            return -1;
        }

        remove(STAMP_FILE);
    } else {
        LOGV(TAG "Ignoring unmount command for %s", mountpoint);
    }

    return 0;
}

static int do_format(const char *mountpoint)
{
    int isData = strcmp(mountpoint, DATA) == 0;
    int ret = 0;
    FTS *ftsp = NULL;
    FTSENT *curr;

    char *files[] = { (char *) mountpoint, NULL };

    ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
    if (!ftsp) {
        LOGE("%s: fts_open failed: %s", mountpoint, strerror(errno));
        ret = -1;
        goto finish;
    }

    while ((curr = fts_read(ftsp))) {
        switch (curr->fts_info) {
        case FTS_NS:
        case FTS_DNR:
        case FTS_ERR:
            LOGW("%s: fts_read error: %s",
                 curr->fts_accpath, strerror(curr->fts_errno));
            continue;

        case FTS_DC:
        case FTS_DOT:
        case FTS_NSOK:
            // Not reached
            continue;
        }

        if (curr->fts_level == 1) {
            // Skip {/system,/cache,/data}/multiboot and /data/media
            if (strcmp(curr->fts_name, "multiboot") == 0
                    || (isData && strcmp(curr->fts_name, "media") == 0)) {
                fts_set(ftsp, curr, FTS_SKIP);
                continue;
            }
        }

        switch (curr->fts_info) {
        case FTS_D:
            // Do nothing. Need depth-first search, so directories are deleted
            // in FTS_DP
            continue;

        case FTS_DP:
        case FTS_F:
        case FTS_SL:
        case FTS_SLNONE:
        case FTS_DEFAULT:
            if (curr->fts_level >= 1 && remove(curr->fts_accpath) < 0) {
                LOGW("%s: Failed to remove: %s",
                     curr->fts_path, strerror(errno));
                ret = -1;
            }
            continue;
        }
    }

finish:
    if (ftsp) {
        fts_close(ftsp);
    }

    return ret;
}

static void update_binary_tool_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: update-binary-tool [action] [mountpoint]\n\n"
            "Actions:\n"
            "  mount          Mount a filesystem in multiboot environment\n"
            "  unmount        Unmount a filesystem in multiboot environment\n"
            "  format         Format a filesystem in multiboot environment\n"
            "\n"
            "Mountpoints:\n"
            "  /system        (Mount|Unmount|Format) multibooted /system\n"
            "  /cache         (Mount|Unmount|Format) multibooted /cache\n"
            "  /data          (Mount|Unmount) multibooted /data or\n"
            "                 Format multibooted /data, excluding /data/media\n"
            "\n"
            "This tool must be run in the chroot environment created by mbtool's\n"
            "update-binary wrapper. If it is run outside of the environment, it will\n"
            "simply print a warning and exit without performing any operation.\n");
}

int update_binary_tool_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            update_binary_tool_usage(0);
            return EXIT_SUCCESS;

        default:
            update_binary_tool_usage(1);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 2) {
        update_binary_tool_usage(1);
        return EXIT_FAILURE;
    }

    // Log to stderr, so the output is ordered correctly in /tmp/recovery.log
    mb_log_set_standard_stream_all(stderr);

    char *action = argv[optind];
    char *mountpoint = argv[optind + 1];

    if (strcmp(action, ACTION_MOUNT) != 0
            && strcmp(action, ACTION_UNMOUNT) != 0
            && strcmp(action, ACTION_FORMAT) != 0) {
        update_binary_tool_usage(1);
        return EXIT_FAILURE;
    }

    if (strcmp(mountpoint, SYSTEM) != 0
            && strcmp(mountpoint, CACHE) != 0
            && strcmp(mountpoint, DATA) != 0) {
        update_binary_tool_usage(1);
        return EXIT_FAILURE;
    }

    if (access("/.chroot", F_OK) < 0) {
        fprintf(stderr, "update-binary-tool must be run inside the chroot\n");
        return EXIT_FAILURE;
    }

    int ret = 0;

    if (strcmp(action, ACTION_MOUNT) == 0) {
        ret = do_mount(mountpoint);
    } else if (strcmp(action, ACTION_UNMOUNT) == 0) {
        ret = do_unmount(mountpoint);
    } else if (strcmp(action, ACTION_FORMAT) == 0) {
        ret = do_format(mountpoint);
    }

    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
