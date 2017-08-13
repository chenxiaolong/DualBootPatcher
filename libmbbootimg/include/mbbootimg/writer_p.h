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

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"

#define WRITER_ENSURE_STATE(INSTANCE, STATES) \
    do { \
        if (!((INSTANCE)->state & (STATES))) { \
            writer_set_error((INSTANCE), \
                             ERROR_PROGRAMMER_ERROR, \
                             "%s: Invalid state: " \
                             "expected 0x%x, actual: 0x%hx", \
                             __func__, (STATES), (INSTANCE)->state); \
            (INSTANCE)->state = WriterState::FATAL; \
            return RET_FATAL; \
        } \
    } while (0)

#define WRITER_ENSURE_STATE_GOTO(INSTANCE, STATES, RETURN_VAR, LABEL) \
    do { \
        if (!((INSTANCE)->state & (STATES))) { \
            writer_set_error((INSTANCE), \
                             ERROR_PROGRAMMER_ERROR, \
                             "%s: Invalid state: " \
                             "expected 0x%x, actual: 0x%hx", \
                             __func__, (STATES), (INSTANCE)->state); \
            (INSTANCE)->state = WriterState::FATAL; \
            (RETURN_VAR) = RET_FATAL; \
            goto LABEL; \
        } \
    } while (0)

#define MAX_FORMATS     10

namespace mb
{
namespace bootimg
{

struct MbBiWriter;

typedef int (*FormatWriterSetOption)(MbBiWriter *biw, void *userdata,
                                     const char *key, const char *value);
typedef int (*FormatWriterGetHeader)(MbBiWriter *biw, void *userdata,
                                     Header &header);
typedef int (*FormatWriterWriteHeader)(MbBiWriter *biw, void *userdata,
                                       const Header &header);
typedef int (*FormatWriterGetEntry)(MbBiWriter *biw, void *userdata,
                                    Entry &entry);
typedef int (*FormatWriterWriteEntry)(MbBiWriter *biw, void *userdata,
                                      const Entry &entry);
typedef int (*FormatWriterWriteData)(MbBiWriter *biw, void *userdata,
                                     const void *buf, size_t buf_size,
                                     size_t &bytes_written);
typedef int (*FormatWriterFinishEntry)(MbBiWriter *biw, void *userdata);
typedef int (*FormatWriterClose)(MbBiWriter *biw, void *userdata);
typedef int (*FormatWriterFree)(MbBiWriter *biw, void *userdata);

struct FormatWriter
{
    int type;
    std::string name;

    // Callbacks
    FormatWriterSetOption set_option_cb;
    FormatWriterGetHeader get_header_cb;
    FormatWriterWriteHeader write_header_cb;
    FormatWriterGetEntry get_entry_cb;
    FormatWriterWriteEntry write_entry_cb;
    FormatWriterWriteData write_data_cb;
    FormatWriterFinishEntry finish_entry_cb;
    FormatWriterClose close_cb;
    FormatWriterFree free_cb;
    void *userdata;
};

enum WriterState : unsigned short
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

struct MbBiWriter
{
    // Global state
    WriterState state;

    // File
    File *file;
    bool file_owned;

    // Error
    int error_code;
    std::string error_string;

    FormatWriter format;
    bool format_set;

    Entry entry;
    Header header;
};

int _writer_register_format(MbBiWriter *biw,
                            void *userdata,
                            int type,
                            const std::string &name,
                            FormatWriterSetOption set_option_cb,
                            FormatWriterGetHeader get_header_cb,
                            FormatWriterWriteHeader write_header_cb,
                            FormatWriterGetEntry get_entry_cb,
                            FormatWriterWriteEntry write_entry_cb,
                            FormatWriterWriteData write_data_cb,
                            FormatWriterFinishEntry finish_entry_cb,
                            FormatWriterClose close_cb,
                            FormatWriterFree free_cb);

int _writer_free_format(MbBiWriter *biw,
                        FormatWriter *format);

}
}
