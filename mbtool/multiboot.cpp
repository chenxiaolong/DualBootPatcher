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
#include <sys/stat.h>

#include "util/copy.h"
#include "util/fts.h"
#include "util/logging.h"
#include "util/string.h"


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
            _error_msg = util::format("%s: Failed to remove: %s",
                                      _curr->fts_path, strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return false;
        }
        return true;
    }
};

bool wipe_directory(const std::string &mountpoint, bool wipe_media)
{
    struct stat sb;
    if (stat(mountpoint.c_str(), &sb) < 0 && errno == ENOENT) {
        // Don't fail if directory does not exist
        return true;
    }

    WipeDirectory wd(mountpoint, wipe_media);
    return wd.run();
}


class CopySystem : public util::FTSWrapper {
public:
    CopySystem(std::string path, std::string target)
        : FTSWrapper(path, FTS_GroupSpecialFiles),
        _target(std::move(target))
    {
    }

    virtual int on_changed_path()
    {
        // We only care about the first level
        if (_curr->fts_level != 1) {
            return Action::FTS_Next;
        }

        // Don't copy multiboot directory
        if (strcmp(_curr->fts_name, "multiboot") == 0) {
            return Action::FTS_Skip;
        }

        _curtgtpath.clear();
        _curtgtpath += _target;
        _curtgtpath += "/";
        _curtgtpath += _curr->fts_name;

        return Action::FTS_OK;
    }

    virtual int on_reached_directory_pre() override
    {
        // _target is the correct parameter here (or pathbuf and
        // COPY_EXCLUDE_TOP_LEVEL flag)
        if (!util::copy_dir(_curr->fts_accpath, _target,
                            util::COPY_ATTRIBUTES | util::COPY_XATTRS)) {
            _error_msg = util::format("%s: Failed to copy directory: %s",
                                      _curr->fts_path, strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return Action::FTS_Skip | Action::FTS_Fail;
        }
        return Action::FTS_Skip;
    }

    virtual int on_reached_file() override
    {
        return copy_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_symlink() override
    {
        return copy_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_special_file() override
    {
        return copy_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

private:
    std::string _target;
    std::string _curtgtpath;

    bool copy_path()
    {
        if (!util::copy_file(_curr->fts_accpath, _curtgtpath,
                             util::COPY_ATTRIBUTES | util::COPY_XATTRS)) {
            _error_msg = util::format("%s: Failed to copy file: %s",
                                      _curr->fts_path, strerror(errno));
            LOGW("%s", _error_msg.c_str());
            return false;
        }
        return true;
    }
};

/*!
 * \brief Copy /system directory excluding multiboot files
 *
 * \param source Source directory
 * \param target Target directory
 */
bool copy_system(const std::string &source, const std::string &target)
{
    CopySystem fts(source, target);
    return fts.run();
}

}
