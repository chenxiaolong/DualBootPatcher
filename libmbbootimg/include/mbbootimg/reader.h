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

#include <cstdarg>
#include <cstddef>

#include "mbcommon/common.h"

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
    bool open_filename(const std::string &filename);
    bool open_filename_w(const std::wstring &filename);
    bool open(std::unique_ptr<File> file);
    bool open(File *file);
    bool close();

    // Operations
    bool read_header(Header &header);
    bool read_entry(Entry &entry);
    bool go_to_entry(Entry &entry, int entry_type);
    bool read_data(void *buf, size_t size, size_t &bytes_read);

    // Format operations
    int format_code();
    std::string format_name();
    bool set_format_by_code(int code);
    bool set_format_by_name(const std::string &name);
    bool enable_format_all();
    bool enable_format_by_code(int code);
    bool enable_format_by_name(const std::string &name);

    // Specific formats
    bool enable_format_android();
    bool enable_format_bump();
    bool enable_format_loki();
    bool enable_format_mtk();
    bool enable_format_sony_elf();

    // Reader state
    bool is_open();
    bool is_fatal();
    void set_fatal();

    // Error handling
    std::error_code error();
    std::string error_string();
    bool set_error(std::error_code ec);
    MB_PRINTF(3, 4)
    bool set_error(std::error_code ec, const char *fmt, ...);
    bool set_error_v(std::error_code ec, const char *fmt, va_list ap);

private:
    bool register_format(std::unique_ptr<detail::FormatReader> format);

    // Global state
    detail::ReaderState m_state;

    // File
    std::unique_ptr<File> m_owned_file;
    File *m_file;

    // Error
    std::error_code m_error_code;
    std::string m_error_string;

    std::vector<std::unique_ptr<detail::FormatReader>> m_formats;
    detail::FormatReader *m_format;
};

}
}
