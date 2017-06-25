/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include "mbp/cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*ProgressUpdatedCallback) (uint64_t, uint64_t, void *);
typedef void (*FilesUpdatedCallback) (uint64_t, uint64_t, void *);
typedef void (*DetailsUpdatedCallback) (const char *, void *);

MB_EXPORT /* enum ErrorCode */ int mbp_patcher_error(const CPatcher *patcher);
MB_EXPORT char * mbp_patcher_id(const CPatcher *patcher);
MB_EXPORT void mbp_patcher_set_fileinfo(CPatcher *patcher, const CFileInfo *info);
MB_EXPORT bool mbp_patcher_patch_file(CPatcher *patcher,
                                      ProgressUpdatedCallback progressCb,
                                      FilesUpdatedCallback filesCb,
                                      DetailsUpdatedCallback detailsCb,
                                      void *userData);
MB_EXPORT void mbp_patcher_cancel_patching(CPatcher *patcher);


MB_EXPORT /* enum ErrorCode */ int mbp_autopatcher_error(const CAutoPatcher *patcher);
MB_EXPORT char * mbp_autopatcher_id(const CAutoPatcher *patcher);
MB_EXPORT char ** mbp_autopatcher_new_files(const CAutoPatcher *patcher);
MB_EXPORT char ** mbp_autopatcher_existing_files(const CAutoPatcher *patcher);
MB_EXPORT bool mbp_autopatcher_patch_files(CAutoPatcher *patcher, const char *directory);


#ifdef __cplusplus
}
#endif
