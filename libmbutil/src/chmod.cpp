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

#include "mbutil/chmod.h"

#include <cstdlib>

#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbcommon/string.h"
#include "mbutil/fts.h"


namespace mb::util
{

class RecursiveChmod : public FtsWrapper {
public:
    std::error_code ec;

    RecursiveChmod(std::string path, mode_t perms)
        : FtsWrapper(std::move(path), FtsFlag::GroupSpecialFiles)
        , _perms(perms)
    {
    }

    Actions on_reached_directory_pre() override
    {
        // Do nothing. Need depth-first search.
        return Action::Ok;
    }

    Actions on_reached_directory_post() override
    {
        return chmod_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_file() override
    {
        return chmod_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_symlink() override
    {
        return Action::Skip;
    }

    Actions on_reached_special_file() override
    {
        return chmod_path() ? Action::Ok : Action::Fail;
    }

private:
    mode_t _perms;

    bool chmod_path()
    {
        if (::chmod(_curr->fts_accpath, _perms) < 0) {
            ec = ec_from_errno();
            return false;
        }
        return true;
    }
};


oc::result<void> chmod(const std::string &path, mode_t perms, ChmodFlags flags)
{
    if (flags & ChmodFlag::Recursive) {
        RecursiveChmod fts(path, perms);
        if (!fts.run()) {
            return fts.ec;
        }
    } else {
        if (::chmod(path.c_str(), perms) < 0) {
            return ec_from_errno();
        }
    }

    return oc::success();
}

}
