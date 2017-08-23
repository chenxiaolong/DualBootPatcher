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

#include "mbbootimg/guard_p.h"

#include <memory>
#include <string>

#include <cstddef>

#include "mbcommon/common.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"

namespace mb
{
namespace bootimg
{

class FormatWriter
{
public:
    FormatWriter(Writer &writer);
    virtual ~FormatWriter();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(FormatWriter)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(FormatWriter)

    virtual int type() = 0;
    virtual std::string name() = 0;

    virtual int init();
    virtual int set_option(const char *key, const char *value);
    virtual int get_header(File &file, Header &header) = 0;
    virtual int write_header(File &file, const Header &header) = 0;
    virtual int get_entry(File &file, Entry &entry) = 0;
    virtual int write_entry(File &file, const Entry &entry) = 0;
    virtual int write_data(File &file, const void *buf, size_t buf_size,
                           size_t &bytes_written) = 0;
    virtual int finish_entry(File &file);
    virtual int close(File &file);

protected:
    Writer &_writer;
};

enum class WriterState : uint8_t
{
    New     = 1u << 1,
    Header  = 1u << 2,
    Entry   = 1u << 3,
    Data    = 1u << 4,
    Closed  = 1u << 5,
    Fatal   = 1u << 6,
};
MB_DECLARE_FLAGS(WriterStates, WriterState)
MB_DECLARE_OPERATORS_FOR_FLAGS(WriterStates)

class WriterPrivate
{
    MB_DECLARE_PUBLIC(Writer)

public:
    WriterPrivate(Writer *writer);

    int register_format(std::unique_ptr<FormatWriter> format);

    Writer *_pub_ptr;

    // Global state
    WriterState state;

    // File
    std::unique_ptr<File> owned_file;
    File *file;

    // Error
    std::error_code error_code;
    std::string error_string;

    std::unique_ptr<FormatWriter> format;
};

}
}
