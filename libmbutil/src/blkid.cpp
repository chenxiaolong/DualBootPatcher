/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/blkid.h"

#include <vector>

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include "mbutil/finally.h"

// NOTE: We don't use libblkid from util-linux because we don't need most of its
// features and it increases mbtool's binary size more than 200KiB (armeabi-v7a)

namespace mb
{
namespace util
{

static inline bool check_magic(const void *data, size_t data_size,
                               const void *magic, size_t magic_size,
                               size_t offset)
{
    if (offset + magic_size > data_size) {
        return false;
    }

    return memcmp(static_cast<const unsigned char *>(data) + offset,
                  magic, magic_size) == 0;
}

static inline bool is_btrfs(const void *data, size_t size)
{
    return check_magic(data, size, "_BHRfS_M", 8, 64 * 1024 + 0x40);
}

static inline bool is_exfat(const void *data, size_t size)
{
    return check_magic(data, size, "EXFAT   ", 8, 3);
}

static inline bool is_ext(const void *data, size_t size)
{
    // This matches ext2, ext3, ext4, ext4dev, and jbd
    return check_magic(data, size, "\x53\xef", 2, 0x400 + 0x38);
}

static inline bool is_f2fs(const void *data, size_t size)
{
    return check_magic(data, size, "\x10\x20\xF5\xF2", 4, 0x400);
}

static inline bool is_ntfs(const void *data, size_t size)
{
    return check_magic(data, size, "NTFS    ", 8, 3);
}

static inline bool is_squashfs(const void *data, size_t size)
{
    return     check_magic(data, size, "hsqs", 4, 0)
            || check_magic(data, size, "sqsh", 4, 0);
}

static inline bool is_vfat(const void *data, size_t size)
{
    return     check_magic(data, size, "MSWIN",    5, 0x52)
            || check_magic(data, size, "FAT32   ", 8, 0x52)
            || check_magic(data, size, "MSDOS",    5, 0x36)
            || check_magic(data, size, "FAT16   ", 8, 0x36)
            || check_magic(data, size, "FAT12   ", 8, 0x36)
            || check_magic(data, size, "FAT     ", 8, 0x36)
            || check_magic(data, size, "\353",     1, 0)
            || check_magic(data, size, "\351",     1, 0)
            || check_magic(data, size, "\125\252", 2, 0x1fe);
}

struct probe_func
{
    const char *name;
    bool (*func)(const void *, size_t);
};

static probe_func probe_funcs[] = {
    { "btrfs",    &is_btrfs },
    { "exfat",    &is_exfat },
    { "ext",      &is_ext },
    { "f2fs",     &is_f2fs },
    { "ntfs",     &is_ntfs },
    { "squashfs", &is_squashfs },
    { "vfat",     &is_vfat },
    { nullptr,    nullptr },
};

static ssize_t read_all(int fd, void *buf, size_t size)
{
    size_t total = 0;

    while (total < size) {
        ssize_t n = read(fd, static_cast<char *>(buf) + total, size - total);
        if (n == 0) {
            break;
        } else if (n < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                return n;
            }
        } else {
            total += n;
        }
    }

    return total;
}

bool blkid_get_fs_type(const char *path, const char **type)
{
    std::vector<unsigned char> buf(1024 * 1024);

    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }

    auto close_fd = finally([&]{
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
    });

    ssize_t n = read_all(fd, buf.data(), buf.size());
    if (n < 0) {
        return false;
    }

    for (auto it = probe_funcs; it->name; ++it) {
        if (it->func(buf.data(), n)) {
            *type = it->name;
            return true;
        }
    }

    *type = nullptr;
    return true;
}

}
}
