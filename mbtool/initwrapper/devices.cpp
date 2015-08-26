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

#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <poll.h>
#include <pthread.h>
#include <unistd.h>

#include "initwrapper/cutils/uevent.h"
#include "initwrapper/util.h"
#include "util/logging.h"
#include "util/string.h"

#define UNUSED __attribute__((__unused__))

static int device_fd = -1;
static volatile bool run_thread = true;
static pthread_t thread;

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

struct perms_ {
    char *name;
    char *attr;
    mode_t perm;
    unsigned int uid;
    unsigned int gid;
    unsigned short prefix;
    unsigned short wildcard;
};

struct platform_node {
    char *name;
    char *path;
    int path_len;
};

static std::vector<perms_> sys_perms;
static std::vector<perms_> dev_perms;
static std::vector<platform_node> platform_names;

static std::unordered_map<std::string, std::string> device_map;

void fixup_sys_perms(const char *upath)
{
    char buf[512];

    // upaths omit the "/sys" that paths in this list
    // contain, so we add 4 when comparing...
    for (const perms_ &dp : sys_perms) {
        if (dp.prefix) {
            if (strncmp(upath, dp.name + 4, strlen(dp.name + 4)) != 0) {
                continue;
            }
        } else if (dp.wildcard) {
            if (fnmatch(dp.name + 4, upath, FNM_PATHNAME) != 0) {
                continue;
            }
        } else {
            if (strcmp(upath, dp.name + 4) != 0) {
                continue;
            }
        }

        if ((strlen(upath) + strlen(dp.attr) + 6) > sizeof(buf)) {
            break;
        }

        sprintf(buf,"/sys%s/%s", upath, dp.attr);
        LOGI("fixup %s %d %d 0%o", buf, dp.uid, dp.gid, dp.perm);
        chown(buf, dp.uid, dp.gid);
        chmod(buf, dp.perm);
    }

    // Now fixup SELinux file labels
    int len = snprintf(buf, sizeof(buf), "/sys%s", upath);
    if ((len < 0) || ((size_t) len >= sizeof(buf))) {
        // Overflow
        return;
    }
}

static bool perm_path_matches(const char *path, const struct perms_ *dp)
{
    if (dp->prefix) {
        if (strncmp(path, dp->name, strlen(dp->name)) == 0) {
            return true;
        }
    } else if (dp->wildcard) {
        if (fnmatch(dp->name, path, FNM_PATHNAME) == 0) {
            return true;
        }
    } else {
        if (strcmp(path, dp->name) == 0) {
            return true;
        }
    }

    return false;
}

static mode_t get_device_perm(const char *path,
                              const std::vector<std::string> &links,
                              unsigned *uid, unsigned *gid)
{
    // Search the perms list in reverse so that ueventd.$hardware can
    // override ueventd.rc
    for (auto it = dev_perms.rbegin(); it != dev_perms.rend(); ++it) {
        bool match = false;

        const struct perms_ &dp = *it;

        if (perm_path_matches(path, &dp)) {
            match = true;
        } else {
            for (const std::string &link : links) {
                if (perm_path_matches(link.c_str(), &dp)) {
                    match = true;
                    break;
                }
            }
        }

        if (match) {
            *uid = dp.uid;
            *gid = dp.gid;
            return dp.perm;
        }
    }
    // Default if nothing found.
    *uid = 0;
    *gid = 0;
    return 0600;
}

static void make_device(const char *path,
                        const char *upath UNUSED,
                        int block, int major, int minor,
                        const std::vector<std::string> &links)
{
    unsigned uid;
    unsigned gid;
    mode_t mode;
    dev_t dev;

    mode = get_device_perm(path, links, &uid, &gid) | (block ? S_IFBLK : S_IFCHR);

    dev = makedev(major, minor);
    // Temporarily change egid to avoid race condition setting the gid of the
    // device node. Unforunately changing the euid would prevent creation of
    // some device nodes, so the uid has to be set with chown() and is still
    // racy. Fixing the gid race at least fixed the issue with system_server
    // opening dynamic input devices under the AID_INPUT gid.
    setegid(gid);
    mknod(path, mode, dev);
    chown(path, uid, -1);
    setegid(0);
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

    for (auto it = platform_names.rbegin(); it != platform_names.rend(); ++it) {
        const struct platform_node &bus = *it;

        if ((bus.path_len < path_len) &&
                (path[bus.path_len] == '/') &&
                strncmp(path, bus.path, bus.path_len) == 0) {
            // Subdevice of an existing platform, ignore it
            return;
        }
    }

    LOGI("adding platform device %s (%s)", name, path);

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
static struct platform_node * find_platform_device(const char *path)
{
    int path_len = strlen(path);

    for (auto it = platform_names.rbegin(); it != platform_names.rend(); ++it) {
        struct platform_node &bus = *it;

        if ((bus.path_len < path_len) &&
                (path[bus.path_len] == '/') &&
                strncmp(path, bus.path, bus.path_len) == 0)
            return &bus;
    }

    return nullptr;
}

static void remove_platform_device(const char *path)
{
    for (auto it = platform_names.rbegin(); it != platform_names.rend(); ++it) {
        const struct platform_node &bus = *it;

        if (strcmp(path, bus.path) == 0) {
            LOGI("Removing platform device %s", bus.name);
            free(bus.path);
            platform_names.erase((it + 1).base());
            return;
        }
    }
}

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
        }

        // Advance to after the next \0
        while (*msg++);
    }

    LOGD("event { '%s', '%s', '%s', '%s', %d, %d }",
         uevent->action, uevent->path, uevent->subsystem,
         uevent->firmware, uevent->major, uevent->minor);
}

static std::vector<std::string> get_block_device_symlinks(struct uevent *uevent)
{
    const char *device;
    struct platform_node *pdev;
    char *slash;
    const char *type;
    char link_path[256];
    char *p;

    pdev = find_platform_device(uevent->path);
    if (pdev) {
        device = pdev->name;
        type = "platform";
    } else {
        return std::vector<std::string>();
    }

    std::vector<std::string> links;

    LOGD("Found %s device %s", type, device);

    snprintf(link_path, sizeof(link_path), "/dev/block/%s/%s", type, device);

    if (uevent->partition_name) {
        p = strdup(uevent->partition_name);
        sanitize(p);
        if (strcmp(uevent->partition_name, p) != 0) {
            LOGV("Linking partition '%s' as '%s'", uevent->partition_name, p);
        }
        links.push_back(mb::util::format("%s/by-name/%s", link_path, p));
        free(p);
    }

    if (uevent->partition_num >= 0) {
        links.push_back(mb::util::format(
                "%s/by-num/p%d", link_path, uevent->partition_num));
    }

    slash = strrchr(uevent->path, '/');
    links.push_back(mb::util::format("%s/%s", link_path, slash + 1));

    return links;
}

static void handle_device(const char *action, const char *devpath,
                          const char *path, int block, int major, int minor,
                          const std::vector<std::string> &links)
{
    if (strcmp(action, "add") == 0) {
        device_map[path] = devpath;
        make_device(devpath, path, block, major, minor, links);
        for (const std::string &link : links) {
            make_link_init(devpath, link.c_str());
        }
    }

    if (strcmp(action, "remove") == 0) {
        device_map.erase(path);
        for (const std::string &link : links) {
            remove_link(devpath, link.c_str());
        }
        unlink(devpath);
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
        LOGE("DEVPATH=%s exceeds %u-character limit on filename; ignoring event",
             name, len);
        return nullptr;
    }

    return name;
}

static void handle_block_device_event(struct uevent *uevent)
{
    const char *base = "/dev/block/";
    const char *name;
    char devpath[96];
    std::vector<std::string> links;

    name = parse_device_name(uevent, 64);
    if (!name) {
        return;
    }

    snprintf(devpath, sizeof(devpath), "%s%s", base, name);
    mkdir(base, 0755);

    if (strncmp(uevent->path, "/devices/", 9) == 0) {
        links = get_block_device_symlinks(uevent);
    }

    handle_device(uevent->action, devpath, uevent->path, 1,
            uevent->major, uevent->minor, links);
}

static void handle_generic_device_event(struct uevent *uevent)
{
    const char *base = "/dev/";
    const char *name;
    char devpath[96];
    std::vector<std::string> links;

    name = parse_device_name(uevent, 64);
    if (!name) {
        return;
    }

    if (strcmp(uevent->subsystem, "misc") == 0) {
        if (strcmp(name, "loop-control") != 0
                && strcmp(name, "fuse") != 0) {
            return;
        }
    } else if (strcmp(uevent->subsystem, "mem") == 0) {
        if (strcmp(name, "null") != 0) {
            return;
        }
    }

    snprintf(devpath, sizeof(devpath), "%s%s", base, name);

    handle_device(uevent->action, devpath, uevent->path, 0,
            uevent->major, uevent->minor, links);
}

static void handle_device_event(struct uevent *uevent)
{
    if (strcmp(uevent->action, "add") == 0
            || strcmp(uevent->action, "change") == 0
            || strcmp(uevent->action, "online") == 0) {
        fixup_sys_perms(uevent->path);
    }

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
    struct pollfd ufd;
    ufd.events = POLLIN;
    ufd.fd = get_device_fd();

    while (run_thread) {
        ufd.revents = 0;
        if (poll(&ufd, 1, -1) <= 0) {
            continue;
        }
        if (ufd.revents & POLLIN) {
            handle_device_fd();
        }
    }

    return nullptr;
}

void device_init()
{
    // Is 256K enough? udev uses 16MB!
    device_fd = uevent_open_socket(256 * 1024, true);
    if (device_fd < 0) {
        return;
    }

    fcntl(device_fd, F_SETFD, FD_CLOEXEC);
    fcntl(device_fd, F_SETFL, O_NONBLOCK);

    coldboot("/sys/class");
    coldboot("/sys/block");
    coldboot("/sys/devices");

    run_thread = true;
    pthread_create(&thread, nullptr, &device_thread, nullptr);
}

void device_close()
{
    run_thread = false;
    pthread_join(thread, nullptr);

    close(device_fd);
    device_fd = -1;
}

int get_device_fd()
{
    return device_fd;
}

const std::unordered_map<std::string, std::string> * get_devices_map()
{
    return &device_map;
}
