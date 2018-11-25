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

#include <memory>
#include <string>
#include <vector>

#include <cstddef>

#include "mbcommon/common.h"
#include "mbcommon/outcome.h"

#include "mbbootimg/defs.h"
#include "mbbootimg/reader_error.h"
#include "mbbootimg/reader_p.h"

namespace mb
{
class File;

namespace bootimg
{

class Entry;
class Header;

class MB_EXPORT Reader
{
public:
    Reader();
    ~Reader();

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
    oc::result<void> read_header(Header &header);
    oc::result<void> read_entry(Entry &entry);
    oc::result<void> go_to_entry(Entry &entry, int entry_type);
    oc::result<size_t> read_data(void *buf, size_t size);

    // Format operations
    int format_code();
    std::string format_name();
    oc::result<void> set_format_by_code(int code);
    oc::result<void> set_format_by_name(const std::string &name);
    oc::result<void> enable_format_all();
    oc::result<void> enable_format_by_code(int code);
    oc::result<void> enable_format_by_name(const std::string &name);

    // Specific formats
    oc::result<void> enable_format_android();
    oc::result<void> enable_format_bump();
    oc::result<void> enable_format_loki();
    oc::result<void> enable_format_mtk();
    oc::result<void> enable_format_sony_elf();

    // Reader state
    bool is_open();

private:
    oc::result<void> register_format(std::unique_ptr<detail::FormatReader> format);

    // Global state
    detail::ReaderState m_state;

    // File
    std::unique_ptr<File> m_owned_file;
    File *m_file;

    std::vector<std::unique_ptr<detail::FormatReader>> m_formats;
    detail::FormatReader *m_format;
    bool m_format_user_set;
};

}
}
