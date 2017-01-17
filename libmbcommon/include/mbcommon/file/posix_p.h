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

#include "mbcommon/guard_p.h"

#include "mbcommon/file/posix.h"
#include "mbcommon/file/vtable_p.h"

/*! \cond INTERNAL */
MB_BEGIN_C_DECLS

struct PosixFileCtx
{
    FILE *fp;
    bool owned;
#ifdef _WIN32
    wchar_t *filename;
    const wchar_t *mode;
#else
    char *filename;
    const char *mode;
#endif

    SysVtable vtable;
};

int _mb_file_open_FILE(SysVtable *vtable, struct MbFile *file, FILE *fp,
                       bool owned);

int _mb_file_open_FILE_filename(SysVtable *vtable, struct MbFile *file,
                                const char *filename, int mode);
int _mb_file_open_FILE_filename_w(SysVtable *vtable, struct MbFile *file,
                                  const wchar_t *filename, int mode);

MB_END_C_DECLS
/*! \endcond */
