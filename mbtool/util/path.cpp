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

#include <cerrno>
#include <libgen.h>

#include "util/logging.h"

namespace mb
{
namespace util
{

std::string get_cwd()
{
    char *cwd = NULL;

    if (!(cwd = getcwd(NULL, 0))) {
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

}
}