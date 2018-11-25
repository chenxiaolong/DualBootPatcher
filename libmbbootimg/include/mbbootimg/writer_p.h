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

#include <string>

#include <cstddef>

#include "mbcommon/common.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"

namespace mb
{
class File;

namespace bootimg
{
class Writer;

namespace detail
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

    virtual oc::result<void>
    set_option(const char *key, const char *value);
    virtual oc::result<void>
    open(File &file);
    virtual oc::result<void>
    close(File &file);
    virtual oc::result<void>
    get_header(File &file, Header &header) = 0;
    virtual oc::result<void>
    write_header(File &file, const Header &header) = 0;
    virtual oc::result<void>
    get_entry(File &file, Entry &entry) = 0;
    virtual oc::result<void>
    write_entry(File &file, const Entry &entry) = 0;
    virtual oc::result<size_t>
    write_data(File &file, const void *buf, size_t buf_size) = 0;
    virtual oc::result<void>
    finish_entry(File &file);

protected:
    Writer &m_writer;
};

enum class WriterState : uint8_t
{
    New     = 1u << 1,
    Header  = 1u << 2,
    Entry   = 1u << 3,
    Data    = 1u << 4,
    Moved   = 1u << 5,
};
MB_DECLARE_FLAGS(WriterStates, WriterState)
MB_DECLARE_OPERATORS_FOR_FLAGS(WriterStates)

}
}
}
