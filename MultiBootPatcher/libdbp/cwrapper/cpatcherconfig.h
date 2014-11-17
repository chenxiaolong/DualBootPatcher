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

#ifndef C_PATCHERCONFIG_H
#define C_PATCHERCONFIG_H

#include "cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

CPatcherConfig * mbp_config_create();
void mbp_config_destroy(CPatcherConfig *pc);

CPatcherError * mbp_config_error(const CPatcherConfig *pc);

char * mbp_config_binaries_directory(const CPatcherConfig *pc);
char * mbp_config_data_directory(const CPatcherConfig *pc);
char * mbp_config_inits_directory(const CPatcherConfig *pc);
char * mbp_config_patches_directory(const CPatcherConfig *pc);
char * mbp_config_patchinfos_directory(const CPatcherConfig *pc);
char * mbp_config_scripts_directory(const CPatcherConfig *pc);

void mbp_config_set_binaries_directory(CPatcherConfig *pc, char *path);
void mbp_config_set_data_directory(CPatcherConfig *pc, char *path);
void mbp_config_set_inits_directory(CPatcherConfig *pc, char *path);
void mbp_config_set_patches_directory(CPatcherConfig *pc, char *path);
void mbp_config_set_patchinfos_directory(CPatcherConfig *pc, char *path);
void mbp_config_set_scripts_directory(CPatcherConfig *pc, char *path);

char * mbp_config_version(const CPatcherConfig *pc);
CDevice ** mbp_config_devices(const CPatcherConfig *pc);
CPatchInfo ** mbp_config_patchinfos(const CPatcherConfig *pc);
CPatchInfo ** mbp_config_patchinfos_for_device(const CPatcherConfig *pc,
                                               const CDevice *device);

CPatchInfo * mbp_config_find_matching_patchinfo(const CPatcherConfig *pc,
                                                CDevice *device,
                                                const char *filename);

char ** mbp_config_patchers(const CPatcherConfig *pc);
char ** mbp_config_autopatchers(const CPatcherConfig *pc);
char ** mbp_config_ramdiskpatchers(const CPatcherConfig *pc);

char * mbp_config_patcher_name(const CPatcherConfig *pc, const char *id);

CPatcher * mbp_config_create_patcher(CPatcherConfig *pc, const char *id);
CAutoPatcher * mbp_config_create_autopatcher(CPatcherConfig *pc,
                                             const char *id,
                                             const CFileInfo *info,
                                             const CStringMap *args);
CRamdiskPatcher * mbp_config_create_ramdisk_patcher(CPatcherConfig *pc,
                                                    const char *id,
                                                    const CFileInfo *info,
                                                    CCpioFile *cpio);

void mbp_config_destroy_patcher(CPatcherConfig *pc, CPatcher *patcher);
void mbp_config_destroy_autopatcher(CPatcherConfig *pc, CAutoPatcher *patcher);
void mbp_config_destroy_ramdisk_patcher(CPatcherConfig *pc, CRamdiskPatcher *patcher);

CPartConfig ** mbp_config_partitionconfigs(const CPatcherConfig *pc);

char ** mbp_config_init_binaries(const CPatcherConfig *pc);

bool mbp_config_load_patchinfos(CPatcherConfig *pc);

#ifdef __cplusplus
}
#endif

#endif // C_PATCHERCONFIG_H
