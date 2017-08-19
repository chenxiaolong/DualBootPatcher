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

#include <string>
#include <vector>

#include <cstddef>

#include "mbcommon/common.h"
#include "mbcommon/file.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"

namespace mb
{
namespace bootimg
{

constexpr size_t MAX_FORMATS = 10;

class FormatReader
{
public:
    FormatReader(Reader &reader);
    virtual ~FormatReader();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(FormatReader)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(FormatReader)

    virtual int type() = 0;
    virtual std::string name() = 0;

    virtual int init();
    virtual int set_option(const char *key, const char *value);
    virtual int bid(File &file, int best_bid) = 0;
    virtual int read_header(File &file, Header &header) = 0;
    virtual int read_entry(File &file, Entry &entry) = 0;
    virtual int go_to_entry(File &file, Entry &entry, int entry_type);
    virtual int read_data(File &file, void *buf, size_t buf_size,
                          size_t &bytes_read) = 0;

protected:
    Reader &_reader;
};

enum ReaderState : unsigned short
{
    NEW             = 1U << 1,
    HEADER          = 1U << 2,
    ENTRY           = 1U << 3,
    DATA            = 1U << 4,
    CLOSED          = 1U << 5,
    FATAL           = 1U << 6,
    // Grouped
    ANY             = NEW | HEADER | ENTRY | DATA | CLOSED | FATAL,
};

class ReaderPrivate
{
    MB_DECLARE_PUBLIC(Reader)

public:
    ReaderPrivate(Reader *reader);

    int register_format(std::unique_ptr<FormatReader> format);

    Reader *_pub_ptr;

    // Global state
    ReaderState state;

    // File
    std::unique_ptr<File> owned_file;
    File *file;

    // Error
    int error_code;
    std::string error_string;

    std::vector<std::unique_ptr<FormatReader>> formats;
    FormatReader *format;
};

}
}
