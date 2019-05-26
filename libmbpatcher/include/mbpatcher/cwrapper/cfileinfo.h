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
#include "mbdevice/capi/device.h"
#include "mbpatcher/cwrapper/ctypes.h"

MB_BEGIN_C_DECLS

MB_EXPORT CFileInfo * mbpatcher_fileinfo_create(void);
MB_EXPORT void mbpatcher_fileinfo_destroy(CFileInfo *info);

MB_EXPORT char * mbpatcher_fileinfo_input_path(const CFileInfo *info);
MB_EXPORT void mbpatcher_fileinfo_set_input_path(CFileInfo *info, const char *path);

MB_EXPORT char * mbpatcher_fileinfo_output_path(const CFileInfo *info);
MB_EXPORT void mbpatcher_fileinfo_set_output_path(CFileInfo *info, const char *path);

MB_EXPORT CDevice * mbpatcher_fileinfo_device(const CFileInfo *info);
MB_EXPORT void mbpatcher_fileinfo_set_device(CFileInfo *info, CDevice *device);

MB_EXPORT char * mbpatcher_fileinfo_rom_id(const CFileInfo *info);
MB_EXPORT void mbpatcher_fileinfo_set_rom_id(CFileInfo *info, const char *id);

MB_END_C_DECLS
