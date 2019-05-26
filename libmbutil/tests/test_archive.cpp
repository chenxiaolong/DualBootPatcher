/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <gtest/gtest.h>

#include "mbutil/archive.h"

using ScopedArchive = std::unique_ptr<archive, decltype(archive_free) *>;
using ScopedArchiveEntry =
        std::unique_ptr<archive_entry, decltype(archive_entry_free) *>;

TEST(ArchiveTest, CheckUtf8FilenamesWork)
{
    ScopedArchive a(archive_write_new(), archive_write_free);
    ASSERT_TRUE(a);

    // We don't care about the result, so just write to nowhere
    constexpr auto write_fn = [](archive *, void *, const void *, size_t length)
            -> la_ssize_t {
        return static_cast<la_ssize_t>(length);
    };

    ASSERT_EQ(archive_write_set_format_pax_restricted(a.get()), ARCHIVE_OK)
            << archive_error_string(a.get());
    ASSERT_EQ(archive_write_set_bytes_per_block(a.get(), 10240), ARCHIVE_OK)
            << archive_error_string(a.get());
    ASSERT_EQ(archive_write_add_filter_lz4(a.get()), ARCHIVE_OK)
            << archive_error_string(a.get());

    ASSERT_EQ(archive_write_open(a.get(), nullptr, nullptr, write_fn, nullptr),
              ARCHIVE_OK) << archive_error_string(a.get());

    ScopedArchiveEntry entry(archive_entry_new(), archive_entry_free);
    ASSERT_TRUE(entry);

    constexpr std::string_view data = "Hello, world!";

    archive_entry_set_pathname(entry.get(), "你好，世界！");
    archive_entry_set_filetype(entry.get(), AE_IFREG);
    archive_entry_set_perm(entry.get(), 0644);
    archive_entry_set_nlink(entry.get(), 1);
    archive_entry_set_size(entry.get(), data.size());

    ASSERT_EQ(archive_write_header(a.get(), entry.get()), ARCHIVE_OK)
            << archive_error_string(a.get());

    ASSERT_EQ(archive_write_data(a.get(), data.data(), data.size()),
              data.size()) << archive_error_string(a.get());

    ASSERT_EQ(archive_write_close(a.get()), ARCHIVE_OK)
            << archive_error_string(a.get());
}
