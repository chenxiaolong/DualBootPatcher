/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#define SYSTEM_STAMP_FILE "/.system-mounted"
#define CACHE_STAMP_FILE "/.cache-mounted"
#define DATA_STAMP_FILE "/.data-mounted"


namespace mb
{

static bool get_paths(const char *mountpoint, const char **out_image_loop_dev,
                      const char **out_stamp_file)
{
    const char *image_loop_dev;
    const char *stamp_file;

    if (strcmp(mountpoint, SYSTEM) == 0) {
        image_loop_dev = CHROOT_SYSTEM_LOOP_DEV;
        stamp_file = SYSTEM_STAMP_FILE;
    } else if (strcmp(mountpoint, CACHE) == 0) {
        image_loop_dev = CHROOT_CACHE_LOOP_DEV;
        stamp_file = CACHE_STAMP_FILE;
    } else if (strcmp(mountpoint, DATA) == 0) {
        image_loop_dev = CHROOT_DATA_LOOP_DEV;
        stamp_file = DATA_STAMP_FILE;
    } else {
        return false;
    }

    if (out_image_loop_dev) {
        *out_image_loop_dev = image_loop_dev;
    }
    if (out_stamp_file) {
        *out_stamp_file = stamp_file;
    }
    return true;
}

static bool do_mount(const char *mountpoint)
{
    const char *image_loop_dev;
    const char *stamp_file;

    if (!get_paths(mountpoint, &image_loop_dev, &stamp_file)) {
        LOGE(TAG "Invalid mountpoint: %s", mountpoint);
        return false;
    }

    if (access(image_loop_dev, F_OK) < 0) {
        // Assume we don't need the image if the wrapper didn't create it
        LOGV(TAG "Ignoring mount command for %s", mountpoint);
        return true;
    }

    if (access(stamp_file, F_OK) == 0) {
        LOGV(TAG "%s already mounted. Skipping", mountpoint);
        return true;
    }

    // NOTE: We don't need the loop mount logic in util::mount()
    if (mount(image_loop_dev, mountpoint, "ext4", MS_NOATIME, "") < 0) {
        LOGE(TAG "Failed to mount %s: %s", image_loop_dev, strerror(errno));
        return false;
    }

    util::file_write_data(stamp_file, "", 0);

    LOGD(TAG "Mounted %s at %s", image_loop_dev, mountpoint);

    return true;
}

static bool do_unmount(const char *mountpoint)
{
    const char *image_loop_dev;
    const char *stamp_file;

    if (!get_paths(mountpoint, &image_loop_dev, &stamp_file)) {
        LOGE(TAG "Invalid mountpoint: %s", mountpoint);
        return false;
    }

    if (access(image_loop_dev, F_OK) < 0) {
        // Assume we don't need the image if the wrapper didn't create it
        LOGV(TAG "Ignoring unmount command for %s", mountpoint);
        return true;
    }

    if (access(stamp_file, F_OK) != 0) {
        LOGV(TAG "%s not mounted. Skipping", mountpoint);
        return true;
    }

    // NOTE: We don't want to use util::umount() because it will disassociate
    // the loop device
    if (umount(mountpoint) < 0) {
        LOGE(TAG "Failed to unmount %s: %s", mountpoint, strerror(errno));
        return false;
    }

    remove(stamp_file);

    LOGD(TAG "Unmounted %s", mountpoint);

    return true;
}

static bool do_format(const char *mountpoint)
{
    const char *image_loop_dev;
    const char *stamp_file;

    if (!get_paths(mountpoint, &image_loop_dev, &stamp_file)) {
        LOGE(TAG "Invalid mountpoint: %s", mountpoint);
        return false;
    }

    // Need to mount the partition if we're using an image file and it
    // hasn't been mounted
    bool needs_mount = access(image_loop_dev, F_OK) == 0
            && access(stamp_file, F_OK) != 0;

    if (needs_mount && !do_mount(mountpoint)) {
        LOGE(TAG "Failed to mount %s", mountpoint);
        return false;
    }

    std::vector<std::string> exclusions;
    if (strcmp(mountpoint, DATA) == 0) {
        exclusions.push_back("media");
    }

    if (!wipe_directory(mountpoint, exclusions)) {
        LOGE(TAG "Failed to wipe %s", mountpoint);
        return false;
    }

    if (needs_mount && !do_unmount(mountpoint)) {
        LOGE(TAG "Failed to unmount %s", mountpoint);
        return false;
    }

    LOGD(TAG "Formatted %s", mountpoint);

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

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
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
