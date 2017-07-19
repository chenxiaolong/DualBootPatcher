/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/autoclose/file.h"
#include "mbutil/finally.h"

namespace mb
{
namespace util
{

/*!
 * \brief Create empty file with 0666 permissions
 *
 * \note Returns 0 if file already exists
 *
 * \param path File to create
 * \return true on success, false on failure with errno set appropriately
 */
bool create_empty_file(const std::string &path)
{
    int fd;
    if ((fd = open(path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR |
                                                   S_IRGRP | S_IWGRP |
                                                   S_IROTH | S_IWOTH)) < 0) {
        return false;
    }

    close(fd);
    return true;
}

/*!
 * \brief Get first line from file
 *
 * \param path File to read
 * \param line_out Pointer to char * which will contain the first line
 *
 * \note line_out will point to a dynamically allocated buffer that should be
 *       free()'d when it is no longer needed
 *
 * \note line_out is not modified if this function fails
 *
 * \return true on success, false on failure and errno set appropriately
 */
bool file_first_line(const std::string &path,
                     std::string *line_out)
{
    autoclose::file fp(autoclose::fopen(path.c_str(), "rb"));
    if (!fp) {
        return false;
    }

    char *line = nullptr;
    size_t len = 0;
    ssize_t read;

    auto free_line = finally([&] {
        free(line);
    });

    if ((read = getline(&line, &len, fp.get())) < 0) {
        return false;
    }

    if (read > 0 && line[read - 1] == '\n') {
        line[read - 1] = '\0';
        --read;
    }

    *line_out = line;

    return true;
}

/*!
 * \brief Write data to a file
 *
 * \param path File to write
 * \param data Pointer to data to write
 * \param size Size of \a data
 *
 * \return true on success, false on failure and errno set appropriately
 */
bool file_write_data(const std::string &path,
                     const char *data, size_t size)
{
    FILE *fp = fopen(path.c_str(), "wb");
    if (!fp) {
        return false;
    }

    size_t nwritten;

    do {
        if ((nwritten = std::fwrite(data, 1, size, fp)) == 0) {
            break;
        }

        size -= nwritten;
        data += nwritten;
    } while (size > 0);

    bool ret = size == 0 || !ferror(fp);

    if (fclose(fp) < 0) {
        return false;
    }

    return ret;
}

bool file_find_one_of(const std::string &path, std::vector<std::string> items)
{
    struct stat sb;
    void *map = MAP_FAILED;
    int fd = -1;

    if ((fd = open(path.c_str(), O_RDONLY)) < 0) {
        return false;
    }

    auto close_fd = util::finally([&] {
        close(fd);
    });

    if (fstat(fd, &sb) < 0) {
        return false;
    }

    map = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        return false;
    }

    auto unmap_map = util::finally([&] {
        munmap(map, sb.st_size);
    });

    for (auto const &item : items) {
        if (memmem(map, sb.st_size, item.data(), item.size())) {
            return true;
        }
    }

    return false;
}

bool file_read_all(const std::string &path,
                   std::vector<unsigned char> *data_out)
{
    autoclose::file fp(autoclose::fopen(path.c_str(), "rb"));
    if (!fp) {
        return false;
    }

    fseek(fp.get(), 0, SEEK_END);
    auto size = ftell(fp.get());
    rewind(fp.get());

    std::vector<unsigned char> data(size);
    if (fread(data.data(), size, 1, fp.get()) != 1) {
        return false;
    }

    data_out->swap(data);

    return true;
}

bool file_read_all(const std::string &path,
                   unsigned char **data_out,
                   std::size_t *size_out)
{
    autoclose::file fp(autoclose::fopen(path.c_str(), "rb"));
    if (!fp) {
        return false;
    }

    fseek(fp.get(), 0, SEEK_END);
    auto size = ftell(fp.get());
    rewind(fp.get());

    unsigned char *data = static_cast<unsigned char *>(std::malloc(size));
    if (!data) {
        return false;
    }

    if (fread(data, size, 1, fp.get()) != 1) {
        free(data);
        return false;
    }

    *data_out = data;
    *size_out = size;

    return true;
}

bool get_blockdev_size(const char *path, uint64_t *size_out)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    auto close_fd = finally([&]{
        close(fd);
    });

    uint64_t size;
    if (ioctl(fd, BLKGETSIZE64, &size) < 0) {
        return false;
    }

    *size_out = size;

    return true;
}

}
}
