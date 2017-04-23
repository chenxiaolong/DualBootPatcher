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

#include <chrono>
#include <memory>
#include <vector>

#include <cerrno>
#include <cctype>
#include <cinttypes>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include "mbutil/external/_system_properties.h"

#include "mbcommon/string.h"

#include "mblog/logging.h"

#include "mbutil/external/bionic_macros.h"
#include "mbutil/file.h"

#include "property_service.h"

#define ALLOW_LOCAL_PROP_OVERRIDE 1

static int property_set_fd = -1;
static int stop_pipe_fd[2];

static pthread_t thread;

bool property_init() {
    if (mb__system_property_area_init()) {
        LOGE("Failed to initialize property area");
        return false;
    }

    return true;
}

std::string property_get(const char* name) {
    char value[PROP_VALUE_MAX] = {0};
    mb__system_property_get(name, value);
    return value;
}

bool is_legal_property_name(const std::string& name) {
    size_t namelen = name.size();

    if (namelen < 1) return false;
    if (name[0] == '.') return false;
    if (name[namelen - 1] == '.') return false;

    /* Only allow alphanumeric, plus '.', '-', '@', or '_' */
    /* Don't allow ".." to appear in a property name */
    for (size_t i = 0; i < namelen; i++) {
        if (name[i] == '.') {
            // i=0 is guaranteed to never have a dot. See above.
            if (name[i-1] == '.') return false;
            continue;
        }
        if (name[i] == '_' || name[i] == '-' || name[i] == '@') continue;
        if (name[i] >= 'a' && name[i] <= 'z') continue;
        if (name[i] >= 'A' && name[i] <= 'Z') continue;
        if (name[i] >= '0' && name[i] <= '9') continue;
        return false;
    }

    return true;
}

uint32_t property_set(const std::string& name, const std::string& value) {
    size_t valuelen = value.size();

    if (!is_legal_property_name(name)) {
        LOGE("property_set(\"%s\", \"%s\") failed: bad name",
             name.c_str(), value.c_str());
        return PROP_ERROR_INVALID_NAME;
    }

    if (valuelen >= PROP_VALUE_MAX) {
        LOGE("property_set(\"%s\", \"%s\") failed: value too long",
             name.c_str(), value.c_str());
        return PROP_ERROR_INVALID_VALUE;
    }

    prop_info* pi = (prop_info*) mb__system_property_find(name.c_str());
    if (pi != nullptr) {
        // ro.* properties are actually "write-once".
        if (mb_starts_with(name.c_str(), "ro.")) {
            LOGE("property_set(\"%s\", \"%s\") failed: property already set",
                 name.c_str(), value.c_str());
            return PROP_ERROR_READ_ONLY_PROPERTY;
        }

        mb__system_property_update(pi, value.c_str(), valuelen);
    } else {
        int rc = mb__system_property_add(name.c_str(), name.size(), value.c_str(), valuelen);
        if (rc < 0) {
            LOGE("property_set(\"%s\", \"%s\") failed: "
                 "mb__system_property_add failed",
                 name.c_str(), value.c_str());
            return PROP_ERROR_SET_FAILED;
        }
    }

    return PROP_SUCCESS;
}

class SocketConnection {
 public:
  SocketConnection(int socket, const struct ucred& cred)
      : socket_(socket), cred_(cred) {}

  ~SocketConnection() {
    close(socket_);
  }

  bool RecvUint32(uint32_t* value, uint32_t* timeout_ms) {
    return RecvFully(value, sizeof(*value), timeout_ms);
  }

  bool RecvChars(char* chars, size_t size, uint32_t* timeout_ms) {
    return RecvFully(chars, size, timeout_ms);
  }

  bool RecvString(std::string* value, uint32_t* timeout_ms) {
    uint32_t len = 0;
    if (!RecvUint32(&len, timeout_ms)) {
      return false;
    }

    if (len == 0) {
      *value = "";
      return true;
    }

    // http://b/35166374: don't allow init to make arbitrarily large allocations.
    if (len > 0xffff) {
      LOGE("sys_prop: RecvString asked to read huge string: %u", len);
      errno = ENOMEM;
      return false;
    }

    std::vector<char> chars(len);
    if (!RecvChars(&chars[0], len, timeout_ms)) {
      return false;
    }

    *value = std::string(&chars[0], len);
    return true;
  }

  bool SendUint32(uint32_t value) {
    int result = TEMP_FAILURE_RETRY(send(socket_, &value, sizeof(value), 0));
    return result == sizeof(value);
  }

  int socket() {
    return socket_;
  }

  const struct ucred& cred() {
    return cred_;
  }

 private:
  bool PollIn(uint32_t* timeout_ms) {
    struct pollfd ufds[1];
    ufds[0].fd = socket_;
    ufds[0].events = POLLIN;
    ufds[0].revents = 0;
    while (*timeout_ms > 0) {
      std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
      int nr = poll(ufds, 1, *timeout_ms);
      std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
      uint64_t millis = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
      *timeout_ms = (millis > *timeout_ms) ? 0 : *timeout_ms - millis;

      if (nr > 0) {
        return true;
      }

      if (nr == 0) {
        // Timeout
        break;
      }

      if (nr < 0 && errno != EINTR) {
        LOGE("sys_prop: error waiting for uid %u to send property message", cred_.uid);
        return false;
      } else { // errno == EINTR
        // Timer rounds milliseconds down in case of EINTR we want it to be rounded up
        // to avoid slowing init down by causing EINTR with under millisecond timeout.
        if (*timeout_ms > 0) {
          --(*timeout_ms);
        }
      }
    }

    LOGE("sys_prop: timeout waiting for uid %u to send property message.", cred_.uid);
    return false;
  }

  bool RecvFully(void* data_ptr, size_t size, uint32_t* timeout_ms) {
    size_t bytes_left = size;
    char* data = static_cast<char*>(data_ptr);
    while (*timeout_ms > 0 && bytes_left > 0) {
      if (!PollIn(timeout_ms)) {
        return false;
      }

      int result = TEMP_FAILURE_RETRY(recv(socket_, data, bytes_left, MSG_DONTWAIT));
      if (result <= 0) {
        return false;
      }

      bytes_left -= result;
      data += result;
    }

    return bytes_left == 0;
  }

  int socket_;
  struct ucred cred_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(SocketConnection);
};

static void handle_property_set(SocketConnection& socket,
                                const std::string& name,
                                const std::string& value,
                                bool legacy_protocol) {
  const char* cmd_name = legacy_protocol ? "PROP_MSG_SETPROP" : "PROP_MSG_SETPROP2";
  if (!is_legal_property_name(name)) {
    LOGE("sys_prop(%s): illegal property name \"%s\"", cmd_name, name.c_str());
    socket.SendUint32(PROP_ERROR_INVALID_NAME);
    return;
  }

  if (mb_starts_with(name.c_str(), "ctl.")) {
    LOGV("Ignoring control message: %s=%s", name.c_str(), value.c_str());
    if (!legacy_protocol) {
      socket.SendUint32(PROP_SUCCESS);
    }
  } else {
    uint32_t result = property_set(name, value);
    if (!legacy_protocol) {
      socket.SendUint32(result);
    }
  }
}

static void handle_property_set_fd() {
    static constexpr uint32_t kDefaultSocketTimeout = 2000; /* ms */

    int s = accept4(property_set_fd, nullptr, nullptr, SOCK_CLOEXEC);
    if (s == -1) {
        return;
    }

    struct ucred cr;
    socklen_t cr_size = sizeof(cr);
    if (getsockopt(s, SOL_SOCKET, SO_PEERCRED, &cr, &cr_size) < 0) {
        close(s);
        LOGE("sys_prop: unable to get SO_PEERCRED");
        return;
    }

    SocketConnection socket(s, cr);
    uint32_t timeout_ms = kDefaultSocketTimeout;

    uint32_t cmd = 0;
    if (!socket.RecvUint32(&cmd, &timeout_ms)) {
        LOGE("sys_prop: error while reading command from the socket");
        socket.SendUint32(PROP_ERROR_READ_CMD);
        return;
    }

    switch (cmd) {
    case PROP_MSG_SETPROP: {
        char prop_name[PROP_NAME_MAX];
        char prop_value[PROP_VALUE_MAX];

        if (!socket.RecvChars(prop_name, PROP_NAME_MAX, &timeout_ms) ||
            !socket.RecvChars(prop_value, PROP_VALUE_MAX, &timeout_ms)) {
          LOGE("sys_prop(PROP_MSG_SETPROP): error while reading name/value from the socket");
          return;
        }

        prop_name[PROP_NAME_MAX-1] = 0;
        prop_value[PROP_VALUE_MAX-1] = 0;

        handle_property_set(socket, prop_value, prop_value, true);
        break;
      }

    case PROP_MSG_SETPROP2: {
        std::string name;
        std::string value;
        if (!socket.RecvString(&name, &timeout_ms) ||
            !socket.RecvString(&value, &timeout_ms)) {
          LOGE("sys_prop(PROP_MSG_SETPROP2): error while reading name/value from the socket");
          socket.SendUint32(PROP_ERROR_READ_DATA);
          return;
        }

        handle_property_set(socket, name, value, false);
        break;
      }

    default:
        LOGE("sys_prop: invalid command %u", cmd);
        socket.SendUint32(PROP_ERROR_INVALID_CMD);
        break;
    }
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

        while (isspace(*key)) key++;
        if (*key == '#') continue;

        tmp = eol - 2;
        while ((tmp > key) && isspace(*tmp)) *tmp-- = 0;

        if (!strncmp(key, "import ", 7) && flen == 0) {
            fn = key + 7;
            while (isspace(*fn)) fn++;

            key = strchr(fn, ' ');
            if (key) {
                *key++ = 0;
                while (isspace(*key)) key++;
            }

            load_properties_from_file(fn, key);

        } else {
            value = strchr(key, '=');
            if (!value) continue;
            *value++ = 0;

            tmp = value - 2;
            while ((tmp > key) && isspace(*tmp)) *tmp-- = 0;

            while (isspace(*value)) value++;

            if (flen > 0) {
                if (filter[flen - 1] == '*') {
                    if (strncmp(key, filter, flen - 1)) continue;
                } else {
                    if (strcmp(key, filter)) continue;
                }
            }

            property_set(key, value);
        }
    }
}

// Filter is used to decide which properties to load: NULL loads all keys,
// "ro.foo.*" is a prefix match, and "ro.foo.bar" is an exact match.
static void load_properties_from_file(const char* filename, const char* filter) {
    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

    std::vector<unsigned char> data;
    if (!mb::util::file_read_all(filename, &data)) {
        LOGW("Couldn't load properties from %s", filename);
        return;
    }
    data.push_back('\n');
    data.push_back('\0');
    load_properties(reinterpret_cast<char *>(data.data()), filter);

    std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = end - start;

    LOGV("(Loading properties from %s took %.2fs.)", filename, elapsed.count());
}

void property_load_boot_defaults() {
    load_properties_from_file("/default.prop", NULL);
    load_properties_from_file("/odm/default.prop", NULL);
    load_properties_from_file("/vendor/default.prop", NULL);
}

void load_system_props() {
    load_properties_from_file("/system/build.prop", NULL);
    load_properties_from_file("/odm/build.prop", NULL);
    load_properties_from_file("/vendor/build.prop", NULL);
    load_properties_from_file("/factory/factory.prop", "ro.*");
}

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

bool start_property_service() {
    if (property_set_fd >= 0) {
        LOGW("Property service has already started");
        return false;
    }

    property_set("ro.property_service.version", "2");

    property_set_fd = create_socket(PROP_SERVICE_NAME, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK,
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
