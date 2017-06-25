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

#include "mbcommon/file/filename.h"

#if defined(_WIN32)
#  include "mbcommon/file/win32_p.h"
#elif defined(__ANDROID__)
#  include "mbcommon/file/fd_p.h"
#else
#  include "mbcommon/file/posix_p.h"
#endif

/*!
 * \file mbcommon/file/filename.h
 * \brief Open file with filename
 *
 * * On Windows systems, the mbcommon/file/win32.h API is used
 * * On Android systems, the mbcommon/file/fd.h API is used
 * * On other Unix-like systems, the mbcommon/file/posix.h API is used
 */

/*!
 * \enum MbFileOpenMode
 *
 * \brief Possible file open modes
 */

/*!
 * \var MbFileOpenMode::MB_FILE_OPEN_READ_ONLY
 *
 * \brief Open file for reading.
 *
 * The file pointer is set to the beginning of the file.
 */

/*!
 * \var MbFileOpenMode::MB_FILE_OPEN_READ_WRITE
 *
 * \brief Open file for reading and writing.
 *
 * The file pointer is set to the beginning of the file.
 */

/*!
 * \var MbFileOpenMode::MB_FILE_OPEN_WRITE_ONLY
 *
 * \brief Truncate file and open for writing.
 *
 * The file pointer is set to the beginning of the file.
 */

/*!
 * \var MbFileOpenMode::MB_FILE_OPEN_READ_WRITE_TRUNC
 *
 * \brief Truncate file and open for reading and writing.
 *
 * The file pointer is set to the beginning of the file.
 */

/*!
 * \var MbFileOpenMode::MB_FILE_OPEN_APPEND
 *
 * \brief Open file for appending.
 *
 * The file pointer is set to the end of the file.
 */

/*!
 * \var MbFileOpenMode::MB_FILE_OPEN_READ_APPEND
 *
 * \brief Open file for reading and appending.
 *
 * The file pointer is initially set to the beginning of the file, but writing
 * always occurs at the end of the file.
 */

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

/*!
 * Open MbFile handle from a multi-byte filename.
 *
 * On Unix-like systems, \p filename is used directly. On Windows systems,
 * \p filename is converted to WCS using mb::mbs_to_wcs() before being used.
 *
 * \param file MbFile handle
 * \param filename MBS filename
 * \param mode Open mode (\ref MbFileOpenMode)
 *
 * \return
 *   * #MB_FILE_OK if the file was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_filename(struct MbFile *file,
                          const char *filename, int mode)
{
    return open_func(file, filename, mode);
}

/*!
 * Open MbFile handle from a wide-character filename.
 *
 * On Unix-like systems, \p filename is converted to MBS using mb::wcs_to_mbs()
 * before begin used. On Windows systems, \p filename is used directly.
 *
 * \param file MbFile handle
 * \param filename WCS filename
 * \param mode Open mode (\ref MbFileOpenMode)
 *
 * \return
 *   * #MB_FILE_OK if the file was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_filename_w(struct MbFile *file,
                            const wchar_t *filename, int mode)
{
    return open_w_func(file, filename, mode);
}

MB_END_C_DECLS
