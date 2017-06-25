/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file.h"

#ifdef __cplusplus
#  include <cwchar>
#else
#  include <wchar.h>
#endif

MB_BEGIN_C_DECLS

enum MbFileOpenMode {
    MB_FILE_OPEN_READ_ONLY          = 0,
    MB_FILE_OPEN_READ_WRITE         = 1,
    MB_FILE_OPEN_WRITE_ONLY         = 2,
    MB_FILE_OPEN_READ_WRITE_TRUNC   = 3,
    MB_FILE_OPEN_APPEND             = 4,
    MB_FILE_OPEN_READ_APPEND        = 5,
};

MB_EXPORT int mb_file_open_filename(struct MbFile *file,
                                    const char *filename, int mode);
MB_EXPORT int mb_file_open_filename_w(struct MbFile *file,
                                      const wchar_t *filename, int mode);

MB_END_C_DECLS
