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

#include "cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*MaxProgressUpdatedCallback) (int, void *);
typedef void (*ProgressUpdatedCallback) (int, void *);
typedef void (*DetailsUpdatedCallback) (const char *, void *);

CPatcherError * mbp_patcher_error(const CPatcher *patcher);
char * mbp_patcher_id(const CPatcher *patcher);
bool mbp_patcher_uses_patchinfo(const CPatcher *patcher);
void mbp_patcher_set_fileinfo(CPatcher *patcher, const CFileInfo *info);
char * mbp_patcher_new_file_path(CPatcher *patcher);
int mbp_patcher_patch_file(CPatcher *patcher,
                           MaxProgressUpdatedCallback maxProgressCb,
                           ProgressUpdatedCallback progressCb,
                           DetailsUpdatedCallback detailsCb,
                           void *userData);
void mbp_patcher_cancel_patching(CPatcher *patcher);


CPatcherError * mbp_autopatcher_error(const CAutoPatcher *patcher);
char * mbp_autopatcher_id(const CAutoPatcher *patcher);
char ** mbp_autopatcher_new_files(const CAutoPatcher *patcher);
char ** mbp_autopatcher_existing_files(const CAutoPatcher *patcher);
int mbp_autopatcher_patch_files(CAutoPatcher *patcher, const char *directory);


CPatcherError * mbp_ramdiskpatcher_error(const CRamdiskPatcher *patcher);
char * mbp_ramdiskpatcher_id(const CRamdiskPatcher *patcher);
int mbp_ramdiskpatcher_patch_ramdisk(CRamdiskPatcher *patcher);


#ifdef __cplusplus
}
#endif
