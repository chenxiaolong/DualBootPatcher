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

#pragma once

#include <string>
#include <vector>

#include <fts.h>

namespace mb
{
namespace util
{

class FTSWrapper {
public:
    enum Flags : int {
        // Follow symlinks while traversing (WARNING: dangerous!)
        FTS_FollowSymlinks              = 0x1,
        // If tree contains a mountpoint, traverse its contents
        FTS_CrossMountPointBoundaries   = 0x2,
        // Call on_reached_special_file() instead of separate functions
        FTS_GroupSpecialFiles           = 0x4
    };

    enum Action : int {
        // Hook succeeded
        FTS_OK                          = 0x0,
        // Hook failed (run() will return false)
        FTS_Fail                        = 0x1,
        // Skip current file or tree (in case of directory)
        FTS_Skip                        = 0x2,
        // Stop traversal (if specified with FTS_Skip, behavior is undefined)
        FTS_Stop                        = 0x4,
        // Go to next entry (only useful for on_changed_path(). If this is
        // returned, then the on_reached_*() functions will not be called)
        FTS_Next                        = 0x8
    };

    FTSWrapper(std::string path, int flags);
    virtual ~FTSWrapper();

    bool run();
    std::string error();

    virtual bool on_pre_execute();
    virtual bool on_post_execute(bool success);
    virtual int on_changed_path();
    virtual int on_reached_directory_pre();
    virtual int on_reached_directory_post();
    virtual int on_reached_file();
    virtual int on_reached_symlink();
    virtual int on_reached_special_file();

    // Special files
    virtual int on_reached_block_device();
    virtual int on_reached_character_device();
    virtual int on_reached_fifo();
    virtual int on_reached_socket();

protected:
    // Input path
    std::string _path;
    // Input flags
    int _flags = 0;
    // fts pointer
    FTS *_ftsp = nullptr;
    // Current fts entry
    FTSENT *_curr = nullptr;
    // Root (level 0) fts entry
    FTSENT *_root = nullptr;
    // Error message (valid only if run() returned false)
    std::string _error_msg;

private:
    bool _ran = false;
};

}
}