/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file.h"

namespace mb
{

enum class FileSearchAction
{
    Continue,
    Stop,
};

using FileSearchResultCallback =
        Expected<FileSearchAction> (*)(File &file, void *userdata,
                                       uint64_t offset);

MB_EXPORT Expected<size_t> file_read_fully(File &file,
                                           void *buf, size_t size);
MB_EXPORT Expected<size_t> file_write_fully(File &file,
                                            const void *buf, size_t size);

MB_EXPORT Expected<uint64_t> file_read_discard(File &file, uint64_t size);

MB_EXPORT Expected<void> file_search(File &file, int64_t start, int64_t end,
                                     size_t bsize, const void *pattern,
                                     size_t pattern_size, int64_t max_matches,
                                     FileSearchResultCallback result_cb,
                                     void *userdata);

MB_EXPORT Expected<uint64_t> file_move(File &file, uint64_t src, uint64_t dest,
                                       uint64_t size);

}
