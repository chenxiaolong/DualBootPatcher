/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

CPatcherConfig * mbp_config_create(void);
void mbp_config_destroy(CPatcherConfig *pc);

CPatcherError * mbp_config_error(const CPatcherConfig *pc);

char * mbp_config_data_directory(const CPatcherConfig *pc);
char * mbp_config_temp_directory(const CPatcherConfig *pc);

void mbp_config_set_data_directory(CPatcherConfig *pc, char *path);
void mbp_config_set_temp_directory(CPatcherConfig *pc, char *path);

char * mbp_config_version(const CPatcherConfig *pc);
CDevice ** mbp_config_devices(const CPatcherConfig *pc);
#ifndef LIBMBP_MINI
char ** mbp_config_patchers(const CPatcherConfig *pc);
char ** mbp_config_autopatchers(const CPatcherConfig *pc);
char ** mbp_config_ramdiskpatchers(const CPatcherConfig *pc);

CPatcher * mbp_config_create_patcher(CPatcherConfig *pc, const char *id);
CAutoPatcher * mbp_config_create_autopatcher(CPatcherConfig *pc,
                                             const char *id,
                                             const CFileInfo *info);
CRamdiskPatcher * mbp_config_create_ramdisk_patcher(CPatcherConfig *pc,
                                                    const char *id,
                                                    const CFileInfo *info,
                                                    CCpioFile *cpio);

void mbp_config_destroy_patcher(CPatcherConfig *pc, CPatcher *patcher);
void mbp_config_destroy_autopatcher(CPatcherConfig *pc, CAutoPatcher *patcher);
void mbp_config_destroy_ramdisk_patcher(CPatcherConfig *pc, CRamdiskPatcher *patcher);
#endif

#ifdef __cplusplus
}
#endif
