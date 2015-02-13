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

#include "util/chown.h"

#include <cerrno>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include <cppformat/format.h>

#include "util/fts.h"
#include "util/logging.h"

namespace mb
{
namespace util
{

// WARNING: Not thread safe! Android doesn't have getpwnam_r() or getgrnam_r()
static bool chown_internal(const std::string &path,
                           const std::string &user,
                           const std::string &group,
                           bool follow_symlinks)
{
    uid_t uid;
    gid_t gid;

    errno = 0;
    struct passwd *pw = getpwnam(user.c_str());
    if (!pw) {
        if (!errno) {
            errno = EINVAL; // User does not exist
        }
        return false;
    } else {
        uid = pw->pw_uid;
    }

    errno = 0;
    struct group *gr = getgrnam(group.c_str());
    if (!gr) {
        if (!errno) {
            errno = EINVAL; // Group does not exist
        }
        return false;
    } else {
        gid = gr->gr_gid;
    }

    if (follow_symlinks) {
        return ::chown(path.c_str(), uid, gid) == 0;
    } else {
        return ::lchown(path.c_str(), uid, gid) == 0;
    }
}

class RecursiveChown : public FTSWrapper {
public:
    RecursiveChown(std::string path, std::string user, std::string group,
                   bool follow_symlinks)
        : FTSWrapper(path, FTS_GroupSpecialFiles),
        _user(std::move(user)),
        _group(std::move(group)),
        _follow_symlinks(follow_symlinks)
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
        return chown_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_file() override
    {
        return chown_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_symlink() override
    {
        return chown_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_special_file() override
    {
        return chown_path() ? Action::FTS_OK : Action::FTS_Fail;
    }

private:
    std::string _user;
    std::string _group;
    bool _follow_symlinks;

    bool chown_path()
    {
        if (!chown_internal(_curr->fts_accpath, _user, _group, _follow_symlinks)) {
            _error_msg = fmt::format("{}: Failed to chown: {}",
                                     _curr->fts_path, strerror(errno));
            LOGW("{}", _error_msg);
            return false;
        }
        return true;
    }
};

bool chown(const std::string &path,
           const std::string &user,
           const std::string &group,
           int flags)
{
    if (flags & MB_CHOWN_RECURSIVE) {
        RecursiveChown fts(path, user, group, flags & MB_CHOWN_FOLLOW_SYMLINKS);
        return fts.run();
    } else {
        return chown_internal(path, user, group,
                              flags & MB_CHOWN_FOLLOW_SYMLINKS);
    }
}

}
}