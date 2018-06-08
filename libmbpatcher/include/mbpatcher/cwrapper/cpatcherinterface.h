/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <stdbool.h>
#include <stdint.h>

#include "mbcommon/common.h"
#include "mbpatcher/cwrapper/ctypes.h"

MB_BEGIN_C_DECLS

typedef void (*ProgressUpdatedCallback) (uint64_t, uint64_t, void *);
typedef void (*FilesUpdatedCallback) (uint64_t, uint64_t, void *);
typedef void (*DetailsUpdatedCallback) (const char *, void *);

MB_EXPORT /* enum ErrorCode */ int mbpatcher_patcher_error(const CPatcher *patcher);
MB_EXPORT char * mbpatcher_patcher_id(const CPatcher *patcher);
MB_EXPORT void mbpatcher_patcher_set_fileinfo(CPatcher *patcher, const CFileInfo *info);
MB_EXPORT bool mbpatcher_patcher_patch_file(CPatcher *patcher,
                                            ProgressUpdatedCallback progress_cb,
                                      FilesUpdatedCallback files_cb,
                                      DetailsUpdatedCallback details_cb,
                                      void *userdata);
MB_EXPORT void mbpatcher_patcher_cancel_patching(CPatcher *patcher);


MB_EXPORT /* enum ErrorCode */ int mbpatcher_autopatcher_error(const CAutoPatcher *patcher);
MB_EXPORT char * mbpatcher_autopatcher_id(const CAutoPatcher *patcher);
MB_EXPORT char ** mbpatcher_autopatcher_new_files(const CAutoPatcher *patcher);
MB_EXPORT char ** mbpatcher_autopatcher_existing_files(const CAutoPatcher *patcher);
MB_EXPORT bool mbpatcher_autopatcher_patch_files(CAutoPatcher *patcher, const char *directory);

MB_END_C_DECLS
