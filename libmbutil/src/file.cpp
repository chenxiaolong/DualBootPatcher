/*
 * Copyright (C) 2014-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/file.h"

#include <memory>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbcommon/file_error.h"
#include "mbcommon/finally.h"

namespace mb::util
{

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

/*!
 * \brief Create empty file with 0666 permissions
 *
 * \note Returns 0 if file already exists
 *
 * \param path File to create
 *
 * \return Nothing on success or the error code on failure
 */
oc::result<void> create_empty_file(const std::string &path)
{
    if (int fd = open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0666);
            fd >= 0) {
        close(fd);
        return oc::success();
    } else {
        return ec_from_errno();
    }
}

/*!
 * \brief Get first line from file
 *
 * The trailing newline (if exists) will be stripped from the result.
 *
 * \param path File to read
 *
 * \return Nothing on success or the error code on failure. Returns
 *         FileError::UnexpectedEof if the file is empty.
 */
oc::result<std::string> file_first_line(const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "rbe"), fclose);
    if (!fp) {
        return ec_from_errno();
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t read;

    auto free_line = finally([&] {
        free(line);
    });

    if ((read = getline(&line, &len, fp.get())) < 0) {
        if (ferror(fp.get())) {
            return ec_from_errno();
        } else {
            return FileError::UnexpectedEof;
        }
    }

    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
        --read;
    }

    return line;
}

/*!
 * \brief Write data to a file
 *
 * \param path File to write
 * \param data Pointer to data to write
 * \param size Size of \a data
 *
 * \return Nothing on success or the error code on failure. Returns
 *         FileError::UnexpectedEof if EOF is reached before the write
 *         completes.
 */
oc::result<void> file_write_data(const std::string &path,
                                 const void *data, size_t size)
{
    ScopedFILE fp(fopen(path.c_str(), "wbe"), fclose);
    if (!fp) {
        return ec_from_errno();
    }

    size_t n = std::fwrite(data, 1, size, fp.get());
    if (n != size) {
        if (ferror(fp.get())) {
            return ec_from_errno();
        } else {
            return FileError::UnexpectedEof;
        }
    }

    if (fclose(fp.release()) != 0) {
        return ec_from_errno();
    }

    return oc::success();
}

/*!
 * \brief Write string to a file
 *
 * \param path File to write
 * \param contents Data to write
 *
 * \return Nothing on success or the error code on failure. Returns
 *         FileError::UnexpectedEof if EOF is reached before the write
 *         completes.
 */
oc::result<void> file_write_string(const std::string &path,
                                   std::string_view contents)
{
    return file_write_data(path, contents.data(), contents.size());
}

oc::result<bool> file_find_one_of(const std::string &path,
                                  const std::vector<std::string> &items)
{
    struct stat sb;
    void *map = MAP_FAILED;
    int fd = -1;

    if ((fd = open(path.c_str(), O_RDONLY | O_CLOEXEC)) < 0) {
        return ec_from_errno();
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    if (fstat(fd, &sb) < 0) {
        return ec_from_errno();
    }

    map = mmap(nullptr, static_cast<size_t>(sb.st_size), PROT_READ, MAP_PRIVATE,
               fd, 0);
    if (map == MAP_FAILED) {
        return ec_from_errno();
    }

    auto unmap_map = finally([&] {
        munmap(map, static_cast<size_t>(sb.st_size));
    });

    for (auto const &item : items) {
        if (memmem(map, static_cast<size_t>(sb.st_size),
                   item.data(), item.size())) {
            return true;
        }
    }

    return false;
}

oc::result<std::string> file_read_all(const std::string &path)
{
    ScopedFILE fp(fopen(path.c_str(), "rbe"), fclose);
    if (!fp) {
        return ec_from_errno();
    }

    std::string data;

    // Reduce allocations if possible
    if (fseeko(fp.get(), 0, SEEK_END) == 0) {
        auto size = ftello(fp.get());
        if (size < 0) {
            return ec_from_errno();
        } else if (std::make_unsigned_t<decltype(size)>(size) > SIZE_MAX) {
            return std::errc::result_out_of_range;
        }

        data.reserve(static_cast<size_t>(size));
    }
    if (fseeko(fp.get(), 0, SEEK_SET) < 0) {
        return ec_from_errno();
    }

    char buf[8192];

    while (true) {
        auto n = fread(buf, 1, sizeof(buf), fp.get());

        data.insert(data.end(), buf, buf + n);

        if (n < sizeof(buf)) {
            if (ferror(fp.get())) {
                return ec_from_errno();
            } else {
                break;
            }
        }
    }

    return std::move(data);
}

oc::result<uint64_t> get_blockdev_size(const std::string &path)
{
    int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return ec_from_errno();
    }

    auto close_fd = finally([&]{
        close(fd);
    });

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

    uint64_t size;
    if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
        return ec_from_errno();
    }

#pragma GCC diagnostic pop

    return size;
}

}
