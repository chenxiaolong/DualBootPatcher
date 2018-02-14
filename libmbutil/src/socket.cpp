/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

#include <sys/socket.h>
#include <unistd.h>

#include "mbcommon/error_code.h"

namespace mb::util
{

oc::result<size_t> socket_read(int fd, void *buf, size_t size)
{
    // read()'s behavior is undefined when size is greater than SSIZE_MAX
    if (size > SSIZE_MAX) {
        return std::errc::invalid_argument;
    }

    size_t bytes_read = 0;

    while (bytes_read < size) {
        auto n = read(fd, static_cast<char *>(buf) + bytes_read,
                 size - bytes_read);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                return ec_from_errno();
            }
        } else if (n == 0) {
            break;
        }

        bytes_read += static_cast<size_t>(n);
    }

    return bytes_read;
}

oc::result<size_t> socket_write(int fd, const void *buf, size_t size)
{
    // write()'s behavior is implementation-defined when size is greater than
    // SSIZE_MAX
    if (size > SSIZE_MAX) {
        return std::errc::invalid_argument;
    }

    size_t bytes_written = 0;

    while (bytes_written < size) {
        auto n = write(fd, static_cast<const char *>(buf) + bytes_written,
                  size - bytes_written);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                return ec_from_errno();
            }
        } else if (n == 0) {
            break;
        }

        bytes_written += static_cast<size_t>(n);
    }

    return bytes_written;
}

oc::result<std::vector<unsigned char>> socket_read_bytes(int fd)
{
    OUTCOME_TRY(len, socket_read_int32(fd));
    if (len < 0) {
        return std::errc::bad_message;
    }

    std::vector<unsigned char> buf(static_cast<size_t>(len));

    OUTCOME_TRY(n, socket_read(fd, buf.data(), static_cast<size_t>(len)));
    if (n != static_cast<size_t>(len)) {
        return std::errc::io_error;
    }

    return std::move(buf);
}

oc::result<void> socket_write_bytes(int fd, const void *data, size_t len)
{
    if (len > INT32_MAX) {
        return std::errc::invalid_argument;
    }

    OUTCOME_TRYV(socket_write_int32(fd, static_cast<int32_t>(len)));
    OUTCOME_TRY(n, socket_write(fd, data, len));
    if (n != len) {
        return std::errc::io_error;
    }

    return oc::success();
}

template<typename T>
static inline oc::result<T> read_copyable_type(int fd)
{
    T buf;
    OUTCOME_TRY(size, socket_read(fd, &buf, sizeof(T)));
    if (size != sizeof(T)) {
        return std::errc::io_error;
    }
    return oc::success(buf);
}

template<typename T>
static inline oc::result<void> write_copyable_type(int fd, T value)
{
    OUTCOME_TRY(size, socket_write(fd, &value, sizeof(T)));
    if (size != sizeof(T)) {
        return std::errc::io_error;
    }
    return oc::success();
}

oc::result<uint16_t> socket_read_uint16(int fd)
{
    return read_copyable_type<uint16_t>(fd);
}

oc::result<void> socket_write_uint16(int fd, uint16_t n)
{
    return write_copyable_type(fd, n);
}

oc::result<uint32_t> socket_read_uint32(int fd)
{
    return read_copyable_type<uint32_t>(fd);
}

oc::result<void> socket_write_uint32(int fd, uint32_t n)
{
    return write_copyable_type(fd, n);
}

oc::result<uint64_t> socket_read_uint64(int fd)
{
    return read_copyable_type<uint64_t>(fd);
}

oc::result<void> socket_write_uint64(int fd, uint64_t n)
{
    return write_copyable_type(fd, n);
}

oc::result<int16_t> socket_read_int16(int fd)
{
    return read_copyable_type<int16_t>(fd);
}

oc::result<void> socket_write_int16(int fd, int16_t n)
{
    return write_copyable_type(fd, n);
}

oc::result<int32_t> socket_read_int32(int fd)
{
    return read_copyable_type<int32_t>(fd);
}

oc::result<void> socket_write_int32(int fd, int32_t n)
{
    return write_copyable_type(fd, n);
}

oc::result<int64_t> socket_read_int64(int fd)
{
    return read_copyable_type<int64_t>(fd);
}

oc::result<void> socket_write_int64(int fd, int64_t n)
{
    return write_copyable_type(fd, n);
}

oc::result<std::string> socket_read_string(int fd)
{
    OUTCOME_TRY(len, socket_read_int32(fd));
    if (len < 0) {
        return std::errc::bad_message;
    }

    std::string buf;
    buf.resize(static_cast<size_t>(len));

    OUTCOME_TRY(n, socket_read(fd, buf.data(), static_cast<size_t>(len)));
    if (n != static_cast<size_t>(len)) {
        return std::errc::io_error;
    }

    return std::move(buf);
}

oc::result<void> socket_write_string(int fd, const std::string &str)
{
    if (str.size() > INT32_MAX) {
        return std::errc::invalid_argument;
    }

    OUTCOME_TRYV(socket_write_int32(fd, static_cast<int32_t>(str.size())));
    OUTCOME_TRY(n, socket_write(fd, str.data(), str.size()));
    if (n != str.size()) {
        return std::errc::io_error;
    }

    return oc::success();
}

oc::result<std::vector<std::string>> socket_read_string_array(int fd)
{
    OUTCOME_TRY(len, socket_read_int32(fd));
    if (len < 0) {
        return std::errc::bad_message;
    }

    std::vector<std::string> buf;
    buf.reserve(static_cast<size_t>(len));

    for (int32_t i = 0; i < len; ++i) {
        OUTCOME_TRY(str, socket_read_string(fd));
        buf.push_back(std::move(str));
    }

    return std::move(buf);
}

oc::result<void> socket_write_string_array(int fd, const std::vector<std::string> &list)
{
    if (list.size() > INT32_MAX) {
        return std::errc::invalid_argument;
    }

    OUTCOME_TRYV(socket_write_int32(fd, static_cast<int32_t>(list.size())));

    for (const std::string &str : list) {
        OUTCOME_TRYV(socket_write_string(fd, str));
    }

    return oc::success();
}

oc::result<void> socket_receive_fds(int fd, std::vector<int> &fds)
{
    char dummy;

    struct iovec iov;
    iov.iov_base = &dummy;
    iov.iov_len = 1;

    std::vector<char> control(
            sizeof(struct cmsghdr) + sizeof(int) * fds.size());

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
        return ec_from_errno();
    }

    int *data = reinterpret_cast<int *>(CMSG_DATA(cmsg));
    std::size_t n_fds = (cmsg->cmsg_len - sizeof(struct cmsghdr)) / sizeof(int);
    if (n_fds != fds.size()) {
        // Did not receive correct amount of file descriptors
        return std::errc::bad_message;
    }

    for (std::size_t i = 0; i < n_fds; ++i) {
        fds[i] = data[i];
    }

    return oc::success();
}

oc::result<void> socket_send_fds(int fd, const std::vector<int> &fds)
{
    if (fds.empty()) {
        return std::errc::invalid_argument;
    }

    char dummy = '!';

    struct iovec iov;
    iov.iov_base = &dummy;
    iov.iov_len = 1;

    std::vector<char> control(
            sizeof(struct cmsghdr) + sizeof(int) * fds.size());

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
    for (std::size_t i = 0; i < fds.size(); ++i) {
        data[i] = fds[i];
    }

    if (sendmsg(fd, &msg, 0) < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

}
