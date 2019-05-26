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

#include "mbutil/delete.h"

#include <cerrno>
#include <cstdlib>
#include <sys/stat.h>

#include "mbcommon/error_code.h"
#include "mbutil/fts.h"


namespace mb::util
{

class RecursiveDeleter : public FtsWrapper {
public:
    std::string error_path;
    std::error_code error;

    RecursiveDeleter(std::string path)
        : FtsWrapper(std::move(path), FtsFlag::GroupSpecialFiles)
    {
    }

    Actions on_reached_directory_pre() override
    {
        // Do nothing. Need depth-first search, so directories are deleted in
        // on_reached_directory_post()
        return Action::Ok;
    }

    Actions on_reached_directory_post() override
    {
        return delete_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_file() override
    {
        return delete_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_symlink() override
    {
        return delete_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_special_file() override
    {
        return delete_path() ? Action::Ok : Action::Fail;
    }

private:
    bool delete_path()
    {
        if (remove(_curr->fts_accpath) < 0) {
            error_path = _curr->fts_path;
            error = ec_from_errno();
            return false;
        }
        return true;
    }
};

FileOpResult<void> delete_recursive(const std::string &path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) < 0 && errno == ENOENT) {
        // Don't fail if directory does not exist
        return oc::success();
    }

    RecursiveDeleter deleter(path);
    if (!deleter.run()) {
        return FileOpErrorInfo{std::move(deleter.error_path), deleter.error};
    }

    return oc::success();
}

}
