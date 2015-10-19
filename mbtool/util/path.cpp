/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util/path.h"

#include <vector>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util/logging.h"

namespace mb
{
namespace util
{

std::string get_cwd()
{
    char *cwd = nullptr;

    if (!(cwd = getcwd(nullptr, 0))) {
        LOGE("Failed to get cwd: %s", strerror(errno));
        return std::string();
    }

    std::string ret(cwd);
    free(cwd);
    return ret;
}

std::string dir_name(const std::string &path)
{
    std::vector<char> buf(path.begin(), path.end());
    buf.push_back('\0');

    char *ptr = dirname(buf.data());
    return std::string(ptr);
}

std::string base_name(const std::string &path)
{
    std::vector<char> buf(path.begin(), path.end());
    buf.push_back('\0');

    char *ptr = basename(buf.data());
    return std::string(ptr);
}

std::string real_path(const std::string &path)
{
    char actual_path[PATH_MAX + 1];
    if (!realpath(path.c_str(), actual_path)) {
        return std::string();
    }
    return std::string(actual_path);
}

bool read_link(const std::string &path, std::string *out)
{
    std::vector<char> buf;
    ssize_t len;

    buf.resize(64);

    for (;;) {
        len = readlink(path.c_str(), buf.data(), buf.size() - 1);
        if (len < 0) {
            return false;
        } else if ((size_t) len == buf.size() - 1) {
            buf.resize(buf.size() << 1);
        } else {
            break;
        }
    }

    buf[len] = '\0';
    out->assign(buf.data());
    return true;
}

bool inodes_equal(const std::string &path1, const std::string &path2)
{
    struct stat sb1;
    struct stat sb2;

    if (lstat(path1.c_str(), &sb1) < 0) {
        LOGE("%s: Failed to stat: %s", path1.c_str(), strerror(errno));
        return false;
    }

    if (lstat(path2.c_str(), &sb2) < 0) {
        LOGE("%s: Failed to stat: %s", path2.c_str(), strerror(errno));
        return false;
    }

    return sb1.st_dev == sb2.st_dev && sb1.st_ino == sb2.st_ino;
}

/*!
 * \brief Split a path into pieces
 *
 * This function splits the path into pieces with the directory separator '/'
 * being the delimiter. If the path is absolute (ie. begins with '/'), then the
 * first piece will be empty. If a piece is equal to the current directory '.',
 * then it is simply removed. If multiple directory separator characters are
 * located next to one another (ie. 'a/////b'), they are treated as one
 * directory separator (ie. becomes 'a/b').
 *
 * \param path Path to split
 *
 * \return Split pieces of the path
 */
std::vector<std::string> path_split(const std::string &path)
{
    char *p;
    char *save_ptr;
    std::vector<char> copy(path.begin(), path.end());
    copy.push_back('\0');

    std::vector<std::string> split;

    // For absolute paths
    if (!path.empty() && path[0] == '/') {
        split.push_back(std::string());
    }

    p = strtok_r(copy.data(), "/", &save_ptr);
    while (p != nullptr) {
        // Ignore useless references to '.' in the path
        if (strcmp(p, ".") != 0) {
            split.push_back(p);
        }
        p = strtok_r(nullptr, "/", &save_ptr);
    }

    return split;
}

/*!
 * \brief Join path pieces into a path
 *
 * This function simply joins each piece with the directory separator character.
 * If the first piece is empty, then the path is treated as an absolute path and
 * a directory separator character will be placed at the beginning of the
 * resulting path. If the pieces contain only a single empty piece, then the
 * root directory '/' is returned.
 *
 * \param components Path pieces to join
 *
 * \return Joined path
 */
std::string path_join(const std::vector<std::string> &components)
{
    std::string path;
    for (auto it = components.begin(); it != components.end(); ++it) {
        if (it->empty()) {
            path += "/";
        } else {
            path += *it;
            if (it != components.end() - 1) {
                path += "/";
            }
        }
    }
    return path;
}

}
}
