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

#include "mbcommon/common.h"

#ifdef __cplusplus
#include <cinttypes>
#include <cstdbool>

extern "C" {
#else
#include <inttypes.h>
#include <stdbool.h>
#endif

MB_EXPORT int64_t get_mnt_total_size(const char *mountpoint);
MB_EXPORT int64_t get_mnt_avail_size(const char *mountpoint);

MB_EXPORT bool is_same_file(const char *path1, const char *path2);

MB_EXPORT bool extract_archive(const char *filename, const char *target);

MB_EXPORT bool find_string_in_file(const char *path, const char *str, int *result);

MB_EXPORT void mblog_set_logcat();

MB_EXPORT int get_pid();

MB_EXPORT char * read_link(const char *path);

#ifdef __cplusplus
}
#endif
