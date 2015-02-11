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

#include "util/chmod.h"

#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>

#include <cppformat/format.h>

#include "util/fts.h"
#include "util/logging.h"

namespace MB {

class RecursiveChmod : public FTSWrapper {
public:
    RecursiveChmod(std::string path, mode_t perms)
        : FTSWrapper(path, FTS_GroupSpecialFiles),
        _perms(perms)
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
        return chmod_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_file() override
    {
        return chmod_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_symlink() override
    {
        // Avoid security issue
        LOGW("%s: Not setting permissions on symlink",
             _curr->fts_path);
        return Action::FTS_Skip;
    }

    virtual int on_reached_special_file() override
    {
        return chmod_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

private:
    mode_t _perms;

    bool chmod_path()
    {
        if (chmod(_curr->fts_accpath, _perms) < 0) {
            _error_msg = fmt::format("%s: Failed to chmod: %s",
                                     _curr->fts_path, strerror(errno));
            LOGW("%s", _error_msg);
            return false;
        }
        return true;
    }
};


bool chmod_recursive(const std::string &path, mode_t perms)
{
    RecursiveChmod fts(path, perms);
    return fts.run();
}

}