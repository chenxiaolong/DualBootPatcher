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

#include <ftw.h>

#include "mbcommon/error/error.h"
#include "mbcommon/error/type/ec_error.h"

namespace mb
{
namespace io
{
namespace posix
{

static int _delete_cb(const char *fpath, const struct stat *sb,
                      int typeflag, struct FTW *ftwbuf)
{
    (void) sb;
    (void) typeflag;
    (void) ftwbuf;

    return remove(fpath);
}

Expected<void> delete_recursively(const std::string &path)
{
    if (nftw(path.c_str(), _delete_cb, 64, FTW_DEPTH | FTW_PHYS) < 0) {
        return make_error<ECError>(ECError::from_errno(errno));
    }
    return {};
}

}
}
}
