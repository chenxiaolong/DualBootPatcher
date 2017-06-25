/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/common.h"

#ifdef __cplusplus
#  include <cstdarg>
#  include <cstddef>
#  include <cstdint>
#else
#  include <stdarg.h>
#  include <stddef.h>
#  include <stdint.h>
#endif

enum MbFileRet
{
    MB_FILE_OK              = 0,
    MB_FILE_RETRY           = -1,
    MB_FILE_UNSUPPORTED     = -2,
    MB_FILE_WARN            = -3,
    MB_FILE_FAILED          = -4,
    MB_FILE_FATAL           = -5,
};

enum MbFileError
{
    MB_FILE_ERROR_NONE              = 0,
    MB_FILE_ERROR_INVALID_ARGUMENT  = 1,
    MB_FILE_ERROR_UNSUPPORTED       = 2,
    MB_FILE_ERROR_PROGRAMMER_ERROR  = 3,
    MB_FILE_ERROR_INTERNAL_ERROR    = 4,
};

MB_BEGIN_C_DECLS

struct MbFile;

typedef int (*MbFileOpenCb)(struct MbFile *file, void *userdata);
typedef int (*MbFileCloseCb)(struct MbFile *file, void *userdata);
typedef int (*MbFileReadCb)(struct MbFile *file, void *userdata,
                            void *buf, size_t size,
                            size_t *bytes_read);
typedef int (*MbFileWriteCb)(struct MbFile *file, void *userdata,
                             const void *buf, size_t size,
                             size_t *bytes_written);
typedef int (*MbFileSeekCb)(struct MbFile *file, void *userdata,
                            int64_t offset, int whence,
                            uint64_t *new_offset);
typedef int (*MbFileTruncateCb)(struct MbFile *file, void *userdata,
                                uint64_t size);

// Handle creation/destruction
MB_EXPORT struct MbFile * mb_file_new();
MB_EXPORT int mb_file_free(struct MbFile *file);

// Callbacks
MB_EXPORT int mb_file_set_open_callback(struct MbFile *file,
                                        MbFileOpenCb open_cb);
MB_EXPORT int mb_file_set_close_callback(struct MbFile *file,
                                         MbFileCloseCb close_cb);
MB_EXPORT int mb_file_set_read_callback(struct MbFile *file,
                                        MbFileReadCb read_cb);
MB_EXPORT int mb_file_set_write_callback(struct MbFile *file,
                                         MbFileWriteCb write_cb);
MB_EXPORT int mb_file_set_seek_callback(struct MbFile *file,
                                        MbFileSeekCb seek_cb);
MB_EXPORT int mb_file_set_truncate_callback(struct MbFile *file,
                                            MbFileTruncateCb truncate_cb);
MB_EXPORT int mb_file_set_callback_data(struct MbFile *file, void *userdata);

// File open/close
MB_EXPORT int mb_file_open(struct MbFile *file);
MB_EXPORT int mb_file_close(struct MbFile *file);

// File operations
MB_EXPORT int mb_file_read(struct MbFile *file, void *buf, size_t size,
                           size_t *bytes_read);
MB_EXPORT int mb_file_write(struct MbFile *file, const void *buf, size_t size,
                            size_t *bytes_written);
MB_EXPORT int mb_file_seek(struct MbFile *file, int64_t offset, int whence,
                           uint64_t *new_offset);
MB_EXPORT int mb_file_truncate(struct MbFile *file, uint64_t size);

// Error handling functions
MB_EXPORT int mb_file_error(struct MbFile *file);
MB_EXPORT const char * mb_file_error_string(struct MbFile *file);
MB_PRINTF(3, 4)
MB_EXPORT int mb_file_set_error(struct MbFile *file, int error_code,
                                const char *fmt, ...);
MB_EXPORT int mb_file_set_error_v(struct MbFile *file, int error_code,
                                  const char *fmt, va_list ap);

MB_END_C_DECLS
