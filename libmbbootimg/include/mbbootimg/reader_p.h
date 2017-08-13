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

#include <cstddef>

#include "mbcommon/common.h"
#include "mbcommon/file.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"

#define READER_ENSURE_STATE(INSTANCE, STATES) \
    do { \
        if (!((INSTANCE)->state & (STATES))) { \
            mb_bi_reader_set_error((INSTANCE), \
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
            mb_bi_reader_set_error((INSTANCE), \
                                   ERROR_PROGRAMMER_ERROR, \
                                   "%s: Invalid state: " \
                                   "expected 0x%x, actual: 0x%hx", \
                                   __func__, (STATES), (INSTANCE)->state); \
            (INSTANCE)->state = ReaderState::FATAL; \
            (RETURN_VAR) = RET_FATAL; \
            goto LABEL; \
        } \
    } while (0)

#define MAX_FORMATS     10

namespace mb
{
namespace bootimg
{

struct MbBiReader;

typedef int (*FormatReaderBidder)(MbBiReader *bir, void *userdata,
                                  int best_bid);
typedef int (*FormatReaderSetOption)(MbBiReader *bir, void *userdata,
                                     const char *key, const char *value);
typedef int (*FormatReaderReadHeader)(MbBiReader *bir, void *userdata,
                                      Header &header);
typedef int (*FormatReaderReadEntry)(MbBiReader *bir, void *userdata,
                                     Entry &entry);
typedef int (*FormatReaderGoToEntry)(MbBiReader *bir, void *userdata,
                                     Entry &entry, int entry_type);
typedef int (*FormatReaderReadData)(MbBiReader *bir, void *userdata,
                                    void *buf, size_t buf_size,
                                    size_t &bytes_read);
typedef int (*FormatReaderFree)(MbBiReader *bir, void *userdata);

struct FormatReader
{
    int type;
    std::string name;

    // Callbacks
    FormatReaderBidder bidder_cb;
    FormatReaderSetOption set_option_cb;
    FormatReaderReadHeader read_header_cb;
    FormatReaderReadEntry read_entry_cb;
    FormatReaderGoToEntry go_to_entry_cb;
    FormatReaderReadData read_data_cb;
    FormatReaderFree free_cb;
    void *userdata;
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

    FormatReader formats[MAX_FORMATS];
    size_t formats_len;
    FormatReader *format;

    Header header;
    Entry entry;
};

int _mb_bi_reader_register_format(MbBiReader *bir,
                                  void *userdata,
                                  int type,
                                  const std::string &name,
                                  FormatReaderBidder bidder_cb,
                                  FormatReaderSetOption set_option_cb,
                                  FormatReaderReadHeader read_header_cb,
                                  FormatReaderReadEntry read_entry_cb,
                                  FormatReaderGoToEntry go_to_entry_cb,
                                  FormatReaderReadData read_data_cb,
                                  FormatReaderFree free_cb);

int _mb_bi_reader_free_format(MbBiReader *bir,
                              FormatReader *format);

}
}
