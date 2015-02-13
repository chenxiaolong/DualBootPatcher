/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cerrno>
#include <cstdlib>
#include <getopt.h>
#include <sys/mount.h>
#include <unistd.h>

#include "multiboot.h"
#include "util/file.h"
#include "util/logging.h"
#include "util/loopdev.h"
#include "util/mount.h"


#define ACTION_MOUNT "mount"
#define ACTION_UNMOUNT "unmount"
#define ACTION_FORMAT "format"
#define SYSTEM "/system"
#define CACHE "/cache"
#define DATA "/data"

#define TAG "update-binary-tool: "

#define STAMP_FILE "/.system-mounted"


namespace mb
{

static bool do_mount(const std::string &mountpoint)
{
    if (mountpoint == SYSTEM) {
        if (access("/multiboot/system.img", F_OK) < 0) {
            // Assume we don't need the image if the wrapper didn't create it
            LOGV(TAG "Ignoring mount command for {}", mountpoint);
            return true;
        }

        if (access(STAMP_FILE, F_OK) == 0) {
            LOGV(TAG "/system already mounted. Skipping");
            return true;
        }

        std::string loopdev = util::loopdev_find_unused();

        if (loopdev.empty()) {
            LOGE(TAG "Failed to find unused loop device: {}", strerror(errno));
            return false;
        }

        if (!util::loopdev_setup_device(loopdev, "/multiboot/system.img", 0, 0)) {
            LOGE(TAG "Failed to setup loop device {}: {}",
                 loopdev, strerror(errno));
            return false;
        }

        if (mount(loopdev.c_str(), SYSTEM, "ext4", 0, "") < 0) {
            LOGE(TAG "Failed to mount {}: {}",
                 loopdev, strerror(errno));
            util::loopdev_remove_device(loopdev);
            return false;
        }

        util::file_write_data(STAMP_FILE, loopdev.data(), loopdev.size());

        LOGD(TAG "Mounted {} at {}", loopdev, SYSTEM);
    } else {
        LOGV(TAG "Ignoring mount command for {}", mountpoint);
    }

    return true;
}

static bool do_unmount(const std::string &mountpoint)
{
    if (mountpoint == SYSTEM) {
        if (access("/multiboot/system.img", F_OK) < 0) {
            // Assume we don't need the image if the wrapper didn't create it
            LOGV(TAG "Ignoring unmount command for {}", mountpoint);
            return true;
        }

        if (access(STAMP_FILE, F_OK) != 0) {
            LOGV(TAG "/system not mounted. Skipping");
            return true;
        }

        std::string loopdev;
        if (!util::file_first_line(STAMP_FILE, &loopdev)) {
            LOGE(TAG "Failed to get first line of " STAMP_FILE);
            return false;
        }

        if (umount(SYSTEM) < 0) {
            LOGE(TAG "Failed to unmount {}: {}", SYSTEM, strerror(errno));
            return false;
        }

        if (!util::loopdev_remove_device(loopdev)) {
            LOGE(TAG "Failed to remove loop device {}: {}",
                 loopdev, strerror(errno));
            return false;
        }

        remove(STAMP_FILE);

        LOGD(TAG "Unmounted {}", SYSTEM);
    } else {
        LOGV(TAG "Ignoring unmount command for {}", mountpoint);
    }

    return true;
}

static bool do_format(const std::string &mountpoint)
{
    if (mountpoint == SYSTEM || mountpoint == CACHE) {
        // Need to mount the partition if we're using an image file and it
        // hasn't been mounted
        int needs_mount = (mountpoint == SYSTEM)
                && (access("/multiboot/system.img", F_OK) == 0)
                && (access(STAMP_FILE, F_OK) != 0);

        if (needs_mount && !do_mount(mountpoint)) {
            LOGE(TAG "Failed to mount {}", mountpoint);
            return false;
        }

        if (!wipe_directory(mountpoint, true)) {
            LOGE(TAG "Failed to wipe {}", mountpoint);
            return false;
        }

        if (needs_mount && !do_unmount(mountpoint)) {
            LOGE(TAG "Failed to unmount {}", mountpoint);
            return false;
        }
    } else if (mountpoint == DATA) {
        if (!wipe_directory(mountpoint, false)) {
            LOGE(TAG "Failed to wipe {}", mountpoint);
            return false;
        }
    }

    LOGD(TAG "Formatted {}", mountpoint);

    return true;
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
    util::log_set_target(util::LogTarget::STDERR);

    std::string action = argv[optind];
    std::string mountpoint = argv[optind + 1];

    if (action != ACTION_MOUNT
            && action != ACTION_UNMOUNT
            && action != ACTION_FORMAT) {
        update_binary_tool_usage(1);
        return EXIT_FAILURE;
    }

    if (mountpoint != SYSTEM
            && mountpoint != CACHE
            && mountpoint != DATA) {
        update_binary_tool_usage(1);
        return EXIT_FAILURE;
    }

    if (access("/.chroot", F_OK) < 0) {
        fprintf(stderr, "update-binary-tool must be run inside the chroot\n");
        return EXIT_FAILURE;
    }

    bool ret = false;

    if (action == ACTION_MOUNT) {
        ret = do_mount(mountpoint);
    } else if (action == ACTION_UNMOUNT) {
        ret = do_unmount(mountpoint);
    } else if (action == ACTION_FORMAT) {
        ret = do_format(mountpoint);
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

}