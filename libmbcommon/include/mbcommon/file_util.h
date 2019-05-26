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
#include <vector>
#ifdef __ANDROID__
#  include <experimental/functional>
#endif

#include "mbcommon/file.h"

namespace mb
{

namespace detail
{

#ifdef __ANDROID__
namespace std2 = std::experimental;
#else
namespace std2 = std;
#endif

constexpr size_t DEFAULT_BUFFER_SIZE = 8 * 1024 * 1024;

}

MB_EXPORT oc::result<size_t> file_read_retry(File &file,
                                             void *buf, size_t size);
MB_EXPORT oc::result<size_t> file_write_retry(File &file,
                                              const void *buf, size_t size);

MB_EXPORT oc::result<void> file_read_exact(File &file,
                                           void *buf, size_t size);
MB_EXPORT oc::result<void> file_write_exact(File &file,
                                            const void *buf, size_t size);

MB_EXPORT oc::result<uint64_t> file_read_discard(File &file, uint64_t size);

MB_EXPORT oc::result<uint64_t> file_move(File &file, uint64_t src,
                                         uint64_t dest, uint64_t size);

class MB_EXPORT FileSearcher final
{
public:
    FileSearcher(File *file, const void *pattern, size_t pattern_size);

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(FileSearcher)

    FileSearcher(FileSearcher &&other) noexcept;
    FileSearcher & operator=(FileSearcher &&rhs) noexcept;

    oc::result<std::optional<uint64_t>> next();

private:
    void clear() noexcept;

    // File to search
    File *m_file;
    // Pattern to search for
    const void *m_pattern;
    size_t m_pattern_size;
    // Boyer-Moore searcher (NOTE std::byte does not work on Android)
    std::optional<detail::std2::boyer_moore_searcher<
            const unsigned char *>> m_searcher;
    // Search buffer
    std::vector<unsigned char> m_buf;
    // Boundaries of current search area within buffer
    size_t m_region_begin;
    size_t m_region_end;
    // File offset of byte 0 of `m_buf`, relative to the starting point
    uint64_t m_offset;
};

}
