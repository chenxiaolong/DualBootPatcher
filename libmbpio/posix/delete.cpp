/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "libmbpio/posix/delete.h"

#include <cerrno>
#include <cstring>

#include <ftw.h>

#include "libmbpio/error.h"
#include "libmbpio/private/string.h"

namespace io
{
namespace posix
{

static int deleteCbNftw(const char *fpath, const struct stat *sb,
                        int typeflag, struct FTW *ftwbuf)
{
    (void) sb;
    (void) typeflag;
    (void) ftwbuf;

    int ret = remove(fpath);
    if (ret < 0) {
        setLastError(Error::PlatformError, priv::format(
                "%s: Failed to remove: %s", fpath, strerror(errno)));
    }
    return ret;
}

bool deleteRecursively(const std::string &path)
{
    return nftw(path.c_str(), deleteCbNftw, 64, FTW_DEPTH | FTW_PHYS);
}

}
}
