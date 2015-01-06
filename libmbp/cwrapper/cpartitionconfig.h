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

#include <stddef.h>

#include "cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

const char * mbp_partconfig_system(void);
const char * mbp_partconfig_cache(void);
const char * mbp_partconfig_data(void);

CPartConfig * mbp_partconfig_create(void);
void mbp_partconfig_destroy(CPartConfig *config);

char * mbp_partconfig_name(const CPartConfig *config);
void mbp_partconfig_set_name(CPartConfig *config, const char *name);

char * mbp_partconfig_description(const CPartConfig *config);
void mbp_partconfig_set_description(CPartConfig *config,
                                    const char *description);

char * mbp_partconfig_kernel(const CPartConfig *config);
void mbp_partconfig_set_kernel(CPartConfig *config, const char *kernel);

char * mbp_partconfig_id(const CPartConfig *config);
void mbp_partconfig_set_id(CPartConfig *config, const char *id);

char * mbp_partconfig_target_system(const CPartConfig *config);
void mbp_partconfig_set_target_system(CPartConfig *config, const char *path);

char * mbp_partconfig_target_cache(const CPartConfig *config);
void mbp_partconfig_set_target_cache(CPartConfig *config, const char *path);

char * mbp_partconfig_target_data(const CPartConfig *config);
void mbp_partconfig_set_target_data(CPartConfig *config, const char *path);

char * mbp_partconfig_target_system_partition(const CPartConfig *config);
void mbp_partconfig_set_target_system_partition(CPartConfig *config,
                                                const char *partition);

char * mbp_partconfig_target_cache_partition(const CPartConfig *config);
void mbp_partconfig_set_target_cache_partition(CPartConfig *config,
                                               const char *partition);

char * mbp_partconfig_target_data_partition(const CPartConfig *config);
void mbp_partconfig_set_target_data_partition(CPartConfig *config,
                                              const char *partition);

#ifdef __cplusplus
}
#endif
