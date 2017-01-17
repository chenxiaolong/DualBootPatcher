/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "mbcommon/file/filename.h"

#ifdef __cplusplus
#  include <cstdbool>
#  include <cstdio>
#else
#  include <stdbool.h>
#  include <stdio.h>
#endif

MB_BEGIN_C_DECLS

MB_EXPORT int mb_file_open_FILE(struct MbFile *file,
                                FILE *fp, bool owned);

MB_EXPORT int mb_file_open_FILE_filename(struct MbFile *file,
                                         const char *filename, int mode);
MB_EXPORT int mb_file_open_FILE_filename_w(struct MbFile *file,
                                           const wchar_t *filename, int mode);

MB_END_C_DECLS
