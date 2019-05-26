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

#include "mbutil/chown.h"

#include <cerrno>
#include <cstdlib>

#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbutil/fts.h"


namespace mb::util
{

static oc::result<void> chown_internal(const std::string &path,
                                       uid_t uid,
                                       gid_t gid,
                                       bool follow_symlinks)
{
    int ret;

    if (follow_symlinks) {
        ret = ::chown(path.c_str(), uid, gid);
    } else {
        ret = ::lchown(path.c_str(), uid, gid);
    }

    if (ret < 0) {
        return ec_from_errno();
    }

    return oc::success();
}

class RecursiveChown : public FtsWrapper {
public:
    std::error_code ec;

    RecursiveChown(std::string path, uid_t uid, gid_t gid,
                   bool follow_symlinks)
        : FtsWrapper(std::move(path), FtsFlag::GroupSpecialFiles)
        , _uid(uid)
        , _gid(gid)
        , _follow_symlinks(follow_symlinks)
    {
    }

    Actions on_reached_directory_post() override
    {
        return chown_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_file() override
    {
        return chown_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_symlink() override
    {
        return chown_path() ? Action::Ok : Action::Fail;
    }

    Actions on_reached_special_file() override
    {
        return chown_path() ? Action::Ok : Action::Fail;
    }

private:
    uid_t _uid;
    gid_t _gid;
    bool _follow_symlinks;

    bool chown_path()
    {
        if (auto r = chown_internal(
                _curr->fts_accpath, _uid, _gid, _follow_symlinks); !r) {
            ec = r.error();
            return false;
        }
        return true;
    }
};

// WARNING: Not thread safe! Android doesn't have getpwnam_r() or getgrnam_r()
oc::result<void> chown(const std::string &path,
                       const std::string &user,
                       const std::string &group,
                       ChownFlags flags)
{
    uid_t uid;
    gid_t gid;

    errno = 0;
    struct passwd *pw = getpwnam(user.c_str());
    if (!pw) {
        if (!errno) {
            errno = EINVAL; // User does not exist
        }
        return ec_from_errno();
    } else {
        uid = pw->pw_uid;
    }

    errno = 0;
    struct group *gr = getgrnam(group.c_str());
    if (!gr) {
        if (!errno) {
            errno = EINVAL; // Group does not exist
        }
        return ec_from_errno();
    } else {
        gid = gr->gr_gid;
    }

    return chown(path, uid, gid, flags);
}

oc::result<void> chown(const std::string &path,
                       uid_t uid,
                       gid_t gid,
                       ChownFlags flags)
{
    if (flags & ChownFlag::Recursive) {
        RecursiveChown fts(path, uid, gid, flags & ChownFlag::FollowSymlinks);
        if (!fts.run()) {
            return fts.ec;
        } else {
            return oc::success();
        }
    } else {
        return chown_internal(path, uid, gid, flags & ChownFlag::FollowSymlinks);
    }
}

}
