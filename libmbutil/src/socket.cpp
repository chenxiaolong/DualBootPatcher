/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/socket.h"

#include <climits>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace mb
{
namespace util
{

ssize_t socket_read(int fd, void *buf, size_t size)
{
    // read()'s behavior is undefined when size is greater than SSIZE_MAX
    if (size > SSIZE_MAX) {
        errno = EINVAL;
        return -1;
    }

    ssize_t bytes_read = 0;
    ssize_t n;

    while (bytes_read < static_cast<ssize_t>(size)) {
        n = read(fd, static_cast<char *>(buf) + bytes_read,
                 size - static_cast<size_t>(bytes_read));
        if (n < 0) {
            return n;
        } else if (n == 0) {
            break;
        }

        bytes_read += n;
    }

    return bytes_read;
}

ssize_t socket_write(int fd, const void *buf, size_t size)
{
    // write()'s behavior is implementation-defined when size is greater than
    // SSIZE_MAX
    if (size > SSIZE_MAX) {
        errno = EINVAL;
        return -1;
    }

    ssize_t bytes_written = 0;
    ssize_t n;

    while (bytes_written < static_cast<ssize_t>(size)) {
        n = write(fd, static_cast<const char *>(buf) + bytes_written,
                  size - static_cast<size_t>(bytes_written));
        if (n < 0) {
            return n;
        } else if (n == 0) {
            break;
        }

        bytes_written += n;
    }

    return bytes_written;
}

bool socket_read_bytes(int fd, std::vector<uint8_t> &result)
{
    int32_t len;
    if (!socket_read_int32(fd, len)) {
        return false;
    }

    if (len < 0) {
        return false;
    }

    std::vector<uint8_t> buf(static_cast<size_t>(len));

    if (socket_read(fd, buf.data(), static_cast<size_t>(len))
            != static_cast<ssize_t>(len)) {
        return false;
    }

    result.swap(buf);
    return true;
}

bool socket_write_bytes(int fd, const uint8_t *data, size_t len)
{
    if (!socket_write_int32(fd, static_cast<int32_t>(len))) {
        return false;
    }

    if (socket_write(fd, data, len) != static_cast<ssize_t>(len)) {
        return false;
    }

    return true;
}

template<typename T>
static inline bool read_copyable_type(int fd, T &result)
{
    T buf;
    if (socket_read(fd, &buf, sizeof(T)) != sizeof(T)) {
        return false;
    }
    result = buf;
    return true;
}

template<typename T>
static inline bool write_copyable_type(int fd, T value)
{
    return socket_write(fd, &value, sizeof(T)) == sizeof(T);
}

bool socket_read_uint16(int fd, uint16_t &result)
{
    return read_copyable_type(fd, result);
}

bool socket_write_uint16(int fd, uint16_t n)
{
    return write_copyable_type(fd, n);
}

bool socket_read_uint32(int fd, uint32_t &result)
{
    return read_copyable_type(fd, result);
}

bool socket_write_uint32(int fd, uint32_t n)
{
    return write_copyable_type(fd, n);
}

bool socket_read_uint64(int fd, uint64_t &result)
{
    return read_copyable_type(fd, result);
}

bool socket_write_uint64(int fd, uint64_t n)
{
    return write_copyable_type(fd, n);
}

bool socket_read_int16(int fd, int16_t &result)
{
    return read_copyable_type(fd, result);
}

bool socket_write_int16(int fd, int16_t n)
{
    return write_copyable_type(fd, n);
}

bool socket_read_int32(int fd, int32_t &result)
{
    return read_copyable_type(fd, result);
}

bool socket_write_int32(int fd, int32_t n)
{
    return write_copyable_type(fd, n);
}

bool socket_read_int64(int fd, int64_t &result)
{
    return read_copyable_type(fd, result);
}

bool socket_write_int64(int fd, int64_t n)
{
    return write_copyable_type(fd, n);
}

bool socket_read_string(int fd, std::string &result)
{
    int32_t len;
    if (!socket_read_int32(fd, len)) {
        return false;
    }

    if (len < 0) {
        return false;
    }

    std::vector<char> buf(static_cast<size_t>(len));

    if (socket_read(fd, buf.data(), static_cast<size_t>(len))
            != static_cast<ssize_t>(len)) {
        return false;
    }

    result.assign(buf.begin(), buf.end());
    return true;
}

bool socket_write_string(int fd, const std::string &str)
{
    if (!socket_write_int32(fd, static_cast<int32_t>(str.size()))) {
        return false;
    }

    if (socket_write(fd, str.data(), str.size())
            != static_cast<ssize_t>(str.size())) {
        return false;
    }

    return true;
}

bool socket_read_string_array(int fd, std::vector<std::string> &result)
{
    int32_t len;
    if (!socket_read_int32(fd, len)) {
        return false;
    }

    if (len < 0) {
        return false;
    }

    std::vector<std::string> buf(static_cast<size_t>(len));
    for (int32_t i = 0; i < len; ++i) {
        if (!socket_read_string(fd, buf[static_cast<size_t>(i)])) {
            return false;
        }
    }

    result.swap(buf);
    return true;
}

bool socket_write_string_array(int fd, const std::vector<std::string> &list)
{
    if (!socket_write_int32(fd, static_cast<int32_t>(list.size()))) {
        return false;
    }

    for (const std::string &str : list) {
        if (!socket_write_string(fd, str)) {
            return false;
        }
    }

    return true;
}

bool socket_receive_fds(int fd, std::vector<int> &fds)
{
    char dummy;

    struct iovec iov;
    iov.iov_base = &dummy;
    iov.iov_len = 1;

    std::size_t n_fds = fds.size();
    std::vector<char> control(sizeof(struct cmsghdr) + sizeof(int) * n_fds);

    struct msghdr msg;
    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_flags = 0;
    msg.msg_control = control.data();
    msg.msg_controllen = control.size();

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_len = msg.msg_controllen;
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;

    if (recvmsg(fd, &msg, 0) < 0) {
        return false;
    }

    int *data = reinterpret_cast<int *>(CMSG_DATA(cmsg));
    n_fds = (cmsg->cmsg_len - sizeof(struct cmsghdr)) / sizeof(int);
    if (n_fds != fds.size()) {
        // Did not receive correct amount of file descriptors
        return false;
    }

    for (std::size_t i = 0; i < n_fds; ++i) {
        fds[i] = data[i];
    }

    return true;
}

bool socket_send_fds(int fd, const std::vector<int> &fds)
{
    std::size_t n_fds = fds.size();
    if (n_fds > 0) {
        char dummy = '!';

        struct iovec iov;
        iov.iov_base = &dummy;
        iov.iov_len = 1;

        std::vector<char> control(sizeof(struct cmsghdr) + sizeof(int) * n_fds);

        struct msghdr msg;
        msg.msg_name = nullptr;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_flags = 0;
        msg.msg_control = control.data();
        msg.msg_controllen = control.size();

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = msg.msg_controllen;
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        int *data = reinterpret_cast<int *>(CMSG_DATA(cmsg));
        for (std::size_t i = 0; i < n_fds; ++i) {
            data[i] = fds[i];
        }

        return sendmsg(fd, &msg, 0) >= 0;
    }

    return false;
}

}
}
