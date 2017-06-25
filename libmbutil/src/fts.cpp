/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/fts.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>

#include "mbcommon/string.h"
#include "mbutil/string.h"


namespace mb
{
namespace util
{

FTSWrapper::FTSWrapper(std::string path, int flags)
{
    _path = std::move(path);

    // Flags
    _flags = flags;
}

FTSWrapper::~FTSWrapper()
{
    if (_ftsp) {
        fts_close(_ftsp);
    }
}

bool FTSWrapper::run()
{
    if (_ran) {
        _error_msg = "Already ran";
        return false;
    }
    _ran = true;

    bool ret = true;
    int fts_flags = 0;
    int result;

    // Don't change directories
    fts_flags += FTS_NOCHDIR;

    // Don't follow symlinks by default
    if (_flags & FTS_FollowSymlinks) {
        fts_flags |= FTS_LOGICAL;
    } else {
        fts_flags |= FTS_PHYSICAL;
    }

    // Don't cross mountpoint boundaries by default
    if ((_flags & FTS_CrossMountPointBoundaries) == 0) {
        fts_flags |= FTS_XDEV;
    }

    // Pre-execute hook
    if (!on_pre_execute()) {
        return false;
    }

    // We only support traversal of one tree
    char *files[] = { (char *) _path.c_str(), nullptr };

    _ftsp = fts_open(files, fts_flags, nullptr);
    if (!_ftsp) {
        char *msg = mb_format("fts_open failed: %s", strerror(errno));
        if (msg) {
            _error_msg = msg;
            free(msg);
        }
        ret = false;
    }

    while (_ftsp && (_curr = fts_read(_ftsp))) {
        switch (_curr->fts_info) {
        case FTS_NS:  // no stat()
        case FTS_DNR: // directory not read
        case FTS_ERR: { // other error
            char *msg = mb_format("fts_read error: %s",
                                  strerror(_curr->fts_errno));
            if (msg) {
                _error_msg = msg;
                free(msg);
            }
            ret = false;
            break;
        }

        case FTS_DC:   // Not reached unless FTS_LOGICAL specified
                       // (FTS_PHYSICAL doesn't follow symlinks)
        case FTS_DOT:  // Not reached unless FTS_SEEDOT specified
        case FTS_NSOK: // Not reached unless FTS_NOSTAT specified
            continue;
        }

        if (_curr->fts_level == 0) {
            _root = _curr;
        }

        // Current path hook
        _error_msg = "Handler returned failure";
        result = on_changed_path();
        if (result & FTS_Fail) {
            ret = false;
        }
        if (result & FTS_Next) {
            continue;
        }
        if (result & FTS_Skip) {
            fts_set(_ftsp, _curr, FTS_SKIP);
            continue;
        }
        if (result & FTS_Stop) {
            break;
        }

        // Call other hooks
        _error_msg = "Handler returned failure";

        switch (_curr->fts_info) {
        case FTS_D: result = on_reached_directory_pre(); break;
        case FTS_DP: result = on_reached_directory_post(); break;
        case FTS_F: result = on_reached_file(); break;
        case FTS_SL:
        case FTS_SLNONE: result = on_reached_symlink(); break;
        case FTS_DEFAULT:
            if (_flags & FTS_GroupSpecialFiles) {
                result = on_reached_special_file();
            } else {
                switch (_curr->fts_statp->st_mode & S_IFMT) {
                case S_IFBLK: result = on_reached_block_device(); break;
                case S_IFCHR: result = on_reached_character_device(); break;
                case S_IFIFO: result = on_reached_fifo(); break;
                case S_IFSOCK: result = on_reached_socket(); break;
                default: result = Action::FTS_Skip; break;
                }
            }
        }

        // Handle result
        if (result & FTS_Fail) {
            ret = false;
        }
        if (result & FTS_Skip) {
            fts_set(_ftsp, _curr, FTS_SKIP);
            continue;
        }
        if (result & FTS_Stop) {
            break;
        }
    }

    if (!on_post_execute(ret)) {
        return false;
    }

    return ret;
}

std::string FTSWrapper::error()
{
    return _error_msg;
}

bool FTSWrapper::on_pre_execute() {
    return true;
}

bool FTSWrapper::on_post_execute(bool success)
{
    (void) success;
    return true;
}

int FTSWrapper::on_changed_path()
{
    return Action::FTS_OK;
}

int FTSWrapper::on_reached_directory_pre()
{
    return Action::FTS_OK;
}

int FTSWrapper::on_reached_directory_post()
{
    return Action::FTS_OK;
}

int FTSWrapper::on_reached_file()
{
    return Action::FTS_OK;
}

int FTSWrapper::on_reached_symlink()
{
    return Action::FTS_OK;
}

int FTSWrapper::on_reached_special_file()
{
    return Action::FTS_OK;
}

int FTSWrapper::on_reached_block_device()
{
    return Action::FTS_OK;
}

int FTSWrapper::on_reached_character_device()
{
    return Action::FTS_OK;
}

int FTSWrapper::on_reached_fifo()
{
    return Action::FTS_OK;
}

int FTSWrapper::on_reached_socket()
{
    return Action::FTS_OK;
}

}
}
