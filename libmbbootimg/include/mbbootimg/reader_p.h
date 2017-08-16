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

#define READER_ENSURE_STATE(INSTANCE, STATES) \
    do { \
        if (!((INSTANCE)->state & (STATES))) { \
            reader_set_error((INSTANCE), \
                             ERROR_PROGRAMMER_ERROR, \
                             "%s: Invalid state: " \
                             "expected 0x%x, actual: 0x%hx", \
                             __func__, (STATES), (INSTANCE)->state); \
            (INSTANCE)->state = ReaderState::FATAL; \
            return RET_FATAL; \
        } \
    } while (0)

#define READER_ENSURE_STATE_GOTO(INSTANCE, STATES, RETURN_VAR, LABEL) \
    do { \
        if (!((INSTANCE)->state & (STATES))) { \
            reader_set_error((INSTANCE), \
                             ERROR_PROGRAMMER_ERROR, \
                             "%s: Invalid state: " \
                             "expected 0x%x, actual: 0x%hx", \
                             __func__, (STATES), (INSTANCE)->state); \
            (INSTANCE)->state = ReaderState::FATAL; \
            (RETURN_VAR) = RET_FATAL; \
            goto LABEL; \
        } \
    } while (0)

namespace mb
{
namespace bootimg
{

constexpr size_t MAX_FORMATS = 10;

class FormatReader
{
public:
    FormatReader(MbBiReader *bir);
    virtual ~FormatReader();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(FormatReader)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(FormatReader)

    virtual int type() = 0;
    virtual std::string name() = 0;

    virtual int init();
    virtual int bid(int best_bid) = 0;
    virtual int set_option(const char *key, const char *value);
    virtual int read_header(Header &header) = 0;
    virtual int read_entry(Entry &entry) = 0;
    virtual int go_to_entry(Entry &entry, int entry_type);
    virtual int read_data(void *buf, size_t buf_size, size_t &bytes_read) = 0;

protected:
    MbBiReader *_bir;
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
    ANY_NONFATAL    = NEW | HEADER | ENTRY | DATA | CLOSED,
    ANY             = ANY_NONFATAL | FATAL,
};

struct MbBiReader
{
    // Global state
    ReaderState state;

    // File
    File *file;
    bool file_owned;

    // Error
    int error_code;
    std::string error_string;

    std::vector<std::unique_ptr<FormatReader>> formats;
    FormatReader *format;

    Header header;
    Entry entry;
};

int _reader_register_format(MbBiReader *bir,
                            std::unique_ptr<FormatReader> format);

}
}
