/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <functional>
#include <optional>

#include "mbcommon/file.h"

namespace mb
{

enum class FileSearchAction
{
    Continue,
    Stop,
};

using FileSearchResultCallback = std::function<oc::result<FileSearchAction>
        (File &file, uint64_t offset)>;

MB_EXPORT oc::result<size_t> file_read_retry(File &file,
                                             void *buf, size_t size);
MB_EXPORT oc::result<size_t> file_write_retry(File &file,
                                              const void *buf, size_t size);

MB_EXPORT oc::result<void> file_read_exact(File &file,
                                           void *buf, size_t size);
MB_EXPORT oc::result<void> file_write_exact(File &file,
                                            const void *buf, size_t size);

MB_EXPORT oc::result<uint64_t> file_read_discard(File &file, uint64_t size);

MB_EXPORT oc::result<void> file_search(File &file,
                                       std::optional<uint64_t> start,
                                       std::optional<uint64_t> end,
                                       size_t bsize, const void *pattern,
                                       size_t pattern_size,
                                       std::optional<uint64_t> max_matches,
                                       const FileSearchResultCallback &result_cb);

MB_EXPORT oc::result<uint64_t> file_move(File &file, uint64_t src,
                                         uint64_t dest, uint64_t size);

}
