/*
 * Copyright (C) 2017 The Android Open Source Project
 * Copyright (C) 2015-2018 Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "boot/init/uevent_listener.h"

#include <memory>

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include "mblog/logging.h"

#include "boot/init/cutils/uevent.h"

#define LOG_TAG "mbtool/boot/init/uevent_listener"

#define LOG_UEVENTS 0

namespace android {
namespace init {

static void ParseEvent(const char* msg, Uevent* uevent) {
    uevent->partition_num = -1;
    uevent->major = -1;
    uevent->minor = -1;
    uevent->action.clear();
    uevent->path.clear();
    uevent->subsystem.clear();
    uevent->firmware.clear();
    uevent->partition_name.clear();
    uevent->device_name.clear();
    // currently ignoring SEQNUM
    while (*msg) {
        if (!strncmp(msg, "ACTION=", 7)) {
            msg += 7;
            uevent->action = msg;
        } else if (!strncmp(msg, "DEVPATH=", 8)) {
            msg += 8;
            uevent->path = msg;
        } else if (!strncmp(msg, "SUBSYSTEM=", 10)) {
            msg += 10;
            uevent->subsystem = msg;
        } else if (!strncmp(msg, "FIRMWARE=", 9)) {
            msg += 9;
            uevent->firmware = msg;
        } else if (!strncmp(msg, "MAJOR=", 6)) {
            msg += 6;
            uevent->major = atoi(msg);
        } else if (!strncmp(msg, "MINOR=", 6)) {
            msg += 6;
            uevent->minor = atoi(msg);
        } else if (!strncmp(msg, "PARTN=", 6)) {
            msg += 6;
            uevent->partition_num = atoi(msg);
        } else if (!strncmp(msg, "PARTNAME=", 9)) {
            msg += 9;
            uevent->partition_name = msg;
        } else if (!strncmp(msg, "DEVNAME=", 8)) {
            msg += 8;
            uevent->device_name = msg;
        }

        // advance to after the next \0
        while (*msg++)
            ;
    }

    if (LOG_UEVENTS) {
        LOGI("event { '%s', '%s', '%s', '%s', %d, %d }",
             uevent->action.c_str(), uevent->path.c_str(),
             uevent->subsystem.c_str(), uevent->firmware.c_str(),
             uevent->major, uevent->minor);
    }
}

UeventListener::UeventListener() {
    // is 256K enough? udev uses 16MB!
    device_fd_.reset(uevent_open_socket(256 * 1024, true));
    if (device_fd_ == -1) {
        LOGE("Could not open uevent socket");
        abort();
    }

    fcntl(device_fd_, F_SETFL, O_NONBLOCK);
}

bool UeventListener::ReadUevent(Uevent* uevent) const {
    char msg[UEVENT_MSG_LEN + 2];
    auto n = uevent_kernel_multicast_recv(device_fd_, msg, UEVENT_MSG_LEN);
    if (n <= 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOGE("Error reading from Uevent Fd");
        }
        return false;
    }
    if (n >= UEVENT_MSG_LEN) {
        LOGE("Uevent overflowed buffer, discarding");
        // Return true here even if we discard as we may have more uevents pending and we
        // want to keep processing them.
        return true;
    }

    msg[n] = '\0';
    msg[n + 1] = '\0';

    ParseEvent(msg, uevent);

    return true;
}

// RegenerateUevents*() walks parts of the /sys tree and pokes the uevent files to cause the kernel
// to regenerate device add uevents that have already happened.  This is particularly useful when
// starting ueventd, to regenerate all of the uevents that it had previously missed.
//
// We drain any pending events from the netlink socket every time we poke another uevent file to
// make sure we don't overrun the socket's buffer.
//

ListenerAction UeventListener::RegenerateUeventsForDir(DIR* d,
                                                       const ListenerCallback& callback) const {
    int dfd = dirfd(d);

    int fd = openat(dfd, "uevent", O_WRONLY);
    if (fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);

        Uevent uevent;
        while (ReadUevent(&uevent)) {
            if (callback(uevent) == ListenerAction::kStop) return ListenerAction::kStop;
        }
    }

    dirent* de;
    while ((de = readdir(d)) != nullptr) {
        if (de->d_type != DT_DIR || de->d_name[0] == '.') continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if (fd < 0) continue;

        std::unique_ptr<DIR, decltype(&closedir)> d2(fdopendir(fd), closedir);
        if (d2 == 0) {
            close(fd);
        } else {
            if (RegenerateUeventsForDir(d2.get(), callback) == ListenerAction::kStop) {
                return ListenerAction::kStop;
            }
        }
    }

    // default is always to continue looking for uevents
    return ListenerAction::kContinue;
}

ListenerAction UeventListener::RegenerateUeventsForPath(const std::string& path,
                                                        const ListenerCallback& callback) const {
    std::unique_ptr<DIR, decltype(&closedir)> d(opendir(path.c_str()), closedir);
    if (!d) return ListenerAction::kContinue;

    return RegenerateUeventsForDir(d.get(), callback);
}

static const char* kRegenerationPaths[] = {"/sys/class", "/sys/block", "/sys/devices"};

void UeventListener::RegenerateUevents(const ListenerCallback& callback) const {
    for (const auto path : kRegenerationPaths) {
        if (RegenerateUeventsForPath(path, callback) == ListenerAction::kStop) return;
    }
}

void UeventListener::Poll(const ListenerCallback& callback, int cancel_fd,
                          const std::optional<std::chrono::milliseconds> relative_timeout) const {
    using namespace std::chrono;

    nfds_t ufds_size = 1;
    pollfd ufds[2];
    ufds[0].events = POLLIN;
    ufds[0].fd = device_fd_;

    if (cancel_fd >= 0) {
        ++ufds_size;
        ufds[1].events = POLLIN;
        ufds[1].fd = cancel_fd;
    }

    auto start_time = steady_clock::now();

    while (true) {
        for (auto &ufd : ufds) {
            ufd.revents = 0;
        }

        int timeout_ms = -1;
        if (relative_timeout) {
            auto now = steady_clock::now();
            auto time_elapsed = duration_cast<milliseconds>(now - start_time);
            if (time_elapsed > *relative_timeout) return;

            auto remaining_timeout = *relative_timeout - time_elapsed;
            timeout_ms = static_cast<int>(remaining_timeout.count());
        }

        int nr = poll(ufds, ufds_size, timeout_ms);
        if (nr == 0) return;
        if (nr < 0) {
            LOGE("poll() of uevent socket failed, continuing: %s",
                 strerror(errno));
            continue;
        }
        // Check for cancellation signal first
        if (ufds[1].revents & POLLIN) {
            break;
        }
        if (ufds[0].revents & POLLIN) {
            // We're non-blocking, so if we receive a poll event keep processing until
            // we have exhausted all uevent messages.
            Uevent uevent;
            while (ReadUevent(&uevent)) {
                if (callback(uevent) == ListenerAction::kStop) return;
            }
        }
    }
}

}  // namespace init
}  // namespace android
