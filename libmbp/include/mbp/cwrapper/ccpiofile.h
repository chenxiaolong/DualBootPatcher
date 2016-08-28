/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <stdbool.h>
#include <stddef.h>

#include "mbcommon/common.h"
#include "mbp/cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

MB_EXPORT CCpioFile * mbp_cpiofile_create(void);
MB_EXPORT void mbp_cpiofile_destroy(CCpioFile *cpio);

MB_EXPORT /* enum ErrorCode */ int mbp_cpiofile_error(const CCpioFile *cpio);

MB_EXPORT bool mbp_cpiofile_load_data(CCpioFile *cpio,
                                      const unsigned char *data, size_t size);

MB_EXPORT bool mbp_cpiofile_create_data(CCpioFile *cpio,
                                        unsigned char **data, size_t *size);

MB_EXPORT bool mbp_cpiofile_exists(const CCpioFile *cpio,
                                   const char *filename);
MB_EXPORT bool mbp_cpiofile_remove(CCpioFile *cpio,
                                   const char *filename);

MB_EXPORT char ** mbp_cpiofile_filenames(const CCpioFile *cpio);

MB_EXPORT bool mbp_cpiofile_is_symlink(const CCpioFile *cpio,
                                       const char *filename);

MB_EXPORT char * mbp_cpiofile_symlink_path(const CCpioFile *cpio,
                                           const char *filename);

MB_EXPORT bool mbp_cpiofile_contents(const CCpioFile *cpio,
                                     const char *filename,
                                     const unsigned char **data, size_t *size);
MB_EXPORT bool mbp_cpiofile_set_contents(CCpioFile *cpio,
                                         const char *filename,
                                         const unsigned char *data, size_t size);

MB_EXPORT bool mbp_cpiofile_add_symlink(CCpioFile *cpio,
                                        const char *source, const char *target);
MB_EXPORT bool mbp_cpiofile_add_file(CCpioFile *cpio,
                                     const char *path, const char *name,
                                     unsigned int perms);
MB_EXPORT bool mbp_cpiofile_add_file_from_data(CCpioFile *cpio,
                                               const unsigned char *data, size_t size,
                                               const char *name, unsigned int perms);

#ifdef __cplusplus
}
#endif
