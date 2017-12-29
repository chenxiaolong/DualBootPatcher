/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbpio/posix/delete.h"

#include <cerrno>
#include <cstring>

#include <ftw.h>

#include "mbcommon/string.h"

#include "mbpio/error.h"

namespace mb::io::posix
{

static int _delete_cb(const char *fpath, const struct stat *sb,
                      int typeflag, struct FTW *ftwbuf)
{
    (void) sb;
    (void) typeflag;
    (void) ftwbuf;

    int ret = remove(fpath);
    if (ret < 0) {
        set_last_error(Error::PlatformError, mb::format(
                "%s: Failed to remove: %s", fpath, strerror(errno)));
    }
    return ret;
}

bool delete_recursively(const std::string &path)
{
    return nftw(path.c_str(), _delete_cb, 64, FTW_DEPTH | FTW_PHYS) == 0;
}

}
