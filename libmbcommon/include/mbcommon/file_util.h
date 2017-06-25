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

#include "mbcommon/file.h"

MB_BEGIN_C_DECLS

typedef int (*MbFileSearchResultCallback)(struct MbFile *file, void *userdata,
                                          uint64_t offset);

MB_EXPORT int mb_file_read_fully(struct MbFile *file,
                                 void *buf, size_t size,
                                 size_t *bytes_read);
MB_EXPORT int mb_file_write_fully(struct MbFile *file,
                                  const void *buf, size_t size,
                                  size_t *bytes_written);

MB_EXPORT int mb_file_read_discard(struct MbFile *file, uint64_t size,
                                   uint64_t *bytes_discarded);

MB_EXPORT int mb_file_search(struct MbFile *file, int64_t start, int64_t end,
                             size_t bsize, const void *pattern,
                             size_t pattern_size, int64_t max_matches,
                             MbFileSearchResultCallback result_cb,
                             void *userdata);

MB_EXPORT int mb_file_move(struct MbFile *file, uint64_t src, uint64_t dest,
                           uint64_t size, uint64_t *size_moved);

MB_END_C_DECLS
