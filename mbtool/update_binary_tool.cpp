/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "update_binary_tool.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <getopt.h>
#include <sys/mount.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
#include "mbutil/file.h"
#include "mbutil/mount.h"

#include "multiboot.h"
#include "wipe.h"


#define ACTION_MOUNT "mount"
#define ACTION_UNMOUNT "unmount"
#define ACTION_FORMAT "format"
#define SYSTEM "/system"
#define CACHE "/cache"
#define DATA "/data"

#define TAG "update-binary-tool: "


namespace mb
{

static bool get_paths(const char *mountpoint, const char **out_source_path,
                      bool *out_is_image)
{
    const char *loop_path;
    const char *bind_path;
    bool is_image;

    if (strcmp(mountpoint, SYSTEM) == 0) {
        loop_path = CHROOT_SYSTEM_LOOP_DEV;
        bind_path = CHROOT_SYSTEM_BIND_MOUNT;
    } else if (strcmp(mountpoint, CACHE) == 0) {
        loop_path = CHROOT_CACHE_LOOP_DEV;
        bind_path = CHROOT_CACHE_BIND_MOUNT;
    } else if (strcmp(mountpoint, DATA) == 0) {
        loop_path = CHROOT_DATA_LOOP_DEV;
        bind_path = CHROOT_DATA_BIND_MOUNT;
    } else {
        return false;
    }

    is_image = access(loop_path, R_OK) == 0;

    if (out_source_path) {
        *out_source_path = is_image ? loop_path : bind_path;
    }
    if (out_is_image) {
        *out_is_image = is_image;
    }

    return true;
}

static bool do_mount(const char *mountpoint)
{
    const char *source_path;
    bool is_image;

    if (!get_paths(mountpoint, &source_path, &is_image)) {
        LOGE(TAG "%s: Invalid mountpoint", mountpoint);
        return false;
    }

    const char *fstype;
    unsigned long flags;

    if (is_image) {
        fstype = "ext4";
        flags = MS_NOATIME;
    } else {
        fstype = "";
        flags = MS_BIND;
    }

    // NOTE: We don't need the loop mount logic in util::mount()
    if (mount(source_path, mountpoint, fstype, flags, "") < 0) {
        LOGE(TAG "%s: Failed to mount path: %s", source_path, strerror(errno));
        return false;
    }

    LOGD(TAG "Successfully mounted %s at %s", source_path, mountpoint);

    return true;
}

static bool do_unmount(const char *mountpoint)
{
    if (!get_paths(mountpoint, nullptr, nullptr)) {
        LOGE(TAG "%s: Invalid mountpoint", mountpoint);
        return false;
    }

    // NOTE: We don't want to use util::umount() because it will disassociate
    // the loop device
    if (umount(mountpoint) < 0) {
        LOGE(TAG "%s: Failed to unmount: %s", mountpoint, strerror(errno));
        return false;
    }

    LOGD(TAG "Successfully unmounted %s", mountpoint);

    return true;
}

static bool do_format(const char *mountpoint)
{
    if (!get_paths(mountpoint, nullptr, nullptr)) {
        LOGE(TAG "%s: Invalid mountpoint", mountpoint);
        return false;
    }

    bool needs_mount = !util::is_mounted(mountpoint);

    if (needs_mount && !do_mount(mountpoint)) {
        LOGE(TAG "%s: Failed to mount path", mountpoint);
        return false;
    }

    std::vector<std::string> exclusions;
    if (strcmp(mountpoint, DATA) == 0) {
        exclusions.push_back("media");
    }

    if (!wipe_directory(mountpoint, exclusions)) {
        LOGE(TAG "%s: Failed to wipe directory", mountpoint);
        return false;
    }

    if (needs_mount && !do_unmount(mountpoint)) {
        LOGE(TAG "%s: Failed to unmount path", mountpoint);
        return false;
    }

    LOGD(TAG "Successfully formatted %s", mountpoint);

    return true;
}

static void update_binary_tool_usage(FILE *stream)
{
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

    static const char *short_options = "h";

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            update_binary_tool_usage(stdout);
            return EXIT_SUCCESS;

        default:
            update_binary_tool_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 2) {
        update_binary_tool_usage(stderr);
        return EXIT_FAILURE;
    }

    // Log to stderr, so the output is ordered correctly in /tmp/recovery.log
    log::log_set_logger(std::make_shared<log::StdioLogger>(stderr, false));

    const char *action = argv[optind];
    const char *mountpoint = argv[optind + 1];

    bool is_valid_action = strcmp(action, ACTION_MOUNT) == 0
            || strcmp(action, ACTION_UNMOUNT) == 0
            || strcmp(action, ACTION_FORMAT) == 0;
    bool is_valid_mountpoint = strcmp(mountpoint, SYSTEM) == 0
            || strcmp(mountpoint, CACHE) == 0
            || strcmp(mountpoint, DATA) == 0;

    if (!is_valid_action || !is_valid_mountpoint) {
        update_binary_tool_usage(stderr);
        return EXIT_FAILURE;
    }

    if (access("/.chroot", F_OK) < 0) {
        fprintf(stderr, "update-binary-tool must be run inside the chroot\n");
        return EXIT_FAILURE;
    }

    bool ret = false;

    if (strcmp(action, ACTION_MOUNT) == 0) {
        ret = do_mount(mountpoint);
    } else if (strcmp(action, ACTION_UNMOUNT) == 0) {
        ret = do_unmount(mountpoint);
    } else if (strcmp(action, ACTION_FORMAT) == 0) {
        ret = do_format(mountpoint);
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
