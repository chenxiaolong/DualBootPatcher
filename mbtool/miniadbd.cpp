/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "miniadbd.h"

#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

#include "miniadbd/adb.h"
#include "miniadbd/transport.h"

#include "external/legacy_property_service.h"

#include "mbcommon/version.h"
#include "mblog/logging.h"
#include "mbutil/directory.h"
#include "mbutil/mount.h"
#include "mbutil/properties.h"


namespace mb
{

static bool write_file(const char *path, const char *data, std::size_t size)
{
    int fd = TEMP_FAILURE_RETRY(
            open(path, O_WRONLY | O_CREAT | O_NOFOLLOW | O_CLOEXEC, 0600));
    if (fd < 0) {
        LOGE("%s: Failed to open file: %s", path, strerror(errno));
        return false;
    }

    while (size > 0) {
        ssize_t n = TEMP_FAILURE_RETRY(write(fd, data, size));
        if (n < 0) {
            close(fd);
            return false;
        }
        data += n;
        size -= n;
    }

    close(fd);
    return true;
}

static bool initialize_adb()
{
    bool ret = true;

    const char *version = mb::version();
    ret = write_file("/sys/class/android_usb/android0/enable", "0", 1) && ret;
    ret = write_file("/sys/class/android_usb/android0/idVendor", "18D1", 4) && ret;
    ret = write_file("/sys/class/android_usb/android0/idProduct", "4EE7", 4) && ret;
    ret = write_file("/sys/class/android_usb/android0/f_ffs/aliases", "adb", 3) && ret;
    ret = write_file("/sys/class/android_usb/android0/functions", "adb", 3) && ret;
    ret = write_file("/sys/class/android_usb/android0/iManufacturer", "mbtool", 6) && ret;
    ret = write_file("/sys/class/android_usb/android0/iProduct", "miniadbd", 8) && ret;
    ret = write_file("/sys/class/android_usb/android0/iSerial", version, strlen(version)) && ret;
    ret = write_file("/sys/class/android_usb/android0/enable", "1", 1) && ret;

    // Create functionfs paths
    if (!util::mkdir_recursive(USB_FFS_ADB_PATH, 0770)) {
        LOGW("%s: Failed to create directory: %s",
             USB_FFS_ADB_PATH, strerror(errno));
        ret = false;
    }
    if (!util::mount("adb", USB_FFS_ADB_PATH, "functionfs", 0, "")) {
        LOGW("%s: Failed to mount functionfs: %s",
             USB_FFS_ADB_PATH, strerror(errno));
        ret = false;
    }

    // Set environment variables
    ret = setenv("PATH", "/system/bin", 1) == 0 && ret;
    ret = setenv("LD_LIBRARY_PATH", "/system/lib", 1) == 0 && ret;
    ret = setenv("ANDROID_DATA", "/data", 1) == 0 && ret;
    ret = setenv("ANDROID_ROOT", "/system", 1) == 0 && ret;

    // Use TWRP's legacy property workspace hack. TouchWiz's sh binary segfaults
    // if there's no property service (i.e. no /dev/__properties__ and no valid
    // ANDROID_PROPERTY_WORKSPACE environment variable). This doesn't actually
    // start a "real" properties service (setprop will never work), but it's
    // enough to prevent the segfaults.
    char tmp[32];
    int propfd, propsz;
    legacy_properties_init();

    // Load /default.prop
    std::unordered_map<std::string, std::string> props;
    util::property_file_get_all("/default.prop", props);
    for (auto const &pair : props) {
        legacy_property_set(pair.first.c_str(), pair.second.c_str());
    }
    // Load /system/build.prop
    props.clear();
    util::property_file_get_all("/system/build.prop", props);
    for (auto const &pair : props) {
        legacy_property_set(pair.first.c_str(), pair.second.c_str());
    }

    legacy_get_property_workspace(&propfd, &propsz);
    snprintf(tmp, sizeof(tmp), "%d,%d", dup(propfd), propsz);

    char *orig_prop_env = getenv("ANDROID_PROPERTY_WORKSPACE");
    LOGD("Original properties environment: %s",
         orig_prop_env ? orig_prop_env : "null");

    setenv("ANDROID_PROPERTY_WORKSPACE", tmp, 1);

    LOGD("Switched to legacy properties environment: %s", tmp);

    return ret;
}

static void cleanup()
{
    usb_cleanup();
}

static void miniadbd_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: miniadbd [OPTION]...\n\n"
            "Options:\n"
            "  -h, --help       Display this help message\n");
}

int miniadbd_main(int argc, char *argv[])
{
    int opt;

    enum options {
        IGNORED_ROOT_SECLEVEL,
        IGNORED_DEVICE_BANNER
    };

    static struct option long_options[] = {
        {"root_seclabel", required_argument, 0, IGNORED_ROOT_SECLEVEL},
        {"device_banner", required_argument, 0, IGNORED_DEVICE_BANNER},
        {"help",          no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case IGNORED_ROOT_SECLEVEL:
            printf("Ignoring --root_seclabel=%s\n", optarg);
            break;
        case IGNORED_DEVICE_BANNER:
            printf("Ignoring --device_banner=%s\n", optarg);
            break;
        case 'h':
            miniadbd_usage(stdout);
            return EXIT_SUCCESS;
        default:
            miniadbd_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        miniadbd_usage(stderr);
        return EXIT_FAILURE;
    }

    umask(0);

    initialize_adb();

    atexit(cleanup);
    // No SIGCHLD. Let the service subproc handle its children.
    signal(SIGPIPE, SIG_IGN);

    init_transport_registration();

    if (access(USB_ADB_PATH, F_OK) == 0 || access(USB_FFS_ADB_EP0, F_OK) == 0) {
        usb_init();
        fdevent_loop();
    } else {
        LOGE("Failed to open either %s or %s", USB_ADB_PATH, USB_FFS_ADB_EP0);
    }

    return EXIT_FAILURE;
}

}
