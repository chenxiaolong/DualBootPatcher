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

#include "multiboot.h"

#include <cerrno>
#include <cstring>

#include "external/cppformat/format.h"
#include "util/fts.h"
#include "util/logging.h"


namespace mb
{

class WipeDirectory : public util::FTSWrapper {
public:
    WipeDirectory(std::string path, bool wipe_media)
        : FTSWrapper(path, FTS_GroupSpecialFiles),
        _wipe_media(wipe_media)
    {
    }

    virtual int on_changed_path()
    {
        // Don't wipe 'multiboot' and 'media' directories on the first level
        if (_curr->fts_level == 1) {
            if (strcmp(_curr->fts_name, "multiboot") == 0
                    || (!_wipe_media && strcmp(_curr->fts_name, "media") == 0)) {
                return Action::FTS_Skip;
            }
        }

        return Action::FTS_OK;
    }

    virtual int on_reached_directory_pre() override
    {
        // Do nothing. Need depth-first search, so directories are deleted
        // in FTS_DP
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
    bool _wipe_media;

    bool delete_path()
    {
        if (_curr->fts_level >= 1 && remove(_curr->fts_accpath) < 0) {
            _error_msg = fmt::format("%s: Failed to remove: %s",
                                     _curr->fts_path, strerror(errno));
            LOGW("%s", _error_msg);
            return false;
        }
        return true;
    }
};

bool wipe_directory(const std::string &mountpoint, bool wipe_media)
{
    WipeDirectory wd(mountpoint, wipe_media);
    return wd.run();
}

}