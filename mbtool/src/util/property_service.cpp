/*
 * Copyright (C) 2007 The Android Open Source Project
 * Copyright (C) 2018 Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/property_service.h"

#include <chrono>
#include <string_view>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include "mbutil/external/_system_properties.h"

#include "mbcommon/error.h"
#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/string.h"

#include "mblog/logging.h"

#include "mbutil/external/bionic_macros.h"
#include "mbutil/properties.h"

#include "util/multiboot.h"

#define LOG_TAG "mbtool/util/property_service"

class SocketConnection
{
public:
    SocketConnection(int fd)
        : m_fd(fd)
    {
    }

    ~SocketConnection()
    {
        close(m_fd);
    }

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(SocketConnection)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(SocketConnection)

    int fd()
    {
        return m_fd;
    }

    mb::oc::result<void> recv_data(void *data, size_t size,
                                   std::chrono::milliseconds &timeout)
    {
        char *ptr = static_cast<char *>(data);
        size_t remain = size;

        while (timeout.count() > 0 && remain > 0) {
            OUTCOME_TRYV(poll_socket(timeout));

            auto result = TEMP_FAILURE_RETRY(
                    recv(m_fd, ptr, remain, MSG_DONTWAIT));
            if (result < 0) {
                return mb::ec_from_errno();
            } else if (result == 0) {
                break;
            }

            ptr += result;
            remain -= result;
        }

        if (remain > 0) {
            return std::errc::io_error;
        }

        return mb::oc::success();
    }


    mb::oc::result<uint32_t> recv_uint32(std::chrono::milliseconds &timeout)
    {
        uint32_t value;
        OUTCOME_TRYV(recv_data(&value, sizeof(value), timeout));
        return value;
    }

    mb::oc::result<std::string> recv_string(std::chrono::milliseconds &timeout)
    {
        OUTCOME_TRY(len, recv_uint32(timeout));
        if (len == 0) {
            return "";
        } else if (len > 0xffff) {
            return std::errc::not_enough_memory;
        }

        std::string str;
        str.resize(len);

        OUTCOME_TRYV(recv_data(str.data(), str.size(), timeout));

        return std::move(str);
    }

    mb::oc::result<void> send_uint32(uint32_t value)
    {
        auto result = TEMP_FAILURE_RETRY(
                send(m_fd, &value, sizeof(value), 0));
        if (result < 0) {
            return mb::ec_from_errno();
        } else if (result != sizeof(value)) {
            return std::errc::io_error;
        }

        return mb::oc::success();
    }

private:
    int m_fd;

    mb::oc::result<void> poll_socket(std::chrono::milliseconds &timeout)
    {
        using namespace std::chrono;

        pollfd pfd;
        pfd.fd = m_fd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        while (timeout.count() > 0) {
            auto start = steady_clock::now();
            int nr = poll(&pfd, 1, timeout.count());

            if (nr > 0) {
                return mb::oc::success();
            } else if (nr == 0) {
                break;
            } else if (errno != EINTR) {
                return mb::ec_from_errno();
            }

            auto end = steady_clock::now();
            auto elapsed = duration_cast<milliseconds>(end - start);

            timeout = (elapsed > timeout) ? 0ms : (timeout - elapsed);
        }

        LOGW("Socket timed out waiting for data");
        return std::errc::timed_out;
    }
};

PropertyService::PropertyService()
    : m_initialized(false)
    , m_setter_fd(-1)
    , m_stopper_pipe{-1, -1}
{
}

PropertyService::~PropertyService()
{
    if (m_setter_fd >= 0) {
        stop_thread();
    }
}

bool PropertyService::initialize()
{
    if (m_initialized) {
        return false;
    }

    if (mb::__system_property_area_init() != 0) {
        return false;
    }

    return m_initialized = true;
}

std::optional<std::string> PropertyService::get(const std::string &name)
{
    if (auto const pi = mb::__system_property_find(name.c_str())) {
        std::string value;
        mb::__system_property_read_callback(
                pi, [](void *cookie, const char *, const char *v, uint32_t) {
            *static_cast<std::string *>(cookie) = v;
        }, &value);
        return value;
    } else {
        return std::nullopt;
    }
}

static bool is_legal_property_name(const std::string_view &name)
{
    if (name.empty() || name.front() == '.' || name.back() == '.') {
        return false;
    }

    /* Only allow alphanumeric, plus '.', '-', '@', ':', or '_' */
    /* Don't allow ".." to appear in a property name */
    for (auto it = name.cbegin(); it != name.cend(); ++it) {
        if (*it == '.') {
            // i=0 is guaranteed to never have a dot. See above.
            if (*(it - 1) == '.') {
                return false;
            }
            continue;
        }
        if (*it == '_' || *it == '-' || *it == '@' || *it == ':'
                || (*it >= 'a' && *it <= 'z')
                || (*it >= 'A' && *it <= 'Z')
                || (*it >= '0' && *it <= '9')) {
            continue;
        }
        return false;
    }

    return true;
}

uint32_t PropertyService::set_internal(const std::string &name,
                                       std::string_view value)
{
    if (!is_legal_property_name(name)) {
        LOGE("['%s'='%s'] Failed to set property: Invalid name",
             name.c_str(), std::string(value).c_str());
        return PROP_ERROR_INVALID_NAME;
    }

    if (value.size() >= PROP_VALUE_MAX) {
        LOGE("['%s'='%s'] Failed to set property: Value too long",
             name.c_str(), std::string(value).c_str());
        return PROP_ERROR_INVALID_VALUE;
    }

    if (auto pi = const_cast<mb::prop_info *>(
            mb::__system_property_find(name.c_str()))) {
        // ro.* properties are actually "write-once".
        if (mb::starts_with(name, "ro.")) {
            LOGE("['%s'='%s'] Failed to set property: Property already set",
                 name.c_str(), std::string(value).c_str());
            return PROP_ERROR_READ_ONLY_PROPERTY;
        }

        mb::__system_property_update(pi, value.data(), value.size());
    } else {
        int rc = mb::__system_property_add(name.data(), name.size(),
                                           value.data(), value.size());
        if (rc < 0) {
            LOGE("['%s'='%s'] Failed to add property",
                 name.c_str(), std::string(value).c_str());
            return PROP_ERROR_SET_FAILED;
        }
    }

    return PROP_SUCCESS;
}

bool PropertyService::set(const std::string &name, std::string_view value)
{
    return set_internal(name, value) == PROP_SUCCESS;
}

bool PropertyService::start_thread()
{
    if (m_setter_fd >= 0) {
        LOGW("Property service has already been started");
        return false;
    }

    set("ro.property_service.version", "2");

    m_setter_fd = create_socket();
    if (m_setter_fd == -1) {
        return false;
    }

    auto close_fd = mb::finally([&] {
        close(std::exchange(m_setter_fd, -1));
    });

    if (listen(m_setter_fd, 8) < 0) {
        LOGE("Failed to listen on socket: %s", strerror(errno));
        return false;
    }

    if (pipe2(m_stopper_pipe, O_CLOEXEC) < 0) {
        LOGE("Failed to create pipe: %s", strerror(errno));
        return false;
    }

    m_thread = std::thread(&PropertyService::socket_handler_loop, this);

    LOGD("Started property service");

    close_fd.dismiss();

    return true;
}

bool PropertyService::stop_thread()
{
    if (m_setter_fd < 0) {
        LOGW("Property service has not been started");
        return false;
    }

    write(m_stopper_pipe[1], "", 1);

    m_thread.join();

    close(std::exchange(m_setter_fd, -1));
    close(std::exchange(m_stopper_pipe[0], -1));
    close(std::exchange(m_stopper_pipe[1], -1));

    return true;
}

int PropertyService::create_socket()
{
    int fd = socket(PF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        LOGE("Failed to create socket: %s", strerror(errno));
        return -1;
    }

    auto close_fd = mb::finally([&] {
        mb::ErrorRestorer restorer;
        close(fd);
    });

    sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strlcpy(addr.sun_path, "/dev/socket/" PROP_SERVICE_NAME,
            sizeof(addr.sun_path));

    if (unlink(addr.sun_path) != 0 && errno != ENOENT) {
        LOGE("%s: Failed to unlink old socket: %s",
             addr.sun_path, strerror(errno));
        return -1;
    }

    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        LOGE("%s: Failed to bind socket: %s", addr.sun_path, strerror(errno));
        return -1;
    }

    auto unlink_socket = mb::finally([&] {
        mb::ErrorRestorer restorer;
        unlink(addr.sun_path);
    });

    if (fchmodat(AT_FDCWD, addr.sun_path, 0666, AT_SYMLINK_NOFOLLOW) < 0) {
        LOGE("%s: Failed to chmod socket: %s", addr.sun_path, strerror(errno));
        return -1;
    }

    LOGD("%s: Successfully created socket", addr.sun_path);

    close_fd.dismiss();
    unlink_socket.dismiss();

    return fd;
}

void PropertyService::socket_handler_loop()
{
    pollfd fds[2];
    fds[0].fd = m_stopper_pipe[0];
    fds[0].events = POLLIN;
    fds[1].fd = m_setter_fd;
    fds[1].events = POLLIN;

    while (true) {
        fds[0].revents = 0;
        fds[1].revents = 0;

        if (poll(fds, 2, -1) < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                LOGE("Failed to poll socket: %s", strerror(errno));
                return;
            }
        }
        if (fds[0].revents & POLLIN) {
            LOGV("Received notification to stop property service");
            break;
        }
        if (fds[1].revents & POLLIN) {
            socket_handle_set_property();
        }
    }
}

void PropertyService::socket_handle_set_property()
{
    static constexpr std::chrono::milliseconds DEFAULT_TIMEOUT(2000);

    int fd = accept4(m_setter_fd, nullptr, nullptr, SOCK_CLOEXEC);
    if (fd == -1) {
        LOGE("Failed to accept socket connection: %s", strerror(errno));
        return;
    }

    SocketConnection socket(fd);
    auto timeout = DEFAULT_TIMEOUT;

    auto cmd = socket.recv_uint32(timeout);
    if (!cmd) {
        LOGE("Failed to read command from socket: %s",
             cmd.error().message().c_str());
        (void) socket.send_uint32(PROP_ERROR_READ_CMD);
        return;
    }

    switch (cmd.value()) {
    case PROP_MSG_SETPROP: {
        char name[PROP_NAME_MAX];
        char value[PROP_VALUE_MAX];

        if (auto r = socket.recv_data(name, PROP_NAME_MAX, timeout); !r) {
            LOGE("Failed to receive name from the socket: %s",
                 r.error().message().c_str());
            return;
        }

        if (auto r = socket.recv_data(value, PROP_VALUE_MAX, timeout); !r) {
            LOGE("Failed to receive value from the socket: %s",
                 r.error().message().c_str());
            return;
        }

        name[PROP_NAME_MAX - 1] = '\0';
        value[PROP_VALUE_MAX - 1] = '\0';

        socket_handle_set_property_impl(socket, name, value, true);
        break;
    }

    case PROP_MSG_SETPROP2: {
        auto name = socket.recv_string(timeout);
        if (!name) {
            LOGE("Failed to receive name from the socket: %s",
                 name.error().message().c_str());
            (void) socket.send_uint32(PROP_ERROR_READ_DATA);
            return;
        }

        auto value = socket.recv_string(timeout);
        if (!value) {
            LOGE("Failed to receive value from the socket: %s",
                 value.error().message().c_str());
            (void) socket.send_uint32(PROP_ERROR_READ_DATA);
            return;
        }

        socket_handle_set_property_impl(socket, name.value(), value.value(),
                                        false);
        break;
      }

    default:
        LOGE("Invalid command: %u", cmd.value());
        (void) socket.send_uint32(PROP_ERROR_INVALID_CMD);
        break;
    }
}

void PropertyService::socket_handle_set_property_impl(SocketConnection &socket,
                                                      const std::string &name,
                                                      std::string_view value,
                                                      bool legacy)
{
    const char *cmd_name = legacy ? "PROP_MSG_SETPROP" : "PROP_MSG_SETPROP2";

    if (!is_legal_property_name(name)) {
        LOGE("[%s] Invalid property name: '%s'", cmd_name, name.c_str());
        (void) socket.send_uint32(PROP_ERROR_INVALID_NAME);
        return;
    }

    if (mb::starts_with(name, "ctl.")) {
        LOGW("[%s] Ignoring control message: '%s'='%s'",
             cmd_name, name.c_str(), std::string(value).c_str());
        if (!legacy) {
            (void) socket.send_uint32(PROP_SUCCESS);
        }
    } else {
        uint32_t result = set(name, value);
        if (!legacy) {
            (void) socket.send_uint32(result);
        }
    }
}

bool PropertyService::load_properties_file(const std::string &path,
                                           std::string_view filter)
{
    if (!mb::util::property_file_iter(path, filter, [&](std::string_view key,
                                                        std::string_view value) {
        set(std::string(key), value);
        return mb::util::PropertyIterAction::Continue;
    })) {
        LOGW("%s: Failed to load property file", path.c_str());
        return false;
    }

    return true;
}

void PropertyService::load_boot_props()
{
    if (!load_properties_file("/system/etc/prop.default", {})) {
        // Try recovery path
        if (!load_properties_file("/prop.default", {})) {
            // Try legacy path
            load_properties_file("/default.prop", {});
        }
    }
    load_properties_file("/odm/default.prop", {});
    load_properties_file("/vendor/default.prop", {});
}

void PropertyService::load_system_props()
{
    load_properties_file(BUILD_PROP_PATH, {});
    load_properties_file("/odm/build.prop", {});
    load_properties_file("/vendor/build.prop", {});
    load_properties_file("/factory/factory.prop", "ro.*");
}
