/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

CFileInfo * mbp_fileinfo_create(void);
void mbp_fileinfo_destroy(CFileInfo *info);

char * mbp_fileinfo_filename(const CFileInfo *info);
void mbp_fileinfo_set_filename(CFileInfo *info, const char *path);

CPatchInfo * mbp_fileinfo_patchinfo(const CFileInfo *info);
void mbp_fileinfo_set_patchinfo(CFileInfo *info, CPatchInfo *pInfo);

CDevice * mbp_fileinfo_device(const CFileInfo *info);
void mbp_fileinfo_set_device(CFileInfo *info, CDevice * device);

CPartConfig * mbp_fileinfo_partconfig(const CFileInfo *info);
void mbp_fileinfo_set_partconfig(CFileInfo *info, CPartConfig *config);

#ifdef __cplusplus
}
#endif
