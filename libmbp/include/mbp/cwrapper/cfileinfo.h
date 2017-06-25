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

#include "mbcommon/common.h"
#include "mbdevice/device.h"
#include "mbp/cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

MB_EXPORT CFileInfo * mbp_fileinfo_create(void);
MB_EXPORT void mbp_fileinfo_destroy(CFileInfo *info);

MB_EXPORT char * mbp_fileinfo_input_path(const CFileInfo *info);
MB_EXPORT void mbp_fileinfo_set_input_path(CFileInfo *info, const char *path);

MB_EXPORT char * mbp_fileinfo_output_path(const CFileInfo *info);
MB_EXPORT void mbp_fileinfo_set_output_path(CFileInfo *info, const char *path);

MB_EXPORT struct Device * mbp_fileinfo_device(const CFileInfo *info);
MB_EXPORT void mbp_fileinfo_set_device(CFileInfo *info, struct Device * device);

MB_EXPORT char * mbp_fileinfo_rom_id(const CFileInfo *info);
MB_EXPORT void mbp_fileinfo_set_rom_id(CFileInfo *info, const char *id);

#ifdef __cplusplus
}
#endif
