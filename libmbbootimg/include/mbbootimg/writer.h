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

#include <cstdarg>
#include <cstddef>

#include "mbcommon/common.h"

#include "mbbootimg/defs.h"
#include "mbbootimg/writer_error.h"

namespace mb
{
class File;

namespace bootimg
{

class Entry;
class Header;

class WriterPrivate;
class MB_EXPORT Writer
{
    MB_DECLARE_PRIVATE(Writer)

public:
    Writer();
    ~Writer();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(Writer)

    Writer(Writer &&other) noexcept;
    Writer & operator=(Writer &&rhs) noexcept;

    // Open/close
    int open_filename(const std::string &filename);
    int open_filename_w(const std::wstring &filename);
    int open(std::unique_ptr<File> file);
    int open(File *file);
    int close();

    // Operations
    int get_header(Header &header);
    int write_header(const Header &header);
    int get_entry(Entry &entry);
    int write_entry(const Entry &entry);
    int write_data(const void *buf, size_t size, size_t &bytes_written);

    // Format operations
    int format_code();
    std::string format_name();
    int set_format_by_code(int code);
    int set_format_by_name(const std::string &name);

    // Specific formats
    int set_format_android();
    int set_format_bump();
    int set_format_loki();
    int set_format_mtk();
    int set_format_sony_elf();

    // Error handling functions
    std::error_code error();
    std::string error_string();
    int set_error(std::error_code ec);
    MB_PRINTF(3, 4)
    int set_error(std::error_code ec, const char *fmt, ...);
    int set_error_v(std::error_code ec, const char *fmt, va_list ap);

private:
    std::unique_ptr<WriterPrivate> _priv_ptr;
};

}
}
