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

#include "util/delete.h"

#include <cerrno>
#include <cstring>
#include <sys/stat.h>

#include <cppformat/format.h>

#include "util/fts.h"
#include "util/logging.h"

namespace mb
{
namespace util
{

class RecursiveDeleter : public FTSWrapper {
public:
    RecursiveDeleter(std::string path)
        : FTSWrapper(path, FTS_GroupSpecialFiles)
    {
    }

    virtual int on_reached_directory_pre() override
    {
        // Do nothing. Need depth-first search, so directories are deleted in
        // on_reached_directory_post()
        return Action::FTS_OK;
    }

    virtual int on_reached_directory_post() override
    {
        return delete_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_file() override
    {
        return delete_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_symlink() override
    {
        return delete_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_special_file() override
    {
        return delete_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

private:
    bool delete_path()
    {
        if (remove(_curr->fts_accpath) < 0) {
            _error_msg = fmt::format("%s: Failed to remove: %s",
                                     _curr->fts_path, strerror(errno));
            LOGE("%s", _error_msg);
            return false;
        }
        return true;
    }
};

bool delete_recursive(const std::string &path)
{
    RecursiveDeleter deleter(path);
    return deleter.run();
}

}
}