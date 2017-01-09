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

#include "mbcommon/file/filename.h"

#if defined(_WIN32)
#  include "mbcommon/file/win32_p.h"
#elif defined(__ANDROID__)
#  include "mbcommon/file/fd_p.h"
#else
#  include "mbcommon/file/posix_p.h"
#endif

MB_BEGIN_C_DECLS

typedef int (*VtableOpenFunc)(SysVtable *vtable, struct MbFile *file,
                              const char *filename, int mode);
typedef int (*VtableOpenWFunc)(SysVtable *vtable, struct MbFile *file,
                               const wchar_t *filename, int mode);
typedef int (*OpenFunc)(struct MbFile *file,
                        const char *filename, int mode);
typedef int (*OpenWFunc)(struct MbFile *file,
                         const wchar_t *filename, int mode);

#define SET_FUNCTIONS(TYPE) \
    static VtableOpenFunc vtable_open_func = \
        _mb_file_open_ ## TYPE ## _filename; \
    static VtableOpenWFunc vtable_open_w_func = \
        _mb_file_open_ ## TYPE ## _filename_w; \
    static OpenFunc open_func = \
        mb_file_open_ ## TYPE ## _filename; \
    static OpenWFunc open_w_func = \
        mb_file_open_ ## TYPE ## _filename_w;

#if defined(_WIN32)
SET_FUNCTIONS(HANDLE)
#elif defined(__ANDROID__)
SET_FUNCTIONS(fd)
#else
SET_FUNCTIONS(FILE)
#endif

int _mb_file_open_filename(SysVtable *vtable, struct MbFile *file,
                           const char *filename, int mode)
{
    return vtable_open_func(vtable, file, filename, mode);
}

int _mb_file_open_filename_w(SysVtable *vtable, struct MbFile *file,
                             const wchar_t *filename, int mode)
{
    return vtable_open_w_func(vtable, file, filename, mode);
}

int mb_file_open_filename(struct MbFile *file,
                          const char *filename, int mode)
{
    return open_func(file, filename, mode);
}

int mb_file_open_filename_w(struct MbFile *file,
                            const wchar_t *filename, int mode)
{
    return open_w_func(file, filename, mode);
}

MB_END_C_DECLS
