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

#include "mbutil/delete.h"

#include <boost/filesystem/operations.hpp>


namespace mb::util
{

namespace fs = boost::filesystem;

oc::result<void> delete_recursive(const fs::path &path)
{
    std::error_code ec;
    fs::remove_all(path, ec);

    if (ec /* && ec != std::errc::no_such_file_or_directory */) {
        return ec;
    }

    return oc::success();
}

}
