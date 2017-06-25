/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cerrno>
#include <cstdlib>

#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/fts.h"
#include "mbutil/string.h"

namespace mb
{
namespace util
{

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
        if (::chmod(_curr->fts_accpath, _perms) < 0) {
            char *msg = mb_format("%s: Failed to chmod: %s",
                                  _curr->fts_path, strerror(errno));
            if (msg) {
                _error_msg = msg;
                free(msg);
            }
            LOGW("%s", _error_msg.c_str());
            return false;
        }
        return true;
    }
};


bool chmod(const std::string &path, mode_t perms, int flags)
{
    if (flags & CHMOD_RECURSIVE) {
        RecursiveChmod fts(path, perms);
        return fts.run();
    } else {
        return ::chmod(path.c_str(), perms) == 0;
    }
}

}
}
