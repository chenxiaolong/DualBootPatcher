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

#include "mbcommon/common.h"
#include "mbp/cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

MB_EXPORT CPatcherConfig * mbp_config_create(void);
MB_EXPORT void mbp_config_destroy(CPatcherConfig *pc);

MB_EXPORT /* enum ErrorCode */ int mbp_config_error(const CPatcherConfig *pc);

MB_EXPORT char * mbp_config_data_directory(const CPatcherConfig *pc);
MB_EXPORT char * mbp_config_temp_directory(const CPatcherConfig *pc);

MB_EXPORT void mbp_config_set_data_directory(CPatcherConfig *pc, char *path);
MB_EXPORT void mbp_config_set_temp_directory(CPatcherConfig *pc, char *path);

MB_EXPORT char ** mbp_config_patchers(const CPatcherConfig *pc);
MB_EXPORT char ** mbp_config_autopatchers(const CPatcherConfig *pc);

MB_EXPORT CPatcher * mbp_config_create_patcher(CPatcherConfig *pc, const char *id);
MB_EXPORT CAutoPatcher * mbp_config_create_autopatcher(CPatcherConfig *pc,
                                                       const char *id,
                                                       const CFileInfo *info);

MB_EXPORT void mbp_config_destroy_patcher(CPatcherConfig *pc, CPatcher *patcher);
MB_EXPORT void mbp_config_destroy_autopatcher(CPatcherConfig *pc, CAutoPatcher *patcher);

#ifdef __cplusplus
}
#endif
