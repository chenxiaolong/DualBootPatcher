/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "boot/init/cutils/uevent.h"

#include <cerrno>
#include <cstring>

#include <strings.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <linux/netlink.h>

#include "mbcommon/finally.h"

/**
 * Like recv(), but checks that messages actually originate from the kernel.
 */
ssize_t uevent_kernel_multicast_recv(int socket, void *buffer, size_t length)
{
    uid_t uid;
    return uevent_kernel_multicast_uid_recv(socket, buffer, length, &uid);
}

/**
 * Like the above, but passes a uid_t in by pointer. In the event that this
 * fails due to a bad uid check, the uid_t will be set to the uid of the
 * socket's peer.
 *
 * If this method rejects a netlink message from outside the kernel, it
 * returns -1, sets errno to EIO, and sets "user" to the UID associated with the
 * message. If the peer UID cannot be determined, "user" is set to -1."
 */
ssize_t uevent_kernel_multicast_uid_recv(int socket, void *buffer, size_t length, uid_t *uid)
{
    return uevent_kernel_recv(socket, buffer, length, true, uid);
}

ssize_t uevent_kernel_recv(int socket, void *buffer, size_t length, bool require_group, uid_t *uid)
{
    iovec iov = { buffer, length };
    sockaddr_nl addr;
    char control[CMSG_SPACE(sizeof(ucred))];
    msghdr hdr = {
        &addr,
        sizeof(addr),
        &iov,
        1,
        control,
        sizeof(control),
        0,
    };

    *uid = static_cast<uid_t>(-1);
    ssize_t n = recvmsg(socket, &hdr, 0);
    if (n <= 0) {
        return n;
    }

    auto clear_buf_and_set_error = mb::finally([&] {
        // Clear residual potentially malicious data
        bzero(buffer, length);
        errno = EIO;
    });

    cmsghdr *cmsg = CMSG_FIRSTHDR(&hdr);
    if (!cmsg || cmsg->cmsg_type != SCM_CREDENTIALS) {
        // Ignoring netlink message with no sender credentials
        return -1;
    }

    auto *cred = reinterpret_cast<ucred *>(CMSG_DATA(cmsg));
    *uid = cred->uid;
    if (cred->uid != 0) {
        // Ignoring netlink message from non-root user
        return -1;
    }

    if (addr.nl_pid != 0) {
        // Ignore non-kernel
        return -1;
    }
    if (require_group && addr.nl_groups == 0) {
        // Ignore unicast messages when requested
        return -1;
    }

    clear_buf_and_set_error.dismiss();

    return n;
}

int uevent_open_socket(int buf_sz, bool passcred)
{
    sockaddr_nl addr;
    int on = passcred;
    int s;

    memset(&addr, 0, sizeof(addr));
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = static_cast<decltype(addr.nl_pid)>(getpid());
    addr.nl_groups = 0xffffffff;

    s = socket(PF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT);
    if (s < 0) {
        return -1;
    }

    auto close_fd = mb::finally([&] {
        close(s);
    });

    // buf_sz should be less than net.core.rmem_max for this to succeed
    if (setsockopt(s, SOL_SOCKET, SO_RCVBUF, &buf_sz, sizeof(buf_sz)) < 0) {
        return -1;
    }

    setsockopt(s, SOL_SOCKET, SO_PASSCRED, &on, sizeof(on));

    if (bind(s, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        return -1;
    }

    close_fd.dismiss();

    return s;
}
