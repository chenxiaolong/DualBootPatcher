/*
 * Copyright (C) 2007-2014 The Android Open Source Project
 * Copyright (C) 2015 Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "initwrapper/devices.h"

#include <mutex>

#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include "mbcommon/common.h"
#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/cmdline.h"
#include "mbutil/directory.h"
#include "mbutil/string.h"
#include "mbutil/external/system_properties.h"

#include "initwrapper/cutils/uevent.h"
#include "initwrapper/util.h"

#define DEVPATH_LEN 96

#define UEVENT_LOGGING 0

static char bootdevice[PROP_VALUE_MAX];
static int device_fd = -1;
static int pipe_fd[2];
static volatile bool run_thread = true;
static pthread_t thread;
static bool dry_run = false;

struct uevent {
    const char *action;
    const char *path;
    const char *subsystem;
    const char *firmware;
    const char *partition_name;
    const char *device_name;
    int partition_num;
    int major;
    int minor;
};

struct platform_node {
    char *name;
    char *path;
    int path_len;
};

static std::vector<platform_node> platform_names;

static std::unordered_map<std::string, BlockDevInfo> block_dev_mappings;
static std::mutex block_dev_mappings_guard;

static mode_t get_device_perm(const char *path,
                              const std::vector<std::string> &links,
                              unsigned *uid, unsigned *gid)
{
    (void) path;
    (void) links;
    *uid = 0;
    *gid = 0;
    return 0600;
}

static void make_device(const char *path,
                        const char *upath MB_UNUSED,
                        int block, int major, int minor,
                        const std::vector<std::string> &links)
{
    unsigned uid;
    unsigned gid;
    mode_t mode;
    dev_t dev;

#if UEVENT_LOGGING
    LOGD("Creating device %s", path);
    for (const std::string &link : links) {
        LOGD("- Symlink %s", link.c_str());
    }
#endif

    mode = get_device_perm(path, links, &uid, &gid) | (block ? S_IFBLK : S_IFCHR);

    dev = makedev(major, minor);
    // Temporarily change egid to avoid race condition setting the gid of the
    // device node. Unforunately changing the euid would prevent creation of
    // some device nodes, so the uid has to be set with chown() and is still
    // racy. Fixing the gid race at least fixed the issue with system_server
    // opening dynamic input devices under the AID_INPUT gid.
    if (!dry_run) {
        setegid(gid);
        mknod(path, mode, dev);
        chown(path, uid, -1);
        setegid(0);
    }
}

static void add_platform_device(const char *path)
{
    int path_len = strlen(path);
    const char *name = path;

    if (strncmp(path, "/devices/", 9) == 0) {
        name += 9;
        if (strncmp(name, "platform/", 9) == 0) {
            name += 9;
        }
    }

#if UEVENT_LOGGING
    LOGI("Adding platform device %s (%s)", name, path);
#endif

    platform_names.emplace_back();
    struct platform_node &bus = platform_names.back();
    bus.path = strdup(path);
    bus.path_len = path_len;
    bus.name = bus.path + (name - path);
}

/*
 * Given a path that may start with a platform device, find the length of the
 * platform device prefix.  If it doesn't start with a platform device, return
 * 0.
 */
static std::vector<struct platform_node *> find_platform_devices(const char *path)
{
    std::vector<struct platform_node *> nodes;
    int path_len = strlen(path);

    for (auto it = platform_names.rbegin(); it != platform_names.rend(); ++it) {
        struct platform_node &bus = *it;

        if ((bus.path_len < path_len) &&
                (path[bus.path_len] == '/') &&
                strncmp(path, bus.path, bus.path_len) == 0) {
            nodes.push_back(&bus);
        }
    }

    return nodes;
}

static void remove_platform_device(const char *path)
{
    for (auto it = platform_names.rbegin(); it != platform_names.rend(); ++it) {
        const struct platform_node &bus = *it;

        if (strcmp(path, bus.path) == 0) {
#if UEVENT_LOGGING
            LOGI("Removing platform device %s", bus.name);
#endif
            free(bus.path);
            platform_names.erase((it + 1).base());
            return;
        }
    }
}

/*
 * Given a path that may start with an MTD device (/devices/virtual/mtd/mtd8/mtdblock8),
 * populate the supplied buffer with the MTD partition number and return 0.
 * If it doesn't start with an MTD device, or there is some error, return -1
 */
static int find_mtd_device_prefix(const char *path, char *buf, ssize_t buf_sz)
{
    const char *start, *end;

    if (strncmp(path, "/devices/virtual/mtd", 20) != 0) {
        return -1;
    }

    // Beginning of the prefix is the initial "mtdXX" after "/devices/virtual/mtd/"
    start = path + 21;

    // End of the prefix is one path '/' later, capturing the partition number
    // Example: mtd8
    end = strchr(start, '/');
    if (!end) {
        return -1;
    }

    // Make sure we have enough room for the string plus null terminator
    if (end - start + 1 > buf_sz) {
        return -1;
    }

    strncpy(buf, start, end - start);
    buf[end - start] = '\0';
    return 0;
}

/*
 * Given a path that may start with a PCI device, populate the supplied buffer
 * with the PCI domain/bus number and the peripheral ID and return 0.
 * If it doesn't start with a PCI device, or there is some error, return -1
 */
static int find_pci_device_prefix(const char *path, char *buf, ssize_t buf_sz)
{
    const char *start, *end;

    if (strncmp(path, "/devices/pci", 12) != 0) {
        return -1;
    }

    // Beginning of the prefix is the initial "pci" after "/devices/"
    start = path + 9;

    // End of the prefix is two path '/' later, capturing the domain/bus number
    // and the peripheral ID. Example: pci0000:00/0000:00:1f.2
    end = strchr(start, '/');
    if (!end) {
        return -1;
    }
    end = strchr(end + 1, '/');
    if (!end) {
        return -1;
    }

    // Make sure we have enough room for the string plus null terminator
    if (end - start + 1 > buf_sz) {
        return -1;
    }

    strncpy(buf, start, end - start);
    buf[end - start] = '\0';
    return 0;
}

#if UEVENT_LOGGING
static void dump_uevent(struct uevent *uevent)
{
    LOGD("event { action='%s', path='%s', subsystem='%s', firmware='%s',"
         " name='%s', partname='%s', partnum=%d, major=%d, minor=%d }",
         uevent->action ? uevent->action : "(null)",
         uevent->path ? uevent->path : "(null)",
         uevent->subsystem ? uevent->subsystem : "(null)",
         uevent->firmware ? uevent->firmware : "(null)",
         uevent->device_name ? uevent->device_name : "(null)",
         uevent->partition_name ? uevent->partition_name : "(null)",
         uevent->partition_num,
         uevent->major,
         uevent->minor);
}
#endif

static void parse_event(const char *msg, struct uevent *uevent)
{
    uevent->action = "";
    uevent->path = "";
    uevent->subsystem = "";
    uevent->firmware = "";
    uevent->major = -1;
    uevent->minor = -1;
    uevent->partition_name = nullptr;
    uevent->partition_num = -1;
    uevent->device_name = nullptr;

    // Currently ignoring SEQNUM
    while (*msg) {
        if (strncmp(msg, "ACTION=", 7) == 0) {
            msg += 7;
            uevent->action = msg;
        } else if (strncmp(msg, "DEVPATH=", 8) == 0) {
            msg += 8;
            uevent->path = msg;
        } else if (strncmp(msg, "SUBSYSTEM=", 10) == 0) {
            msg += 10;
            uevent->subsystem = msg;
        } else if (strncmp(msg, "FIRMWARE=", 9) == 0) {
            msg += 9;
            uevent->firmware = msg;
        } else if (strncmp(msg, "MAJOR=", 6) == 0) {
            msg += 6;
            uevent->major = atoi(msg);
        } else if (strncmp(msg, "MINOR=", 6) == 0) {
            msg += 6;
            uevent->minor = atoi(msg);
        } else if (strncmp(msg, "PARTN=", 6) == 0) {
            msg += 6;
            uevent->partition_num = atoi(msg);
        } else if (strncmp(msg, "PARTNAME=", 9) == 0) {
            msg += 9;
            uevent->partition_name = msg;
        } else if (strncmp(msg, "DEVNAME=", 8) == 0) {
            msg += 8;
            uevent->device_name = msg;
        } else {
#if UEVENT_LOGGING
            if (strcmp(uevent->subsystem, "block") == 0) {
                LOGW("Unknown message: '%s'", msg);
            }
#endif
        }

        // Advance to after the next \0
        while (*msg++);
    }

#if UEVENT_LOGGING
    dump_uevent(uevent);
#endif
}

static std::vector<std::string> get_character_device_symlinks(struct uevent *uevent)
{
    const char *parent;
    const char *slash;
    int width;
    std::vector<struct platform_node *> pdevs;

    pdevs = find_platform_devices(uevent->path);
    if (pdevs.empty()) {
        return {};
    }

    std::vector<std::string> links;

    for (struct platform_node *pdev : pdevs) {
        // Skip "/devices/platform/<driver>"
        parent = strchr(uevent->path + pdev->path_len, '/');
        if (!parent) {
            continue;
        }

        if (strncmp(parent, "/usb", 4) == 0) {
            // Skip root hub name and device. use device interface
            while (*++parent && *parent != '/');
            if (*parent) {
                while (*++parent && *parent != '/');
            }
            if (!*parent) {
                continue;
            }
            slash = strchr(++parent, '/');
            if (!slash) {
                continue;
            }
            width = slash - parent;
            if (width <= 0) {
                continue;
            }

            char *path = mb_format("/dev/usb/%s%.*s",
                                   uevent->subsystem, width, parent);
            if (path) {
                links.push_back(path);
                free(path);
            }

            if (!dry_run) {
                mkdir("/dev/usb", 0755);
            }
        }
    }

    return links;
}

static std::vector<std::string> get_block_device_symlinks(struct uevent *uevent)
{
    std::vector<std::string> devices;
    std::vector<struct platform_node *> pdevs;
    const char *slash;
    const char *type;
    char buf[256];
    char link_path[256];
    char *p;
    bool is_bootdevice = false;
    int mtd_fd = -1;
    int nr;
    char mtd_name_path[256];
    char mtd_name[64];

    pdevs = find_platform_devices(uevent->path);
    if (!pdevs.empty()) {
        for (auto *pdev : pdevs) {
            devices.push_back(pdev->name);
        }
        type = "platform";
    } else if (find_pci_device_prefix(uevent->path, buf, sizeof(buf)) == 0) {
        devices.push_back(buf);
        type = "pci";
    } else if (find_mtd_device_prefix(uevent->path, buf, sizeof(buf)) == 0) {
        devices.push_back(buf);
        type = "mtd";
    } else {
        return {};
    }

    std::vector<std::string> links;

#if UEVENT_LOGGING
    for (auto const &device : devices) {
        LOGD("Found %s device %s", type, device.c_str());
    }
#endif

    for (auto const &device : devices) {
        snprintf(link_path, sizeof(link_path),
                 "/dev/block/%s/%s", type, device.c_str());

        if (strcmp(type, "mtd") == 0) {
            snprintf(mtd_name_path, sizeof(mtd_name_path),
                     "/sys/devices/virtual/%s/%s/name", type, device.c_str());
            mtd_fd = open(mtd_name_path, O_RDONLY | O_CLOEXEC);
            if (mtd_fd < 0) {
#if UEVENT_LOGGING
                LOGE("Unable to open %s for reading", mtd_name_path);
#endif
                continue;
            }
            nr = read(mtd_fd, mtd_name, sizeof(mtd_name) - 1);
            if (nr <= 0) {
                continue;
            }
            close(mtd_fd);
            mtd_name[nr - 1] = '\0';

            p = strdup(mtd_name);
            sanitize(p);

            char *path = mb_format("/dev/block/%s/by-name/%s", type, p);
            if (path) {
                links.push_back(path);
                free(path);
            }

            free(p);
        }

        if (!pdevs.empty() && bootdevice[0] != '\0'
                && strstr(device.c_str(), bootdevice)) {
            if (!dry_run) {
                make_link_init(link_path, "/dev/block/bootdevice");
            }
            is_bootdevice = true;
        } else {
            is_bootdevice = false;
        }

        if (uevent->partition_name) {
            p = strdup(uevent->partition_name);
            sanitize(p);
            if (strcmp(uevent->partition_name, p) != 0) {
#if UEVENT_LOGGING
                LOGV("Linking partition '%s' as '%s'",
                     uevent->partition_name, p);
#endif
            }

            char *path = mb_format("%s/by-name/%s", link_path, p);
            if (path) {
                links.push_back(path);
                free(path);
            }

            if (is_bootdevice) {
                path = mb_format("/dev/block/bootdevice/by-name/%s", p);
                if (path) {
                    links.push_back(path);
                    free(path);
                }
            }
            free(p);
        }

        if (uevent->partition_num >= 0) {
            char *path = mb_format("%s/by-num/p%d",
                                   link_path, uevent->partition_num);
            if (path) {
                links.push_back(path);
                free(path);
            }

            if (is_bootdevice) {
                path = mb_format("/dev/block/bootdevice/by-num/p%d",
                                 uevent->partition_num);
                if (path) {
                    links.push_back(path);
                    free(path);
                }
            }
        }

        slash = strrchr(uevent->path, '/');

        char *path = mb_format("%s/%s", link_path, slash + 1);
        if (path) {
            links.push_back(path);
            free(path);
        }
    }

    return links;
}

static void handle_device(const char *action, const char *devpath,
                          const char *path, int block, int major, int minor,
                          const std::vector<std::string> &links)
{
    if (strcmp(action, "add") == 0) {
        make_device(devpath, path, block, major, minor, links);
        for (const std::string &link : links) {
            if (!dry_run) {
                make_link_init(devpath, link.c_str());
            }
        }
    }

    if (strcmp(action, "remove") == 0) {
        for (const std::string &link : links) {
            if (!dry_run) {
                remove_link(devpath, link.c_str());
            }
        }
        if (!dry_run) {
            unlink(devpath);
        }
    }
}

static void handle_platform_device_event(struct uevent *uevent)
{
    if (strcmp(uevent->action, "add") == 0) {
        add_platform_device(uevent->path);
    } else if (strcmp(uevent->action, "remove") == 0) {
        remove_platform_device(uevent->path);
    }
}

static const char * parse_device_name(struct uevent *uevent, unsigned int len)
{
    const char *name;

    // If it's not a /dev device, nothing else to do
    if ((uevent->major < 0) || (uevent->minor < 0)) {
        return nullptr;
    }

    // Do we have a name?
    name = strrchr(uevent->path, '/');
    if (!name) {
        return nullptr;
    }
    ++name;

    // Too-long names would overrun our buffer
    if (strlen(name) > len) {
#if UEVENT_LOGGING
        LOGE("DEVPATH=%s exceeds %u-character limit on filename; ignoring event",
             name, len);
#endif
        return nullptr;
    }

    return name;
}

static void handle_block_device_event(struct uevent *uevent)
{
    const char *base = "/dev/block/";
    const char *name;
    char devpath[DEVPATH_LEN];
    std::vector<std::string> links;

    name = parse_device_name(uevent, 64);
    if (!name) {
        return;
    }

    snprintf(devpath, sizeof(devpath), "%s%s", base, name);
    if (!dry_run) {
        mkdir(base, 0755);
    }

    if (strncmp(uevent->path, "/devices/", 9) == 0) {
        links = get_block_device_symlinks(uevent);
    }

    handle_device(uevent->action, devpath, uevent->path, 1,
            uevent->major, uevent->minor, links);

    // Add/remove block device mapping
    if (strcmp(uevent->action, "add") == 0) {
        BlockDevInfo info;
        info.path = devpath;
        info.partition_num = uevent->partition_num;
        info.major = uevent->major;
        info.minor = uevent->minor;

        if (uevent->partition_name) {
            info.partition_name = uevent->partition_name;
        }

        std::lock_guard<std::mutex> lock(block_dev_mappings_guard);
        block_dev_mappings.emplace(
                std::make_pair(uevent->path, std::move(info)));
    } else if (strcmp(uevent->action, "remove") == 0) {
        std::lock_guard<std::mutex> lock(block_dev_mappings_guard);
        block_dev_mappings.erase(uevent->path);
    }
}

static bool assemble_devpath(char *devpath, const char *dirname,
                             const char *devname)
{
    int s = snprintf(devpath, DEVPATH_LEN, "%s/%s", dirname, devname);
    if (s < 0) {
#if UEVENT_LOGGING
        LOGE("Failed to assemble device path (%s); ignoring event",
             strerror(errno));
#endif
        return false;
    } else if (s >= DEVPATH_LEN) {
#if UEVENT_LOGGING
        LOGE("%s/%s exceeds %u-character limit on path; ignoring event",
             dirname, devname, DEVPATH_LEN);
#endif
        return false;
    }
    return true;
}

static void handle_generic_device_event(struct uevent *uevent)
{
    const char *base = nullptr;
    const char *name;
    char devpath[DEVPATH_LEN] = { 0 };
    std::vector<std::string> links;

    name = parse_device_name(uevent, 64);
    if (!name) {
        return;
    }

    if (strncmp(uevent->subsystem, "usb", 3) == 0) {
        if (strcmp(uevent->subsystem, "usb") == 0) {
            if (uevent->device_name) {
                if (!assemble_devpath(devpath, "/dev", uevent->device_name)) {
                    return;
                }
                if (!dry_run) {
                    mb::util::mkdir_parent(devpath, 0755);
                }
            } else {
                // This imitates the file system that would be created
                // if we were using devfs instead.
                // Minors are broken up into groups of 128, starting at "001"
                int bus_id = uevent->minor / 128 + 1;
                int device_id = uevent->minor % 128 + 1;
                // Build directories
                if (!dry_run) {
                    mkdir("/dev/bus", 0755);
                    mkdir("/dev/bus/usb", 0755);
                    snprintf(devpath, sizeof(devpath),
                            "/dev/bus/usb/%03d", bus_id);
                    mkdir(devpath, 0755);
                    snprintf(devpath, sizeof(devpath),
                            "/dev/bus/usb/%03d/%03d", bus_id, device_id);
                }
            }
        } else {
            // Ignore other USB events
            return;
        }
    } else if (strncmp(uevent->subsystem, "graphics", 8) == 0) {
        base = "/dev/graphics/";
        if (!dry_run) {
            mkdir(base, 0755);
        }
    } else if (strncmp(uevent->subsystem, "drm", 3) == 0) {
        base = "/dev/dri/";
        if (!dry_run) {
            mkdir(base, 0755);
        }
    } else if (strncmp(uevent->subsystem, "oncrpc", 6) == 0) {
        base = "/dev/oncrpc/";
        if (!dry_run) {
            mkdir(base, 0755);
        }
    } else if (strncmp(uevent->subsystem, "adsp", 4) == 0) {
        base = "/dev/adsp/";
        if (!dry_run) {
            mkdir(base, 0755);
        }
    } else if (strncmp(uevent->subsystem, "msm_camera", 10) == 0) {
        base = "/dev/msm_camera/";
        if (!dry_run) {
            mkdir(base, 0755);
        }
    } else if (strncmp(uevent->subsystem, "input", 5) == 0) {
        base = "/dev/input/";
        if (!dry_run) {
            mkdir(base, 0755);
        }
    } else if (strncmp(uevent->subsystem, "mtd", 3) == 0) {
        base = "/dev/mtd/";
        if (!dry_run) {
            mkdir(base, 0755);
        }
    } else if (strncmp(uevent->subsystem, "sound", 5) == 0) {
        base = "/dev/snd/";
        if (!dry_run) {
            mkdir(base, 0755);
        }
    } else if (strncmp(uevent->subsystem, "misc", 4) == 0
            && strncmp(name, "log_", 4) == 0) {
#if UEVENT_LOGGING
        LOGI("kernel logger is deprecated");
#endif
        base = "/dev/log/";
        if (!dry_run) {
            mkdir(base, 0755);
        }
        name += 4;
    } else {
        base = "/dev/";
    }
    links = get_character_device_symlinks(uevent);

    if (!devpath[0]) {
        snprintf(devpath, sizeof(devpath), "%s%s", base, name);
    }

    handle_device(uevent->action, devpath, uevent->path, 0,
            uevent->major, uevent->minor, links);
}

static void handle_device_event(struct uevent *uevent)
{
    if (strncmp(uevent->subsystem, "block", 5) == 0) {
        handle_block_device_event(uevent);
    } else if (strncmp(uevent->subsystem, "platform", 8) == 0) {
        handle_platform_device_event(uevent);
    } else {
        handle_generic_device_event(uevent);
    }
}

#define UEVENT_MSG_LEN  2048
void handle_device_fd()
{
    char msg[UEVENT_MSG_LEN + 2];
    int n;
    while ((n = uevent_kernel_multicast_recv(device_fd, msg, UEVENT_MSG_LEN)) > 0) {
        if (n >= UEVENT_MSG_LEN) {
            // overflow -- discard
            continue;
        }

        msg[n] = '\0';
        msg[n + 1] = '\0';

        struct uevent uevent;
        parse_event(msg, &uevent);

        if (uevent.path && strstr(uevent.path, "sec-battery")) {
            // sec-battery causes boot delays on the Galaxy S4
            continue;
        }

        handle_device_event(&uevent);
    }
}

/*
 * Coldboot walks parts of the /sys tree and pokes the uevent files
 * to cause the kernel to regenerate device add events that happened
 * before init's device manager was started
 *
 * We drain any pending events from the netlink socket every time
 * we poke another uevent file to make sure we don't overrun the
 * socket's buffer.
 */

static void do_coldboot(DIR *d)
{
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY);
    if (fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);
        handle_device_fd();
    }

    while ((de = readdir(d))) {
        DIR *d2;

        if (de->d_type != DT_DIR || de->d_name[0] == '.') {
            continue;
        }

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if (fd < 0) {
            continue;
        }

        d2 = fdopendir(fd);
        if (d2 == 0) {
            close(fd);
        } else {
            do_coldboot(d2);
            closedir(d2);
        }
    }
}

static void coldboot(const char *path)
{
    DIR *d = opendir(path);
    if (d) {
        do_coldboot(d);
        closedir(d);
    }
}

void * device_thread(void *)
{
    struct pollfd fds[2];
    fds[0].fd = pipe_fd[0];
    fds[0].events = POLLIN;
    fds[1].fd = get_device_fd();
    fds[1].events = POLLIN;

    while (run_thread) {
        fds[0].revents = 0;
        fds[1].revents = 0;

        if (poll(fds, 2, -1) <= 0) {
            continue;
        }
        if (fds[0].revents & POLLIN) {
            LOGV("Received notification to stop uevent thread");
            break;
        }
        if (fds[1].revents & POLLIN) {
            handle_device_fd();
        }
    }

    return nullptr;
}

void device_init(bool dry_run_)
{
    dry_run = dry_run_;

    bootdevice[0] = '\0';
    std::string value;
    if (mb::util::kernel_cmdline_get_option("androidboot.bootdevice", &value)) {
        strlcpy(bootdevice, value.c_str(), sizeof(bootdevice));
    }

    // Is 256K enough? udev uses 16MB!
    device_fd = uevent_open_socket(256 * 1024, true);
    if (device_fd < 0) {
        return;
    }

    fcntl(device_fd, F_SETFL, O_NONBLOCK);

    coldboot("/sys/class");
    coldboot("/sys/block");
    coldboot("/sys/devices");

    run_thread = true;
    pipe(pipe_fd);
    pthread_create(&thread, nullptr, &device_thread, nullptr);
}

void device_close()
{
    run_thread = false;
    write(pipe_fd[1], "", 1);

    pthread_join(thread, nullptr);

    close(device_fd);
    device_fd = -1;
    close(pipe_fd[0]);
    close(pipe_fd[1]);
}

int get_device_fd()
{
    return device_fd;
}

std::unordered_map<std::string, BlockDevInfo> get_block_dev_mappings()
{
    std::lock_guard<std::mutex> lock(block_dev_mappings_guard);
    return block_dev_mappings;
}
