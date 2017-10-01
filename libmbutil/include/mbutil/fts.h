/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <string>
#include <vector>

#include <fts.h>

#include "mbcommon/flags.h"

namespace mb
{
namespace util
{

enum class FtsFlag : uint8_t
{
    // Follow symlinks while traversing (WARNING: dangerous!)
    FollowSymlinks              = 1 << 0,
    // If tree contains a mountpoint, traverse its contents
    CrossMountPointBoundaries   = 1 << 1,
    // Call on_reached_special_file() instead of separate functions
    GroupSpecialFiles           = 1 << 2,
};
MB_DECLARE_FLAGS(FtsFlags, FtsFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(FtsFlags)

class FtsWrapper
{
public:
    enum class Action : uint8_t
    {
        // Hook succeeded
        Ok      = 1 << 0,
        // Hook failed (run() will return false)
        Fail    = 1 << 1,
        // Skip current file or tree (in case of directory)
        Skip    = 1 << 2,
        // Stop traversal (if specified with Skip, behavior is undefined)
        Stop    = 1 << 3,
        // Go to next entry (only useful for on_changed_path(). If this is
        // returned, then the on_reached_*() functions will not be called)
        Next    = 1 << 4,
    };
    MB_DECLARE_FLAGS(Actions, Action)

    FtsWrapper(std::string path, FtsFlags flags);
    virtual ~FtsWrapper();

    bool run();
    std::string error();

    virtual bool on_pre_execute();
    virtual bool on_post_execute(bool success);
    virtual Actions on_changed_path();
    virtual Actions on_reached_directory_pre();
    virtual Actions on_reached_directory_post();
    virtual Actions on_reached_file();
    virtual Actions on_reached_symlink();
    virtual Actions on_reached_special_file();

    // Special files
    virtual Actions on_reached_block_device();
    virtual Actions on_reached_character_device();
    virtual Actions on_reached_fifo();
    virtual Actions on_reached_socket();

protected:
    // Input path
    std::string _path;
    // Input flags
    FtsFlags _flags;
    // fts pointer
    FTS *_ftsp;
    // Current fts entry
    FTSENT *_curr;
    // Root (level 0) fts entry
    FTSENT *_root;
    // Error message (valid only if run() returned false)
    std::string _error_msg;

private:
    bool _ran;
};

MB_DECLARE_OPERATORS_FOR_FLAGS(FtsWrapper::Actions)

}
}
