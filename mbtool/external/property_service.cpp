/*
 * Copyright (C) 2007 The Android Open Source Project
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

#include "property_service.h"

#include <chrono>

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/file.h"

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include "mbutil/external/_system_properties.h"

#define PERSISTENT_PROPERTY_DIR  "/data/property"
#define FSTAB_PREFIX "/fstab."
#define RECOVERY_MOUNT_POINT "/recovery"

static int persistent_properties_loaded = 0;
static bool property_area_initialized = false;

static int property_set_fd = -1;
static int stop_pipe_fd[2];

static pthread_t thread;

struct workspace {
    size_t size;
    int fd;
};

static workspace pa_workspace;

bool property_init()
{
    if (property_area_initialized) {
        return false;
    }

    if (mb__system_property_area_init()) {
        return false;
    }

    pa_workspace.size = 0;
    pa_workspace.fd = open(PROP_FILENAME, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (pa_workspace.fd == -1) {
        LOGE("Failed to open %s: %s", PROP_FILENAME, strerror(errno));
        return false;
    }

    property_area_initialized = true;

    return true;
}

bool property_cleanup()
{
    if (!property_area_initialized) {
        return false;
    }

    close(pa_workspace.fd);
    pa_workspace.fd = -1;
    pa_workspace.size = 0;

    property_area_initialized = false;

    return true;
}

int __property_get(const char *name, char *value)
{
    return mb__system_property_get(name, value);
}

bool property_get_bool(const char *key, bool default_value)
{
    if (!key) {
        return default_value;
    }

    bool result = default_value;
    char buf[PROP_VALUE_MAX] = {'\0',};

    int len = __property_get(key, buf);
    if (len == 1) {
        char ch = buf[0];
        if (ch == '0' || ch == 'n') {
            result = false;
        } else if (ch == '1' || ch == 'y') {
            result = true;
        }
    } else if (len > 1) {
         if (!strcmp(buf, "no") || !strcmp(buf, "false") || !strcmp(buf, "off")) {
            result = false;
        } else if (!strcmp(buf, "yes") || !strcmp(buf, "true") || !strcmp(buf, "on")) {
            result = true;
        }
    }

    return result;
}

static void write_persistent_property(const char *name, const char *value)
{
    char tempPath[PATH_MAX];
    char path[PATH_MAX];
    int fd;

    snprintf(tempPath, sizeof(tempPath),
             "%s/.temp.XXXXXX", PERSISTENT_PROPERTY_DIR);
    fd = mkstemp(tempPath);
    if (fd < 0) {
        LOGE("Unable to write persistent property to temp file %s: %s",
             tempPath, strerror(errno));
        return;
    }
    write(fd, value, strlen(value));
    fsync(fd);
    close(fd);

    snprintf(path, sizeof(path), "%s/%s", PERSISTENT_PROPERTY_DIR, name);
    if (rename(tempPath, path)) {
        unlink(tempPath);
        LOGE("Unable to rename persistent property file %s to %s",
             tempPath, path);
    }
}

static bool is_legal_property_name(const char *name, size_t namelen)
{
    size_t i;
    if (namelen >= PROP_NAME_MAX) return false;
    if (namelen < 1) return false;
    if (name[0] == '.') return false;
    if (name[namelen - 1] == '.') return false;

    /* Only allow alphanumeric, plus '.', '-', or '_' */
    /* Don't allow ".." to appear in a property name */
    for (i = 0; i < namelen; i++) {
        if (name[i] == '.') {
            // i=0 is guaranteed to never have a dot. See above.
            if (name[i-1] == '.') return false;
            continue;
        }
        if (name[i] == '_' || name[i] == '-') continue;
        if (name[i] >= 'a' && name[i] <= 'z') continue;
        if (name[i] >= 'A' && name[i] <= 'Z') continue;
        if (name[i] >= '0' && name[i] <= '9') continue;
        return false;
    }

    return true;
}

static int property_set_impl(const char *name, const char *value)
{
    size_t namelen = strlen(name);
    size_t valuelen = strlen(value);

    if (!is_legal_property_name(name, namelen)) {
        return -1;
    }
    if (valuelen >= PROP_VALUE_MAX) {
        return -1;
    }

    prop_info *pi = (prop_info *) mb__system_property_find(name);

    if (pi != 0) {
        /* ro.* properties may NEVER be modified once set */
        if (!strncmp(name, "ro.", 3)) {
            return -1;
        }

        mb__system_property_update(pi, value, valuelen);
    } else {
        int rc = mb__system_property_add(name, namelen, value, valuelen);
        if (rc < 0) {
            return rc;
        }
    }
    /* If name starts with "net." treat as a DNS property. */
    if (strncmp("net.", name, strlen("net.")) == 0)  {
        if (strcmp("net.change", name) == 0) {
            return 0;
        }
        /*
         * The 'net.change' property is a special property used track when any
         * 'net.*' property name is updated. It is _ONLY_ updated here. Its value
         * contains the last updated 'net.*' property.
         */
        property_set("net.change", name);
    } else if (persistent_properties_loaded
            && strncmp("persist.", name, strlen("persist.")) == 0) {
        /*
         * Don't write properties to disk until after we have read all default properties
         * to prevent them from being overwritten by default values.
         */
        write_persistent_property(name, value);
    }
    return 0;
}

int property_set(const char *name, const char *value)
{
    int rc = property_set_impl(name, value);
    if (rc == -1) {
        LOGE("property_set(\"%s\", \"%s\") failed", name, value);
    }
    return rc;
}

static void handle_property_set_fd()
{
    prop_msg msg;
    int s;
    int r;
    struct ucred cr;
    struct sockaddr_un addr;
    socklen_t addr_size = sizeof(addr);
    socklen_t cr_size = sizeof(cr);
    struct pollfd ufds[1];
    const int timeout_ms = 2 * 1000;  /* Default 2 sec timeout for caller to send property. */
    int nr;

    if ((s = accept(property_set_fd, (struct sockaddr *) &addr, &addr_size)) < 0) {
        return;
    }

    /* Check socket options here */
    if (getsockopt(s, SOL_SOCKET, SO_PEERCRED, &cr, &cr_size) < 0) {
        close(s);
        LOGE("Unable to receive socket options");
        return;
    }

    ufds[0].fd = s;
    ufds[0].events = POLLIN;
    ufds[0].revents = 0;
    nr = TEMP_FAILURE_RETRY(poll(ufds, 1, timeout_ms));
    if (nr == 0) {
        LOGE("sys_prop: timeout waiting for uid=%d to send property message.", cr.uid);
        close(s);
        return;
    } else if (nr < 0) {
        LOGE("sys_prop: error waiting for uid=%d to send property message: %s",
             cr.uid, strerror(errno));
        close(s);
        return;
    }

    r = TEMP_FAILURE_RETRY(recv(s, &msg, sizeof(msg), MSG_DONTWAIT));
    if (r != sizeof(prop_msg)) {
        LOGE("sys_prop: mis-match msg size received: %d expected: %zu: %s",
             r, sizeof(prop_msg), strerror(errno));
        close(s);
        return;
    }

    switch (msg.cmd) {
    case PROP_MSG_SETPROP:
        msg.name[PROP_NAME_MAX - 1] = 0;
        msg.value[PROP_VALUE_MAX - 1] = 0;

        if (!is_legal_property_name(msg.name, strlen(msg.name))) {
            LOGE("sys_prop: illegal property name. Got: \"%s\"", msg.name);
            close(s);
            return;
        }

        if (memcmp(msg.name, "ctl.", 4) == 0) {
            // Keep the old close-socket-early behavior when handling
            // ctl.* properties.
            close(s);
            LOGV("Ignoring control message: %s=%s", msg.name, msg.value);
        } else {
            property_set((char *) msg.name, (char *) msg.value);

            // Note: bionic's property client code assumes that the
            // property server will not close the socket until *AFTER*
            // the property is written to memory.
            close(s);
        }
        break;

    default:
        close(s);
        break;
    }
}

void get_property_workspace(int *fd, int *sz)
{
    *fd = pa_workspace.fd;
    *sz = pa_workspace.size;
}

static void load_properties_from_file(const char *, const char *);

/*
 * Filter is used to decide which properties to load: NULL loads all keys,
 * "ro.foo.*" is a prefix match, and "ro.foo.bar" is an exact match.
 */
static void load_properties(char *data, const char *filter)
{
    char *key, *value, *eol, *sol, *tmp, *fn;
    size_t flen = 0;

    if (filter) {
        flen = strlen(filter);
    }

    sol = data;
    while ((eol = strchr(sol, '\n'))) {
        key = sol;
        *eol++ = 0;
        sol = eol;

        while (isspace(*key)) {
            key++;
        }
        if (*key == '#') {
            continue;
        }

        tmp = eol - 2;
        while ((tmp > key) && isspace(*tmp)) {
            *tmp-- = 0;
        }

        if (!strncmp(key, "import ", 7) && flen == 0) {
            fn = key + 7;
            while (isspace(*fn)) {
                fn++;
            }

            key = strchr(fn, ' ');
            if (key) {
                *key++ = 0;
                while (isspace(*key)) {
                    key++;
                }
            }

            load_properties_from_file(fn, key);
        } else {
            value = strchr(key, '=');
            if (!value) {
                continue;
            }
            *value++ = 0;

            tmp = value - 2;
            while ((tmp > key) && isspace(*tmp)) {
                *tmp-- = 0;
            }

            while (isspace(*value)) {
                value++;
            }

            if (flen > 0) {
                if (filter[flen - 1] == '*') {
                    if (strncmp(key, filter, flen - 1)) {
                        continue;
                    }
                } else {
                    if (strcmp(key, filter)) {
                        continue;
                    }
                }
            }

            property_set(key, value);
        }
    }
}

/*
 * Filter is used to decide which properties to load: NULL loads all keys,
 * "ro.foo.*" is a prefix match, and "ro.foo.bar" is an exact match.
 */
static void load_properties_from_file(const char *filename, const char *filter)
{
    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

    std::vector<unsigned char> data;
    if (mb::util::file_read_all(filename, &data)) {
        data.push_back('\n');
        data.push_back('\0');
        load_properties((char *) data.data(), filter);
    }

    std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    LOGI("(Loading properties from %s took %.2fs.)", filename, elapsed.count());
}

void property_load_boot_defaults()
{
    load_properties_from_file(PROP_PATH_RAMDISK_DEFAULT, nullptr);
}

bool properties_initialized()
{
    return property_area_initialized;
}

void load_system_props()
{
    load_properties_from_file(PROP_PATH_SYSTEM_BUILD, nullptr);
    load_properties_from_file(PROP_PATH_VENDOR_BUILD, nullptr);
    load_properties_from_file(PROP_PATH_FACTORY, "ro.*");
}

#define ANDROID_SOCKET_DIR           "/dev/socket"

static int create_socket(const char *name, int type, mode_t perm, uid_t uid,
                         gid_t gid)
{
    struct sockaddr_un addr;
    int fd, ret;

    fd = socket(PF_UNIX, type, 0);
    if (fd < 0) {
        LOGE("Failed to open socket '%s': %s", name, strerror(errno));
        return -1;
    }

    memset(&addr, 0 , sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), ANDROID_SOCKET_DIR"/%s",
             name);

    ret = unlink(addr.sun_path);
    if (ret != 0 && errno != ENOENT) {
        LOGE("Failed to unlink old socket '%s': %s", name, strerror(errno));
        goto out_close;
    }

    ret = bind(fd, (struct sockaddr *) &addr, sizeof (addr));
    if (ret) {
        LOGE("Failed to bind socket '%s': %s", name, strerror(errno));
        goto out_unlink;
    }

    chown(addr.sun_path, uid, gid);
    chmod(addr.sun_path, perm);

    LOGI("Created socket '%s' with mode '%o', user '%d', group '%d'\n",
         addr.sun_path, perm, uid, gid);

    return fd;

out_unlink:
    unlink(addr.sun_path);
out_close:
    close(fd);
    return -1;
}

#undef ANDROID_SOCKET_DIR

void * property_service_thread(void *)
{
    struct pollfd fds[2];
    fds[0].fd = stop_pipe_fd[0];
    fds[0].events = POLLIN;
    fds[1].fd = property_set_fd;
    fds[1].events = POLLIN;

    while (true) {
        fds[0].revents = 0;
        fds[1].revents = 0;

        if (poll(fds, 2, -1) <= 0) {
            continue;
        }
        if (fds[0].revents & POLLIN) {
            LOGV("Received notification to stop property service");
            break;
        }
        if (fds[1].revents & POLLIN) {
            handle_property_set_fd();
        }
    }

    return nullptr;
}

bool start_property_service()
{
    if (property_set_fd >= 0) {
        LOGW("Property service has already started");
        return false;
    }

    property_set_fd = create_socket(PROP_SERVICE_NAME,
                                    SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK,
                                    0666, 0, 0);
    if (property_set_fd == -1) {
        LOGE("start_property_service socket creation failed: %s",
             strerror(errno));
        goto error;
    }

    if (listen(property_set_fd, 8) < 0) {
        LOGE("Failed to listen on socket: %s", strerror(errno));
        goto error;
    }

    if (pipe(stop_pipe_fd) < 0) {
        LOGE("Failed to create pipe: %s", strerror(errno));
        goto error;
    }

    if (pthread_create(&thread, nullptr, &property_service_thread, nullptr) < 0) {
        LOGE("Failed to start property service thread: %s", strerror(errno));
        goto error;
    }

    LOGD("Started property service");

    return true;

error:
    if (property_set_fd >= 0) {
        close(property_set_fd);
        property_set_fd = -1;
    }
    return false;
}

bool stop_property_service()
{
    if (property_set_fd < 0) {
        LOGW("Property service has not yet started");
        return false;
    }

    write(stop_pipe_fd[1], "", 1);

    pthread_join(thread, nullptr);

    close(property_set_fd);
    property_set_fd = -1;

    close(stop_pipe_fd[0]);
    close(stop_pipe_fd[1]);

    return true;
}
