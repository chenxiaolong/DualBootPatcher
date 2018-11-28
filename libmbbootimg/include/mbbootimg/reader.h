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

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <cstddef>

#include "mbcommon/common.h"
#include "mbcommon/outcome.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader_error.h"
#include "mbbootimg/reader_p.h"

namespace mb
{
class File;

namespace bootimg
{

class MB_EXPORT Reader
{
public:
    Reader() noexcept;
    ~Reader() noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(Reader)

    Reader(Reader &&other) noexcept;
    Reader & operator=(Reader &&rhs) noexcept;

    // Open/close
    oc::result<void> open_filename(const std::string &filename);
    oc::result<void> open_filename_w(const std::wstring &filename);
    oc::result<void> open(std::unique_ptr<File> file);
    oc::result<void> open(File *file);
    oc::result<void> close();

    // Operations
    oc::result<Header> read_header();
    oc::result<Entry> read_entry();
    oc::result<Entry> go_to_entry(std::optional<EntryType> entry_type);
    oc::result<size_t> read_data(void *buf, size_t size);

    // Format operations
    std::optional<Format> format();
    oc::result<void> enable_formats(Formats formats);
    oc::result<void> enable_formats_all();

    // Reader state
    bool is_open();

private:
    // Global state
    detail::ReaderState m_state;

    // File
    std::unique_ptr<File> m_owned_file;
    File *m_file;

    std::vector<std::unique_ptr<detail::FormatReader>> m_formats;
    detail::FormatReader *m_format;
};

}
}
