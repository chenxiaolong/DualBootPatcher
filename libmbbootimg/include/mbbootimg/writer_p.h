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

#ifdef __cplusplus
#  include <cstddef>
#else
#  include <stddef.h>
#endif

#include "mbcommon/common.h"

#define WRITER_ENSURE_STATE(INSTANCE, STATES) \
    do { \
        if (!((INSTANCE)->state & (STATES))) { \
            mb_bi_writer_set_error((INSTANCE), \
                                   MB_BI_ERROR_PROGRAMMER_ERROR, \
                                   "%s: Invalid state: " \
                                   "expected 0x%x, actual: 0x%hx", \
                                   __func__, (STATES), (INSTANCE)->state); \
            (INSTANCE)->state = WriterState::FATAL; \
            return MB_BI_FATAL; \
        } \
    } while (0)

#define WRITER_ENSURE_STATE_GOTO(INSTANCE, STATES, RETURN_VAR, LABEL) \
    do { \
        if (!((INSTANCE)->state & (STATES))) { \
            mb_bi_writer_set_error((INSTANCE), \
                                   MB_BI_ERROR_PROGRAMMER_ERROR, \
                                   "%s: Invalid state: " \
                                   "expected 0x%x, actual: 0x%hx", \
                                   __func__, (STATES), (INSTANCE)->state); \
            (INSTANCE)->state = WriterState::FATAL; \
            (RETURN_VAR) = MB_BI_FATAL; \
            goto LABEL; \
        } \
    } while (0)

#define MAX_FORMATS     10

MB_BEGIN_C_DECLS

struct MbBiWriter;
struct MbBiEntry;
struct MbBiHeader;
struct MbFile;

typedef int (*FormatWriterSetOption)(struct MbBiWriter *biw, void *userdata,
                                     const char *key, const char *value);
typedef int (*FormatWriterGetHeader)(struct MbBiWriter *biw, void *userdata,
                                     struct MbBiHeader *header);
typedef int (*FormatWriterWriteHeader)(struct MbBiWriter *biw, void *userdata,
                                       struct MbBiHeader *header);
typedef int (*FormatWriterGetEntry)(struct MbBiWriter *biw, void *userdata,
                                    struct MbBiEntry *entry);
typedef int (*FormatWriterWriteEntry)(struct MbBiWriter *biw, void *userdata,
                                      struct MbBiEntry *entry);
typedef int (*FormatWriterWriteData)(struct MbBiWriter *biw, void *userdata,
                                     const void *buf, size_t buf_size,
                                     size_t *bytes_written);
typedef int (*FormatWriterFinishEntry)(struct MbBiWriter *biw, void *userdata);
typedef int (*FormatWriterClose)(struct MbBiWriter *biw, void *userdata);
typedef int (*FormatWriterFree)(struct MbBiWriter *biw, void *userdata);

struct FormatWriter
{
    int type;
    char *name;

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
    struct MbFile *file;
    bool file_owned;

    // Error
    int error_code;
    char *error_string;

    struct FormatWriter format;
    bool format_set;

    struct MbBiEntry *entry;
    struct MbBiHeader *header;
};

int _mb_bi_writer_register_format(struct MbBiWriter *biw,
                                  void *userdata,
                                  int type,
                                  const char *name,
                                  FormatWriterSetOption set_option_cb,
                                  FormatWriterGetHeader get_header_cb,
                                  FormatWriterWriteHeader write_header_cb,
                                  FormatWriterGetEntry get_entry_cb,
                                  FormatWriterWriteEntry write_entry_cb,
                                  FormatWriterWriteData write_data_cb,
                                  FormatWriterFinishEntry finish_entry_cb,
                                  FormatWriterClose close_cb,
                                  FormatWriterFree free_cb);

int _mb_bi_writer_free_format(struct MbBiWriter *biw,
                              struct FormatWriter *format);

MB_END_C_DECLS
