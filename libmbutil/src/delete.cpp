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

#include "mbutil/delete.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/fts.h"
#include "mbutil/string.h"

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
            char *msg = mb_format("%s: Failed to remove: %s",
                                  _curr->fts_path, strerror(errno));
            if (msg) {
                _error_msg = msg;
                free(msg);
            }
            LOGE("%s", _error_msg.c_str());
            return false;
        }
        return true;
    }
};

bool delete_recursive(const std::string &path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) < 0 && errno == ENOENT) {
        // Don't fail if directory does not exist
        return true;
    }

    RecursiveDeleter deleter(path);
    return deleter.run();
}

}
}
